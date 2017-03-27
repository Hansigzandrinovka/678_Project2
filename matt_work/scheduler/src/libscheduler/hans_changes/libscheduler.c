/** @file libscheduler.c
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "libscheduler.h"
#include "../libpriqueue/libpriqueue.h"

//added
scheme_t current_scheduling_mode; //monitors the current scheduling mode of the system
_priqueue_t pending_tasks; //tracks pending tasks to be scheduled next
_priqueue_t finished_tasks; //tracks finished tasks to schedule monitoring statistics
//int is_preempting = 0; //(boolean) tracks whether currently running tasks can be kicked off of their cores by preemption
*Comparer preemption_comparer = 0; //if not 0, points to function for preemptive comparison

/**
  Stores information making up a job to be scheduled including any statistics.

  You may need to define some global variables or a struct to store your job queue elements. 
*/

//have core queue, and ready queue. When preempted 
typedef struct _job_t
{
	int job_number; //stores job identifier
	int job_time; //expected runtime to complete
	int run_time; //time spent running
	int wait_time; //time spent waiting in pending_tasks queue
	int priority; //how little user wants job to go first
} job_t;

//If job a is more important than job b (should be closer to the front) return -1, else return 1

//in fcfs, all new jobs come after all current jobs
int sort_by_fcfs_or_rr(const void *hopefully_job_a, const void *hopefully_job_b)
{
	//***convert the void pointers to jobs
		//
		//not needed since the outcome is preordained
	//***compare job properties
	//
	return 1;
}
//compare job_time to see which will be finished first
int sort_by_shortest_job(const void *hopefully_job_a, const void *hopefully_job_b)
{
	//***convert the void pointers to jobs
	_job_t *job_a = (_job_t*)hopefully_job_a; //convert pointer to a job struct
	_job_t *job_b = (_job_t*)hopefully_job_b;
	//***compare job properties
	if(job_a->run_time > job_b->run_time)
		return -1;
	return 1;
}
int sort_by_priority(const void *hopefully_job_a, const void *hopefully_job_b)
{
	//***convert the void pointers to jobs
	_job_t *job_a = (_job_t*)hopefully_job_a; //convert pointer to a job struct
	_job_t *job_b = (_job_t*)hopefully_job_b;
	//***compare job properties
	if(job_a->priority > job_b->priority)
		return -1;
	return 1;
}


//queue prioritization methods:

//int(*comparer)(const void *, const void *)


/**
  Initalizes the scheduler.
 
  Assumptions:
    - You may assume this will be the first scheduler function called.
    - You may assume this function will be called once once.
    - You may assume that cores is a positive, non-zero number.
    - You may assume that scheme is a valid scheduling scheme.

  @param cores the number of cores that is available by the scheduler. These cores will be known as core(id=0), core(id=1), ..., core(id=cores-1).
  @param scheme  the scheduling scheme that should be used. This value will be one of the six enum values of scheme_t
*/
void scheduler_start_up(int cores, scheme_t scheme)
{
	//run $cores cores in $scheme mode (rr,fcfs,sjf,...)
	current_scheduling_mode = scheme;
	//initialize and configure pending_tasks based on which scheme to prioritize with
	switch(scheme)
	{
		case(scheme_t.FCFS):
		{
			priqueue_init(&pending_tasks,sort_by_fcfs_or_rr);
		}
		case(scheme_t.SJF):
		{
			priqueue_init(&pending_tasks,sort_by_shortest_job);
		}
		case(scheme_t.PSJF):
		{
			//like shortest-job-first, but with preemption function defined, will try to preempt before adding to queue
			priqueue_init(&pending_tasks,sort_by_shortest_job);
			preemption_comparer = sort_by_shortest_job;
		}
		case(scheme_t.PRI):
		{
			priqueue_init(&pending_tasks,sort_by_priority);
		}
		case(scheme_t.PPRI):
		{
			priqueue_init(&pending_tasks,sort_by_priority);
			preemption_comparer = sort_by_priority;
		}
		case(scheme_t.RR):
		{
			priqueue_init(&pending_tasks,sort_by_fcfs_or_rr);
			//TODO: configure flag to notify of quantum-based prioritization
		}
	}
	priqueue_init(&finished_tasks,sort_by_fcfs_or_rr); //we don't care how finished jobs are sorted, we just want them stored somewhere
	//TODO: allocate cores
}


/**
  Called when a new job arrives.
 
  If multiple cores are idle, the job should be assigned to the core with the
  lowest id.
  If the job arriving should be scheduled to run during the next
  time cycle, return the zero-based index of the core the job should be
  scheduled on. If another job is already running on the core specified,
  this will preempt the currently running job.
  Assumptions:
    - You may assume that every job wil have a unique arrival time.

  @param job_number a globally unique identification number of the job arriving.
  @param time the current time of the simulator.
  @param running_time the total number of time units this job will run before it will be finished.
  @param priority the priority of the job. (The lower the value, the higher the priority.)
  @return index of core job should be scheduled on
  @return -1 if no scheduling changes should be made. 
 
 */
int scheduler_new_job(int job_number, int time, int running_time, int priority)
{
	//when new job arrives, decide what to do with it depending on which scheduler mode we are in
	struct _job_t *new_job; //creates a job to store on queue/push to core
	new_job = malloc(sizeof(new_job));
	new_job->job_number = job_number;
	new_job->job_time = time;
	new_job->wait_time = 0;
	new_job->run_time = running_time;
	new_job->priority = priority;
	
	//if our preempting flag is true, compare against active processes
	//then/else push to queue (which has already been configured to prioritize based on scheduler mode)
	if(preemption_comparer != 0) //if we might preempt current jobs
	{
		//***check against current jobs, preempt if need be, otherwise continue
	}
	//reaching here, either not preempting, or did not need to preempt, so add to queue
	priqueue_offer(*new_job); //push the job onto the queue (it handles prioritizing it)
	
	//if a core gets preempted by new job, return the id of core that gets preempted
	
	return -1; //we were unable to schedule this job to a core (because cores are busy / job wasn't preempted)
}


/**
  Called when a job has completed execution.
 
  The core_id, job_number and time parameters are provided for convenience. You may be able to calculate the values with your own data structure.
  If any job should be scheduled to run on the core free'd up by the
  finished job, return the job_number of the job that should be scheduled to
  run on core core_id.
 
  @param core_id the zero-based index of the core where the job was located.
  @param job_number a globally unique identification number of the job.
  @param time the current time of the simulator.
  @return job_number of the job that should be scheduled to run on core core_id
  @return -1 if core should remain idle.
 */
int scheduler_job_finished(int core_id, int job_number, int time)
{
	struct _job_t *finished_job;
	//***retrieve finished job from core
	priqueue_offer(finished_tasks,finished_job); //mark job as finished, keep it tracked for usage statistics
	struct _job_t *next_job; //a pointer the next job (or zero if no next job) that will run on current core
	next_job = 0; //points to nullptr
	if(priqueue_size(pending_tasks) >= 1) //if there are still jobs to run,
	{
		next_job = priqueue_remove_at(pending_tasks,0); //grab first job from queue and run it
		//allocate this job to the core that just finished
	}
	return -1;
}


/**
  When the scheme is set to RR, called when the quantum timer has expired
  on a core.
 
  If any job should be scheduled to run on the core free'd up by
  the quantum expiration, return the job_number of the job that should be
  scheduled to run on core core_id.

  @param core_id the zero-based index of the core where the quantum has expired.
  @param time the current time of the simulator. 
  @return job_number of the job that should be scheduled on core cord_id
  @return -1 if core should remain idle
 */
int scheduler_quantum_expired(int core_id, int time)
{
	struct _job_t *finished_job; //the job that core just stopped acting on
	//***grab the job from the core that has it
	finished_job->run_time = time;
	//***push the job back onto the queue
	priqueue_offer(pending_tasks,finished_job);
	//***fetch the next job off of the queue
	if(priqueue_size(pending_tasks) >= 1) //if there are still jobs to grab,
	{
		//grab the next one and run it
		struct _job_t *next_job; //the next job that will run on core
		next_job = priqueue_remove_at(0); //grab first job off of queue
		
		//***make core start using new job
		
		return next_job->job_number;
	}
	return -1;
}


//for each loop, update active jobs, 

/**
  Returns the average waiting time of all jobs scheduled by your scheduler.

  Assumptions:
    - This function will only be called after all scheduling is complete (all jobs that have arrived will have finished and no new jobs will arrive).
  @return the average waiting time of all jobs scheduled.
 */
float scheduler_average_waiting_time()
{
	int running_total_wait_time = 0.0; //incremented by elements in queue
	int i = 0; //queue index marker
	int num_tasks = priqueue_size(finished_tasks);
	for(i = 0; i < num_tasks; i++) //get each finished task's total wait time
	{
		running_total_wait_time += priqueue_at(finished_tasks,i)->wait_time;
	}
	//for each element in finished_job queue, get its wait time, add them together, divide by size
	return (running_total_wait_time / num_tasks);
}


/**
  Returns the average turnaround time of all jobs scheduled by your scheduler.

  Assumptions:
    - This function will only be called after all scheduling is complete (all jobs that have arrived will have finished and no new jobs will arrive).
  @return the average turnaround time of all jobs scheduled.
 */
float scheduler_average_turnaround_time()
{
	//turnaround time, or time to complete task, is run_time + wait_time
	int running_total_turnaround_time = 0.0; //incremented by elements in queue
	int i = 0; //queue index marker
	int num_tasks = priqueue_size(finished_tasks);
	for(i = 0; i < num_tasks; i++) //get each finished task's total wait time
	{
		struct _job_t job_reference = priqueue_at(finished_job,i);
		running_total_turnaround_time += job_reference->wait_time + job_reference->run_time;
	}
	//for each element in finished_job queue, get its wait time, add them together, divide by size
	return (running_total_turnaround_time / num_tasks);
}


/**
  Returns the average response time of all jobs scheduled by your scheduler.

  Assumptions:
    - This function will only be called after all scheduling is complete (all jobs that have arrived will have finished and no new jobs will arrive).
  @return the average response time of all jobs scheduled.
 */
float scheduler_average_response_time()
{
	//return scheduler_average_turnaround_time();
	return 0.0;
}


/**
  Free any memory associated with your scheduler.
 
  Assumptions:
    - This function will be the last function called in your library.
*/
void scheduler_clean_up()
{
	//free data stored on queue
	priqueue_destroy(pending_tasks);
}


/**
  This function may print out any debugging information you choose. This
  function will be called by the simulator after every call the simulator
  makes to your scheduler.
  In our provided output, we have implemented this function to list the jobs in the order they are to be scheduled. Furthermore, we have also listed the current state of the job (either running on a given core or idle). For example, if we have a non-preemptive algorithm and job(id=4) has began running, job(id=2) arrives with a higher priority, and job(id=1) arrives with a lower priority, the output in our sample output will be:

    2(-1) 4(0) 1(-1)  
  
  This function is not required and will not be graded. You may leave it
  blank if you do not find it useful.
 */
void scheduler_show_queue()
{

}
