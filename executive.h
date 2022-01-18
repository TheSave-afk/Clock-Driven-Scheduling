#ifndef EXECUTIVE_H
#define EXECUTIVE_H

#include <vector>
#include <functional>
#include <chrono>
#include <thread>
#include <mutex>
#include <condition_variable>


class Executive
{
	public:
		/* Inizializza l'executive, impostando i parametri di scheduling:
			num_tasks: numero totale di task presenti nello schedule;
			frame_length: lunghezza del frame (in quanti temporali);
			unit_duration: durata dell'unita di tempo, in millisecondi (default 10ms).
		*/
		Executive(size_t num_tasks, unsigned int frame_length, unsigned int unit_duration = 1000);

		/* Imposta il task periodico di indice "task_id" (da invocare durante la creazione dello schedule):
			task_id: indice progressivo del task, nel range [0, num_tasks);
			periodic_task: funzione da eseguire al rilascio del task;
			wcet: tempo di esecuzione di caso peggiore (in quanti temporali).
		*/
		void set_periodic_task(size_t task_id, std::function<void()> periodic_task, unsigned int wcet);

		/* Imposta il task aperiodico (da invocare durante la creazione dello schedule):
			aperiodic_task: funzione da eseguire al rilascio del task;
			wcet: tempo di esecuzione di caso peggiore (in quanti temporali).
		*/
		void set_aperiodic_task(std::function<void()> aperiodic_task, unsigned int wcet);

		/* Lista di task da eseguire in un dato frame (da invocare durante la creazione dello schedule):
			frame: lista degli id corrispondenti ai task da eseguire nel frame, in sequenza
		*/
		void add_frame(std::vector<size_t> frame);

		/* Esegue l'applicazione */
		void run();

		/* Richiede il rilascio del task aperiodico (da invocare durante l'esecuzione):
			ritorna il numero di frame necessari per completare l'esecuzione del task (caso peggiore).
		*/
		unsigned int ap_task_request();

	private:

		enum task_state {IDLE, RUNNING, PENDING};

		struct task_data
		{
			std::function<void()> function;
			unsigned int wcet;

			std::thread thread;

			std::mutex mutex;
			std::condition_variable cond;
			task_state state;
		};

		std::mutex exec_mutex;

		std::vector<task_data> p_tasks;	// vettore tasks periodici
		task_data ap_task;							// task aperiodico (uno solo)
		unsigned int ap_request;

		std::vector< std::vector<size_t> > frames;		// vettore dei frames
		std::vector<unsigned int> slack_times; // vettore di storage degli slack time per ogni frame


		const unsigned int frame_length; // lunghezza del frame (in quanti temporali)
		const std::chrono::milliseconds unit_time; // durata dell'unita di tempo (quanto temporale)
		unsigned int current_frame_id; // si tiene traccia del frame corrente

		/* ... */

		std::vector<int> deadl_miss;
		static void task_function(task_data & task);//in questo modo è una funzione esterna alla classe che non ha accesso hai dati che gli vengono passati
		//una la deve eseguire il thread dell'exe e l'altra viene eseguita da tutti gli altri thread , l'exe deve avere accesso a tutti questi dati
		void exec_function();//ha accesso ai dati su cui viene chiamato
};

#endif
