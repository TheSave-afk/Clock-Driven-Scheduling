#include "executive.h"
#include "busy_wait.h"
#include <iostream>
#include <sstream>
#include "rt/priority.h"

#define WAIT 1000

using namespace std;

unsigned int counter = 0;

Executive exec(5, 4);

void task0()
{
	/* Custom Code */

	rt::priority my_prio(rt::this_thread::get_priority());

	cout << "Sono il task 0 ed eseguo con priorità " << my_prio << endl;

	busy_wait(WAIT);

	cout << "Sono il task 0 e ho finito\n" << endl;

}

void task1()
{
	rt::priority my_prio(rt::this_thread::get_priority());

	cout << "Sono il task 1 ed eseguo con priorità " << my_prio << endl;

	busy_wait(WAIT);

	if(++counter % 3 == 0)
	{
		unsigned int total_frames = exec.ap_task_request();
		cout << "AP: RICHIESTO TASK APERIODICO - Frames necessari = " << total_frames << " frames\n" << endl;
	}

	cout << "Sono il task 1 e ho finito\n" << endl;

}

void task2()
{
	rt::priority my_prio(rt::this_thread::get_priority());

	cout << "Sono il task 2 ed eseguo con priorità " << my_prio << endl;

	busy_wait(WAIT);

	cout << "Sono il task 2 e ho finito\n" << endl;

}

void task3()
{
	rt::priority my_prio(rt::this_thread::get_priority());

	cout << "Sono il task 3 ed eseguo con priorità " << my_prio << endl;

	busy_wait(WAIT);

	cout << "Sono il task 3 e ho finito\n" << endl;
}

void task4()
{
	rt::priority my_prio(rt::this_thread::get_priority());

	cout << "Sono il task 4 ed eseguo con priorità " << my_prio << endl;

	busy_wait(WAIT);

	cout << "Sono il task 4 e ho finito\n" << endl;
}

/* Nota: nel codice di uno o piu' task periodici e' lecito chiamare Executive::ap_task_request() */

void ap_task()
{
	// Il task periodico esegue
	rt::priority my_prio(rt::this_thread::get_priority());
	cout << "Sono il task aperiodico ed eseguo con priorità " << my_prio << endl;

	busy_wait(WAIT);

	/* Custom aperiodic task code */
}

int main()
{
	busy_wait_init();

	exec.set_periodic_task(0, task0, 1); // tau_1
	exec.set_periodic_task(1, task1, 2); // tau_2
	exec.set_periodic_task(2, task2, 1); // tau_3,1
	exec.set_periodic_task(3, task3, 3); // tau_3,2
	exec.set_periodic_task(4, task4, 2); // tau_3,3
	/* ... */

	exec.set_aperiodic_task(ap_task, 2);
	//

	exec.add_frame({0,1,2});
	exec.add_frame({0,3});
	exec.add_frame({0,1});
	exec.add_frame({0,1});
	exec.add_frame({0,2,4});
	/* ... */

	exec.run();

	return 0;
}
