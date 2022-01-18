#include <cassert>
#include <iostream>
#include <chrono>
#include <ctime>
#include <sys/time.h>

#include "executive.h"

#include "rt/priority.h"
#include "rt/affinity.h"


Executive::Executive(size_t num_tasks, unsigned int frame_length, unsigned int unit_duration)
	: p_tasks(num_tasks), frame_length(frame_length), unit_time(unit_duration)
{

}

void Executive::set_periodic_task(size_t task_id, std::function<void()> periodic_task, unsigned int wcet)
{
	assert(task_id < p_tasks.size()); // Fallisce in caso di task_id non corretto (fuori range)

	p_tasks[task_id].function = periodic_task;
	p_tasks[task_id].wcet = wcet;
}

void Executive::set_aperiodic_task(std::function<void()> aperiodic_task, unsigned int wcet)
{
 	ap_task.function = aperiodic_task;
 	ap_task.wcet = wcet;
}

void Executive::add_frame(std::vector<size_t> frame)
{
	for (auto & id: frame)
		assert(id < p_tasks.size()); // Fallisce in caso di task_id non corretto (fuori range)

	frames.push_back(frame);

	//calcolo dello slack time
	unsigned int periodic_tasks_execution_time = 0;
	unsigned int ptask_index;

	for (unsigned int i = 0; i < frame.size(); ++i)
	{
		ptask_index =  frame[i];
		periodic_tasks_execution_time += p_tasks[ptask_index].wcet;
	}

	unsigned int this_frame_slack_time = (frame_length - periodic_tasks_execution_time);
  slack_times.push_back(this_frame_slack_time);

}

void Executive::run()
{

	rt::priority exec_prio(rt::priority::rt_max); //l'executive avrà massima priorià
	rt::priority aperi_prio(rt::priority::rt_min); //il task aperiodico avrà priorità minima

	rt::affinity aff("1");

	for (size_t id = 0; id < p_tasks.size(); ++id)
	{
		assert(p_tasks[id].function); // Fallisce se set_periodic_task() non e' stato invocato per questo id

		p_tasks[id].thread = std::thread(&Executive::task_function, std::ref(p_tasks[id]));

		rt::set_affinity(p_tasks[id].thread, aff);

		deadl_miss.push_back(0);


	}

	std::thread exec_thread(&Executive::exec_function, this);

//setto la priorità dell'executive
	try
	{
		rt::set_priority(exec_thread, exec_prio);
	}
	catch (rt::permission_error & e)
	{
		std::cerr << "Error setting RT priorities on exec: " << e.what() << std::endl;
		return;
	}

	assert(ap_task.function); // Fallisce se set_aperiodic_task() non e' stato invocato

	ap_task.thread = std::thread(&Executive::task_function, std::ref(ap_task));	// creazione task aperiodico


	rt::set_affinity(ap_task.thread, aff);

	//setto la priorità del task aperiodico
	try
	{
		rt::set_priority(ap_task.thread, aperi_prio);
	}
	catch (rt::permission_error & e)
	{
		std::cerr << "Error setting RT priorities on aperiodic task: " << e.what() << std::endl;
		return;
	}

	exec_thread.join();

	ap_task.thread.join();

	for (auto & pt: p_tasks)
		pt.thread.join();

}

unsigned int Executive::ap_task_request()
{
	unsigned int needed_frames_number = 0;
	unsigned int time = 0;
	unsigned int frame = 0;

	{
		std::unique_lock<std::mutex> lock(exec_mutex);
 	  frame = current_frame_id;
		ap_request = 1;
	}

	while (time < ap_task.wcet)
	{
		time += slack_times[frame];
		needed_frames_number++;
		frame = (frame + 1) % (frames.size());
	}

	return needed_frames_number;
}


void Executive::task_function(Executive::task_data & task)
{
	while(true)
	{
		{
			std::unique_lock<std::mutex> lock(task.mutex);

			task.state = IDLE;

			while( task.state != PENDING )
				task.cond.wait(lock);

			task.state = RUNNING;
		}
			task.function();
		}
}

void Executive::exec_function()
{

	unsigned int frame_id = 0;


	while (true)
	{
		//calcolo tempo di risveglio dell'executive
		auto point = std::chrono::steady_clock::now();
		point += std::chrono::milliseconds(((unit_time * frame_length))); // punto in cui si risveglia l'executive


		std::cout << "\n*** FRAME " << current_frame_id << " ***" << std::endl;


		// DEADLINE MISS TASK PERIODICI
		for(size_t i=0; i < p_tasks.size(); ++i)
		{
			{
				std::unique_lock<std::mutex> lock(p_tasks[i].mutex);

				if(p_tasks[i].state != IDLE)
				{
					std::cout<<"DEADLINE MISS --> Task " << i << std::endl;
					deadl_miss[i] = 1;
				}

		/*		if (p_tasks[i].state == PENDING )
				{
						{
							std::cout << "INTERRUZIONE TASK " << i << std::endl;
							{
								std::unique_lock<std::mutex> lock(p_tasks[i].mutex);
								p_tasks[i].state = IDLE;
							}
						}
				}
				*/
			}
		}


/* Rilascio dei task periodici del frame corrente e aperiodico (se necessario)... */
		const std::vector<size_t>&f = frames[frame_id]; // estraggo il vettore che contiene gli indici e task numbers diventa l'indice del vettore dei task


		// APERIODICO
			if(ap_request)
			{
				{
					std::unique_lock<std::mutex> lock(exec_mutex);
					ap_request = 0;
				}

				{
					std::unique_lock<std::mutex> lock(ap_task.mutex);
					if(ap_task.state != IDLE)
					{
						std::cout<<"DEADLINE MISS Task aperiodico " << std::endl;
					}
					else
					{
						ap_task.state = PENDING;
						ap_task.cond.notify_one();
					}
				}
		 }


	// RISVEGLIO TASK CORRETTI PER OGNI FRAME
		for( unsigned int i=0; i<f.size(); ++i)
		{

				size_t tasks_number = f[i];
				rt::priority pri(rt::priority::rt_max -1 - i);

				if(!deadl_miss[tasks_number])
				{
					{
						std::unique_lock<std::mutex> lock(p_tasks[tasks_number].mutex);
						p_tasks[tasks_number].state = PENDING;
					}

				try
				{
					rt::set_priority(p_tasks[tasks_number].thread, pri);
				}
				catch (rt::permission_error & e)
				{
					std::cerr << "Error setting RT priorities on "<< tasks_number <<" periodic task: " << e.what() << std::endl;
					return;
				}
				p_tasks[tasks_number].cond.notify_one();
			 }
			 else
			 {
			 		deadl_miss[tasks_number] = 0;
					std::cout << "Il task " << i <<" è stato saltato" << '\n';
			 }
	  }


	// AGGIORNAMENTO FRAME CORRENTE
		{
			std::unique_lock<std::mutex> lock(exec_mutex);
			if (++frame_id == frames.size())
				frame_id = 0;

			current_frame_id = frame_id;
		}


		/* Attesa fino al prossimo inizio frame ... */   // ATTESA ASSOLUTA
		std::this_thread::sleep_until(point);

		std::cout << "\n-- EXECUTIVE --" 	<< '\n';

	}
}

//per aperiodici: richiamare al funzione ap_task_request() senza parametri, nell'exe c'è un metodo ap_task_request che può essere invocato nel codice dell'applicazione ovvero exec.ap_task_request
