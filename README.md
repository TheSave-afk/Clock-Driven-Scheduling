# Clock-Driven-Scheduling
Developing of a real time scheduling system that  execute a set of periodic tasks and perform one aperiodic task according to a clock driven approach, using a static scheduling algorithm. The release of the aperiodic task occours sporadically during the system operations.

How the system works?

The project run on GNU / Linux operating system, with the usage of the API to manage the multithreaded programming provided natively by the C ++ 11 language and the POSIX extensions for real-time scheduling.

Some considerations:
Task: Each task has its own execution frame, there is a thread for each task, whose execution is controlled by the executive, which governs its activation at the beginning of each frame and checks that it has ended in time, upon expiry of the deadline. The synchronization is based on condition and mutex variables.

Executive: The executive starts at the beginning of each frame, with the lowest possible latency, and guarantee the continuation of the execution of the tasks does not delay the next activation of the executive. To do this the executive it is executed on a special thread and the priorities of the threads involved in the system is calibrated through FIFO real-time scheduling.

Deadline miss: The recovery procedure is the emission of an error signal on the standard output.

Test & Debug: To check the real duration of the single tasks and to verify the real instants of activation of the threads, are used the timers made available by the C ++ language on the hardware used.

Aperiodic Task considerations:
The execution of the aperiodic task make use of the slack time present in the immediate frames immediately after the release date, without interfering with compliance with the deadline periodic tasks.

The release request is only made by one or more periodic tasks, by invoking a specific ap_task_request () function. This function will return to a worst case estimate of the number of frames needed for the completion of the required aperiodic task. 

The execution of the aperiodic task is considered correct when its execution ends within the next request for release, therefore a deadline miss check must be carried out that emits an error signal on the standard output when the executive, activating a new instance of the aperiodic task, verifies that its previous execution is still running. In this situation, one or more can be skipped subsequent executions of the aperiodic task. 

For simplicity it is assumed that it is not permissible to request more than one activation of the task aperiodic within the same frame.
