/** @file libscheduler.c
 */

 /*TODO: Wait time is calculated using an integer timestamp value.
	Whenever a new job is added OR a job is finished, the timestamp increases to match the time of arrival of the new job. (new job's time property)
	Whenever a job is scheduled, add the difference between job's job_time and current timestamp to wait_time.
	Thus, a job doesn't know how long its waited until it is scheduled, but when it does get scheduled, it knows exactly how many time units its waited.
 */
 
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "libscheduler.h"
#include "../libpriqueue/libpriqueue.h"

typedef int(*comparer)(const void *, const void *); //defines method type for job comparison function, same defintion as used by PriQueue

/**
  Stores information making up a job to be scheduled including any statistics.

  You may need to define some global variables or a struct to store your job queue elements. 
*/
typedef struct _job_t
{
	int job_number; //stores job identifier
	int job_time; //time that job last entered queue
	int run_time; //time left before finished running
	int wait_time; //time spent waiting in pending_tasks queue
	int priority; //how little user wants job to go first
} job_t;

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
	struct _job_t *job_a = (job_t*)hopefully_job_a; //convert pointer to a job struct
	struct _job_t *job_b = (job_t*)hopefully_job_b;
	//***compare job properties
	if(job_a->run_time > job_b->run_time)
		return -1;
	return 1;
}
int sort_by_priority(const void *hopefully_job_a, const void *hopefully_job_b)
{
	//***convert the void pointers to jobs
	struct _job_t *job_a = (job_t*)hopefully_job_a; //convert pointer to a job struct
	struct _job_t *job_b = (job_t*)hopefully_job_b;
	//***compare job properties
	if(job_a->priority > job_b->priority)
		return -1;
	return 1;
}

//added
scheme_t current_scheduling_mode; //monitors the current scheduling mode of the system
struct _priqueue_t pending_tasks; //tracks pending tasks to be scheduled next
struct _priqueue_t finished_tasks; //tracks finished tasks to schedule monitoring statistics
//int is_preempting = 0; //(boolean) tracks whether currently running tasks can be kicked off of their cores by preemption
comparer preemption_comparer = 0; //if not 0, points to function for preemptive comparison
struct _job_t** current_core_jobs = 0; //tracks all currently active jobs on cores. If a job points to 0, that core is not doing anything
int current_num_cores = 0; //tracks size of current_core_jobs to prevent pointer-out-of-bounds errors
int quantum_size = 0; //tracks if round-robin quantum tracking is enabled (0 for disabled, >= 1 for enabled), and tracks size of quantum

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
	//initialize core tracking
	current_core_jobs = (job_t**)malloc(cores * sizeof(job_t*)); //allocate space for (cores) active jobs (not defined yet)
	int i = 0;
	current_num_cores = cores;
	for(i = 0; i < cores; i++)
	{
		current_core_jobs[i] = NULL; //make every core point to nullptr (no active job)
	}
	
	current_scheduling_mode = scheme; //all cores run with this scheme
	//initialize and configure pending_tasks based on which scheme to prioritize with
	switch(scheme)
	{
		case(FCFS): //first jobs in have highest priority
		{
			priqueue_init(&pending_tasks,sort_by_fcfs_or_rr);
			break;
		}
		case(SJF): //shortest jobs have highest priority when fetching next job to do
		{
			priqueue_init(&pending_tasks,sort_by_shortest_job);
			break;
		}
		case(PSJF): //shortest jobs have highest priority and can preempt active jobs
		{
			//like shortest-job-first, but with preemption function defined, will try to preempt before adding to queue
			priqueue_init(&pending_tasks,sort_by_shortest_job);
			preemption_comparer = sort_by_shortest_job; //defines preemption method so that preemption is handled correctly
			break;
		}
		case(PRI): //low priority jobs go first when fetching next job
		{
			priqueue_init(&pending_tasks,sort_by_priority);
		}
		case(PPRI): //low priority jobs can preempt active jobs
		{
			priqueue_init(&pending_tasks,sort_by_priority);
			preemption_comparer = sort_by_priority;
		}
		case(RR): //first jobs have highest priority, enable quantum-based preemption
		{
			priqueue_init(&pending_tasks,sort_by_fcfs_or_rr);
			quantum_size = 1;//default quantum size is 1, simply to raise awareness of quantum timing
			//TODO: configure flag to notify of quantum-based prioritization
		}
	}
	priqueue_init(&finished_tasks,sort_by_fcfs_or_rr); //we don't care how finished jobs are sorted, we just want them stored somewhere
	
	//end state: 
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
	int return_core_id = -1; //becomes the core id of the core that was preempted by new job, otherwise is -1
	//when new job arrives, decide what to do with it depending on which scheduler mode we are in
	struct _job_t *new_job; //creates a job to store on queue/push to core
	new_job = malloc(sizeof(new_job));
	new_job->job_number = job_number;
	new_job->job_time = time;
	new_job->wait_time = 0;
	new_job->run_time = running_time;
	new_job->priority = priority;
	
	int i = 0; //used for loop iterations
	for(i = 0; i < current_num_cores; i++) //check that none of the cores are idle (if they're idle, immediately schedule this task)
	{
		//if core idle, make core's task this task, return core id
		if(current_core_jobs[i] == NULL) //if given core is doing nothing, make it do this instead
		{
			current_core_jobs[i] = new_job;
			//since core was idle before, there is no job to pull off, so return id of core pushed-to
			return i;
		}
	}
	
	//if our preempting flag is true, compare against active processes
	if(preemption_comparer != 0) //if we might preempt current jobs (checks that preemption comparer is defined)
	{
		for(i = 0; i < current_num_cores; i++) //for each core, check its task against the current task to see if it must be preempted. NOTE: when new job preempts, restarts for loop with new job
		{
			if(preemption_comparer(new_job,current_core_jobs[i]) == -1) //if new job should preempt existing job, swap them and continue (its possible the swapped job can preempt on another core)
			{
				job_t *temp_job = current_core_jobs[i];
				current_core_jobs[i] = new_job;
				new_job = temp_job;
				if(return_core_id == -1) //only use core id of first preempted core
					return_core_id = i;
				//DECISION: should we terminate after first task swap, or do we reorganize all core tasks to reach highest prioritization?
				break;
				//i = 0; //restart for loop with 0th core to ensure cores are running the most optimal configuration of tasks
				//NOTE: assumes no circular prioritization (IE A < B < C < A), otherwise will be in infinite loop
				//because of this assumption, for some preempted core j, we can assume new job will not be preempted back off of core j or preempted onto another core
			}
		}
		//***check against current jobs, preempt if need be, otherwise continue
	}
	//reaching here, our "new job" (either newly scheduled job, or the job removed by preemption) will be pushed onto pending tasks queue
	//push job to queue (which has already been configured to prioritize based on scheduler mode)
	
	priqueue_offer(&pending_tasks,(void *)new_job); //push the job onto the queue (it handles prioritizing it)
	
	//if a core gets preempted by new job, return the id of core that gets preempted
	
	return return_core_id; //if no preemption or not preempted, return -1. If preempted, return id of core that was first preempted by new task
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
	finished_job = current_core_jobs[core_id];
	current_core_jobs[core_id] = NULL;
	//***retrieve finished job from core - to push onto finished queue for later
	priqueue_offer(&finished_tasks,finished_job); //mark job as finished, keep it tracked for usage statistics
	struct _job_t *next_job; //a pointer to the next job (or NULL if no next job) that will run on current core
	next_job = NULL; //points to nullptr
	if(priqueue_size(&pending_tasks) >= 1) //if there are still jobs to run,
	{
		next_job = priqueue_remove_at(&pending_tasks,0); //grab first job from queue and run it
		//allocate this job to the core that just finished
	}
	//push the next job onto the newly-freed core
	current_core_jobs[core_id] = next_job; //NOTE: if no next job, current core has NULL job, so it is doing nothing
	if(next_job != NULL) //if we successfully added a new job
		return next_job->job_number;
	//reaching here, there was no next job to schedule, so schedule nothing
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
	struct _job_t *stopped_job;
	//grab job from core, push back onto pending tasks queue (since is round robin, will go to end of queue)
	stopped_job = current_core_jobs[core_id]; //grab job from core
	current_core_jobs[core_id] = NULL;
	
	priqueue_offer(&pending_tasks,stopped_job); //mark job as finished, keep it tracked for usage statistics
	struct _job_t *next_job; //a pointer the next job (or zero if no next job) that will run on current core
	next_job = NULL; //points to nullptr
	if(priqueue_size(&pending_tasks) >= 1) //if there are still jobs to run,
	{
		next_job = priqueue_remove_at(&pending_tasks,0); //grab first job from queue and run it
		//allocate this job to the core that just finished
	}
	//push the next job onto the newly-freed core
	current_core_jobs[core_id] = next_job; //NOTE: if no next job, current core has NULL job, so it is doing nothing
	if(next_job != NULL) //if we successfully added a new job
		return next_job->job_number;
	//reaching here, there was no next job to schedule, so schedule nothing
	return -1;
}


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
	int num_tasks = priqueue_size(&finished_tasks);
	for(i = 0; i < num_tasks; i++) //get each finished task's total wait time
	{
		struct _job_t *finished_job = (job_t*)priqueue_at(&finished_tasks,i);
		running_total_wait_time += finished_job->wait_time;
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
	int num_tasks = priqueue_size(&finished_tasks);
	for(i = 0; i < num_tasks; i++) //get each finished task's total wait time
	{
		struct _job_t *job_reference = (job_t*)priqueue_at(&finished_tasks,i);
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
	return 0.0;
}


/**
  Free any memory associated with your scheduler.
 
  Assumptions:
    - This function will be the last function called in your library.
*/
void scheduler_clean_up()
{
	priqueue_destroy(&pending_tasks);
	priqueue_destroy(&finished_tasks);
	for(int i = 0; i < current_num_cores; i++)
	{
		free((void *)current_core_jobs[i]);
	}
	free((void *)current_core_jobs);
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
