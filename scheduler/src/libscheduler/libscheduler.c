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
    int job_time; //last timestamp
    int run_time; //time left
    int priority; //how little user wants job to go first
    int original_process_time; //first time processed
    int response_time; //time that job last operated upon
    int last_checked_time; //last time that progress was recorded (for SJF)
} job_t;

//in fcfs, all new jobs come after all current jobs
int sort_by_fcfs_or_rr(const void *hopefully_job_a, const void *hopefully_job_b)
{
	//***convert the void pointers to jobs
		//
		//not needed since the outcome is preordained
	//***compare job properties
	//
	//printf("adding job %d to back of stack\n",((job_t*)hopefully_job_a)->job_number);
	return 1;
}
//compare job_time to see which will be finished first
int sort_by_shortest_job(const void *hopefully_job_a, const void *hopefully_job_b)
{
	//***convert the void pointers to jobs
	struct _job_t *job_a = (job_t*)hopefully_job_a; //convert pointer to a job struct
	struct _job_t *job_b = (job_t*)hopefully_job_b;
	//***compare job properties
	if(job_a->run_time < job_b->run_time) //shorter jobs run first
	{
		printf("new job is shorter\n");
		return -1;
	}
	if(job_a->run_time > job_b->run_time) //job b has shorter time
	{
		//printf("old job is shorter\n");
		return 1;
	}
	//reaching here, jobs tie for run time, use arrival time
        if(job_a->job_time < job_b->job_time) //job a came first
	{
		return -1;
	}
	//otw, job b has earlier arrival time
	return 1;

        //IOTW, return a's time - b's time, unless equal, then retunr a's timestamp - b's timestamp
}
//highest (smallest value) priority goes first
int sort_by_priority(const void *hopefully_job_a, const void *hopefully_job_b)
{
	//***convert the void pointers to jobs
	struct _job_t *job_a = (job_t*)hopefully_job_a; //convert pointer to a job struct
	struct _job_t *job_b = (job_t*)hopefully_job_b;
	//***compare job properties
	if(job_a->priority < job_b->priority) //if first job's priority is higher (lower value) than second job's priority
	{
		//printf("new job has higher priority\n"); 
		return -1; //first job goes first
	}
	if(job_a->priority > job_b->priority) //job b priority higher
	{
		//printf("old job has higher priority\n");
		return 1;
	}
	//reaching here, priority is the same, check timestamp
        if(job_a->job_time < job_b->job_time) //job_a shot first
	{
		return -1;
	}
	return 1; //otw 2nd job goes first
}

//added
scheme_t current_scheduling_mode; //monitors the current scheduling mode of the system
struct _priqueue_t pending_tasks; //tracks pending tasks to be scheduled next
struct _priqueue_t finished_tasks; //tracks finished tasks to schedule monitoring statistics

comparer preemption_comparer = 0; //if not 0, points to function for preemptive comparison
struct _job_t** current_core_jobs = 0; //tracks all currently active jobs on cores. If a job points to 0, that core is not doing anything
int current_num_cores = 0; //tracks size of current_core_jobs to prevent pointer-out-of-bounds errors
int quantum_size = 0; //tracks if round-robin quantum tracking is enabled (0 for disabled, >= 1 for enabled), and tracks size of quantum
int lowest_core_index = 0;
int initial_timestamp = 0; //the last time program updated, used for calculating wait time of programs

//statistic tracking variables
float total_waiting_time = 0.0;
float total_response_time = 0.0;
float total_turnaround_time = 0.0;
int total_num_jobs = 0;

//@pre: preemption is allowed, preemption-comparer is well defined
//checks each core to see if core is free or if core has lower priority (more preferable to be preempted than) than previous cores.
//then returns ID of core that has lowest priority
//Using this method, we can find the core that will be preempted by a preempting method, and only have to preempt on that core.
//@return: ID of core that should be preempted by a new task through preemption_comparer
int get_lowest_priority_core()
{
    int index_to_return = 0;
    int i = 0;
    //iterate through cores looking for lowest priority core
    if(current_core_jobs[0] == NULL) //handle case where 0th core has no job before we start iterating
	{
        //printf("0th core is idle, is lowest priority\n");
		return 0;
	}
    //reaching here, current_core_jobs[0] is defined, so we can skip it for iteration sake
    //initial case, index_to_return is 0
    for(i = 1; i < current_num_cores; i++) //if only 1 core, will not execute for loop since 1 !< 1, so return index_to_return (0)
    //for every job after first, compare it against the lowest priority task to find absolute lowest priority task
    {
        if(current_core_jobs[i] == NULL) //if no job scheduled for this core, it has lowest priority, should be allocated next
		{
			//printf("core %d is idle, is lowest priority\n",i);
			return i;
		}
        //reaching here, current_core_jobs[i] is defined
        if(preemption_comparer(current_core_jobs[i],current_core_jobs[index_to_return]) < 0) //if our picked core is higher priority than i'th core, pick i'th core
        {
			//printf("by comparison, core %d is lowest priority\n",i);
            index_to_return = i;
        }
    }
    //reaching here, core with lowest priority has ID index_to_return, return it
    return index_to_return;
}

//checks if any of the cores are idle (have job pointing to NULL
//if so, return id of core, else return -1
int get_idle_core_index()
{
    int i = 0; //used for loop iterations
    for(i = 0; i < current_num_cores; i++) //check that none of the cores are idle (if they're idle, immediately schedule this task)
    {
            //if core idle, make core's task this task, return core id
            if(current_core_jobs[i] == NULL) //if given core is doing nothing, make it do this instead
            {
                    //printf("core %d was idle, allocating it to job %d\n",i,new_job->job_number);
                    //since core was idle before, there is no job to pull off, so return id of core pushed-to
                    return i;
            }
    }
    //reaching here, all cores busy
    return -1;
}

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

        //initialize variables
        total_waiting_time = 0.0;
        total_response_time = 0.0;
        total_turnaround_time = 0.0;
        total_num_jobs = 0;
        initial_timestamp = 0; //initialize timestamp
	
	current_scheduling_mode = scheme; //all cores run with this scheme
	//initialize and configure pending_tasks based on which scheme to prioritize with
	switch(scheme)
	{
		case(FCFS): //first jobs in have highest priority
		{
			//printf("detected FCFS\n");
			priqueue_init(&pending_tasks,sort_by_fcfs_or_rr);
			break;
		}
		case(SJF): //shortest jobs have highest priority when fetching next job to do
		{
			//printf("detected SJF\n");
			priqueue_init(&pending_tasks,sort_by_shortest_job);
			break;
		}
		case(PSJF): //shortest jobs have highest priority and can preempt active jobs
		{
			//like shortest-job-first, but with preemption function defined, will try to preempt before adding to queue
			//printf("detected PSJF\n");
			priqueue_init(&pending_tasks,sort_by_shortest_job);
			preemption_comparer = sort_by_shortest_job; //defines preemption method so that preemption is handled correctly
			break;
		}
		case(PRI): //low priority jobs go first when fetching next job
		{
			//printf("detected PRI\n");
			priqueue_init(&pending_tasks,sort_by_priority);
		}
		case(PPRI): //low priority jobs can preempt active jobs
		{
			//printf("detected PPRI\n");
			priqueue_init(&pending_tasks,sort_by_priority);
			preemption_comparer = sort_by_priority;
		}
		case(RR): //first jobs have highest priority, enable quantum-based preemption
		{
			//printf("detected RR\n");
			priqueue_init(&pending_tasks,sort_by_fcfs_or_rr);
			//quantum_size = 1;//default quantum size is 1, simply to raise awareness of quantum timing
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
    //initial_timestamp = time; //we have a new record last time snapshot
    //printf("new job %d's time is %d, run time %d priority %d\n",job_number,time,running_time,priority);
    int return_core_id = -1; //becomes the core id of the core that takes in the new job, otherwise is -1
    //when new job arrives, decide what to do with it depending on which scheduler mode we are in
    struct _job_t *new_job; //creates a job to store on queue/push to core
    new_job = malloc(sizeof(new_job));
    new_job->job_number = job_number;
    new_job->job_time = time; //arrival time
    //new_job->wait_time = 0;
    new_job->run_time = running_time;
    new_job->priority = priority;
    new_job->original_process_time = running_time; //first time job processed by scheduler
    new_job->response_time = -1; //job has not been received yet

    int idle_core_index = get_idle_core_index();
    if(idle_core_index != -1) //handle idle core case
    {
        current_core_jobs[idle_core_index] = new_job; //idle core detected, allocate job to it

        new_job->response_time = 0; //job has been received immediately, so no response time

        if(current_scheduling_mode == PSJF) //if we are preemptively scheduling shortest job first
        {
            new_job->last_checked_time = time; //last checked for job status at current time
        }
        return idle_core_index;
    }
    /*generally:
     * find lowest on cores
     *  check for preemption
     * if preemption, check if was JUST scheduled
     *      if was, reset time
     *      return it
     */

    //reaching here, none of our cores are idle, check for preemption

    //if our preempting flag is true (points to a method), compare against active processes
    if(preemption_comparer != 0) //if we might preempt current jobs (checks that preemption comparer is defined)
    {
            //NOTE: tasks are preempted before they can run, so time differences will be 1 more than they should be (it doesn't run this phase)
            //EG: at time 0, task 0 (total time 8) runs successfully twice. last clock time = 0. Time = 3, task 0 gets preempted by task 1 (total time 2). 3(now) - 0(last) - 1 = 2 = number of executions run since
            //assume since only one task can be created at a time, tasks don't get preempted in the same timestep they arrive

        int lowest_priority_index = get_lowest_priority_core();
                    //properly update core job's remaining time to be fair with comparison
        job_t *temp_job = current_core_jobs[lowest_priority_index]; //grab the job to (possibly) push back onto queue
                    //since this job was last scheduled (job_time), it has been working on a core, so decrease remaining time (run_time) by the difference


        if(preemption_comparer(new_job,current_core_jobs[lowest_priority_index]) == -1) //if new job should preempt old job
        {
            if(temp_job->response_time == time) //if temp job arrived on core in THIS time block
            {
                temp_job->response_time = -1; //undo its receive-time time-stamping
            }
            current_core_jobs[lowest_priority_index] = new_job; //swap new job with temp job
            new_job = temp_job;
            return_core_id = lowest_priority_index; //we just preempted job at core # low_priority_index
                            //printf("preempting core %d with new job %d\n",i,new_job->job_number);
            //reaching here, new_job now points to the job we preempted from the core

                            //since preempting task just preempted, mark its receiving time now
            current_core_jobs[lowest_priority_index]->response_time = 0; //preempting task is being handled by cores
        }
    }
    //reaching here, our "new job" (either newly scheduled job, or the job removed by preemption) will be pushed onto pending tasks queue
    //push job to queue (which has already been configured to prioritize based on scheduler mode)
	
    priqueue_offer(&pending_tasks,(void *)new_job); //push the job onto the queue (queue handles prioritizing it)
	
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
    //update tallying info
    total_waiting_time += time - current_core_jobs[core_id]->job_time - current_core_jobs[core_id]->original_process_time;
    total_response_time += current_core_jobs[core_id]->response_time;
    total_turnaround_time += time - current_core_jobs[core_id]->job_time;
    total_num_jobs++;

    //at this point, we don't need the job anymore, so free it
    free(current_core_jobs[core_id]);
    current_core_jobs[core_id] = NULL;


    if(priqueue_size(&pending_tasks) >= 1) //if there are still jobs to run,
    {
        struct _job_t *next_job; //a pointer to the next job (or NULL if no next job) that will run on current core
        next_job = NULL; //points to nullptr
        next_job = priqueue_remove_at(&pending_tasks,0); //grab first job from queue and run it
        //allocate this job to the core that just finished
        if(next_job->response_time == -1) //if job has not been received yet
        {
                next_job->response_time = time - next_job->job_time; //its earliest receive time is now, so its response time is the time since last queue entry that its been waiting until now
        }

        //push the next job onto the newly-freed core
        current_core_jobs[core_id] = next_job; //NOTE: if no next job, current core has NULL job, so it is doing nothing

        return next_job->job_number; //new job is now on core
    }


    //reaching here, there was no next job to schedule, so schedule nothing

    //initial_timestamp = time; //we have a new record last time snapshot

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
	//printf("quantum expired on core %d\n",core_id);
	struct _job_t *stopped_job;
	//grab job from core, push back onto pending tasks queue (since is round robin, will go to end of queue)
	stopped_job = current_core_jobs[core_id]; //grab job from core



	current_core_jobs[core_id] = NULL;
        if(stopped_job != NULL) //if core had a job, push it back into queue
        {
            //update the new job's remaining time for when it exits queue
            //stopped_job->run_time -= (time - stopped_job->job_time);
            //stopped_job->job_time = time;//since run time was changed, update job time so that next update, it doesn't decrease job time too much
            priqueue_offer(&pending_tasks,stopped_job); //mark job as finished, keep it tracked for usage statistics
        }
	


	

	struct _job_t *next_job; //a pointer for the next job (or zero if no next job) that will run on current core
	next_job = NULL; //points to nullptr
	if(priqueue_size(&pending_tasks) >= 1) //if there are still jobs to run,
	{
		next_job = priqueue_remove_at(&pending_tasks,0); //grab first job from queue and run it
		//allocate this job to the core that just finished
                if(next_job != NULL && next_job->response_time == -1) //if job has yet to be processed, mark that it took (now - job time) to respond to it
                    next_job->response_time = time - next_job->job_time;
	}
	//push the next job onto the newly-freed core
	current_core_jobs[core_id] = next_job; //NOTE: if no next job, current core has NULL job, so it is doing nothing
	//though the fact we added expired job back to queue means it CANNOT be empty
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
        return (total_waiting_time / (float)total_num_jobs);
}


/**
  Returns the average turnaround time of all jobs scheduled by your scheduler.

  Assumptions:
    - This function will only be called after all scheduling is complete (all jobs that have arrived will have finished and no new jobs will arrive).
  @return the average turnaround time of all jobs scheduled.
 */
float scheduler_average_turnaround_time()
{
        return total_turnaround_time / (float)total_num_jobs;
}


/**
  Returns the average response time of all jobs scheduled by your scheduler.

  Assumptions:
    - This function will only be called after all scheduling is complete (all jobs that have arrived will have finished and no new jobs will arrive).
  @return the average response time of all jobs scheduled.
 */
float scheduler_average_response_time()
{
        return total_response_time / (float)total_num_jobs;
}


/**
  Free any memory associated with your scheduler.
 
  Assumptions:
    - This function will be the last function called in your library.
*/
void scheduler_clean_up()
{
	priqueue_destroy(&pending_tasks);
        //priqueue_destroy(&finished_tasks);
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
	
        /*int i = 0;
	int f = priqueue_size(&pending_tasks);
	for(i = 0; i < f; i++)
	{
		//for each task in the queue, print its details
		struct _job_t *found_job = (job_t*)priqueue_at(&pending_tasks,i);
		//printf("#%d: ID %d time %d runtime %d priority %d, ",i,found_job->job_number,found_job->job_time,found_job->run_time, found_job->priority);
        }*/
}
