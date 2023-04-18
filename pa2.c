/**********************************************************************
 * Copyright (c) 2019-2023
 *  Sang-Hoon Kim <sanghoonkim@ajou.ac.kr>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTIABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 **********************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include "types.h"
#include "list_head.h"

/**
 * The process which is currently running
 */
#include "process.h"
extern struct process* current;

/**
 * List head to hold the processes ready to run
 */
extern struct list_head readyqueue;

/**
 * Resources in the system.
 */
#include "resource.h"
extern struct resource resources[NR_RESOURCES];

/**
 * Monotonically increasing ticks. Do not modify it
 */
extern unsigned int ticks;

/**
 * Quiet mode. True if the program was started with -q option
 */
extern bool quiet;

int ticks_left(struct process* p) { return p->lifespan - p->age; }

/***********************************************************************
 * Default FCFS resource acquision function
 *
 * DESCRIPTION
 *   This is the default resource acquision function which is called back
 *   whenever the current process is to acquire resource @resource_id.
 *   The current implementation serves the resource in the requesting order
 *   without considering the priority. See the comments in sched.h
 ***********************************************************************/
static bool fcfs_acquire(int resource_id)
{
	struct resource* r = resources + resource_id;

	if (!r->owner) {
		/* This resource is not owned by any one. Take it! */
		r->owner = current;
		return true;
	}

	/* OK, this resource is taken by @r->owner. */

	/* Update the current process state */
	current->status = PROCESS_BLOCKED;

	/* And append current to waitqueue */
	list_add_tail(&current->list, &r->waitqueue);

	/**
	 * And return false to indicate the resource is not available.
	 * The scheduler framework will soon call schedule() function to
	 * schedule out current and to pick the next process to run.
	 */
	return false;
}

/***********************************************************************
 * Default FCFS resource release function
 *
 * DESCRIPTION
 *   This is the default resource release function which is called back
 *   whenever the current process is to release resource @resource_id.
 *   The current implementation serves the resource in the requesting order
 *   without considering the priority. See the comments in sched.h
 ***********************************************************************/
static void fcfs_release(int resource_id)
{
	struct resource* r = resources + resource_id;

	/* Ensure that the owner process is releasing the resource */
	assert(r->owner == current);

	/* Un-own this resource */
	r->owner = NULL;

	/* Let's wake up ONE waiter (if exists) that came first */
	if (!list_empty(&r->waitqueue)) {
		struct process* waiter = list_first_entry(&r->waitqueue, struct process, list);

		/**
		 * Ensure the waiter is in the wait status
		 */
		assert(waiter->status == PROCESS_BLOCKED);

		/**
		 * Take out the waiter from the waiting queue. Note we use
		 * list_del_init() over list_del() to maintain the list head tidy
		 * (otherwise, the framework will complain on the list head
		 * when the process exits).
		 */
		list_del_init(&waiter->list);

		/* Update the process status */
		waiter->status = PROCESS_READY;

		/**
		 * Put the waiter process into ready queue. The framework will
		 * do the rest.
		 */
		list_add_tail(&waiter->list, &readyqueue);
	}
}

#include "sched.h"

/***********************************************************************
 * FIFO scheduler
 ***********************************************************************/
static int fifo_initialize(void) { return 0; }

static void fifo_finalize(void) { }

static struct process* fifo_schedule(void)
{
	struct process* next = NULL;

	/* You may inspect the situation by calling dump_status() at any time */
	// dump_status();

	/**
	 * When there was no process to run in the previous tick (so does
	 * in the very beginning of the simulation), there will be
	 * no @current process. In this case, pick the next without examining
	 * the current process. Also, the current process can be blocked
	 * while acquiring a resource. In this case just pick the next as well.
	 */
	if (!current || current->status == PROCESS_BLOCKED) {
		goto pick_next;
	}

	/* The current process has remaining lifetime. Schedule it again */
	if (current->age < current->lifespan) {
		return current;
	}

pick_next:
	/* Let's pick a new process to run next */

	if (!list_empty(&readyqueue)) {
		/**
		 * If the ready queue is not empty, pick the first process
		 * in the ready queue
		 */
		next = list_first_entry(&readyqueue, struct process, list);

		/**
		 * Detach the process from the ready queue. Note that we use
		 * list_del_init() over list_del() to maintain the list head tidy.
		 * Otherwise, the framework will complain (assert) on process exit.
		 */
		list_del_init(&next->list);
	}

	/* Return the process to run next */
	return next;
}

struct scheduler fifo_scheduler = {
	.name = "FIFO",
	.acquire = fcfs_acquire,
	.release = fcfs_release,
	.initialize = fifo_initialize,
	.finalize = fifo_finalize,
	.schedule = fifo_schedule,
};

/***********************************************************************
 * SJF scheduler
 ***********************************************************************/
static struct process* sjf_schedule(void)
{
	struct process* next = NULL;
	if (current && ticks_left(current) > 0)
		return current;
	else {
		if (!list_empty(&readyqueue)) {
			// Find Shortest Job from Queue
			next = list_first_entry(&readyqueue, struct process, list);
			struct process* p = NULL;
			list_for_each_entry(p, &readyqueue, list)
			{
				if (p->lifespan < next->lifespan)
					next = p;
			}
		}
		if (next == NULL)
			return NULL;
		list_del_init(&next->list);
		return next;
	}
}

struct scheduler sjf_scheduler = {
	.name = "Shortest-Job First",
	.acquire = fcfs_acquire, /* Use the default FCFS acquire() */
	.release = fcfs_release, /* Use the default FCFS release() */
	.schedule = sjf_schedule, /* TODO: Assign your schedule function
							   to this function pointer to activate
							   SJF in the simulation system */
};

/***********************************************************************
 * STCF scheduler
 ***********************************************************************/
static struct process* stcf_schedule(void)
{
	// if no job is ready in queue, return current process
	if (list_empty(&readyqueue)) {
		if (ticks_left(current) > 0)
			return current;
		else
			return NULL;
	}

	// Find STC Process
	struct process* stc = list_first_entry_or_null(&readyqueue, struct process, list);
	struct process* p = NULL;
	list_for_each_entry(p, &readyqueue, list)
	{
		if (!stc || ticks_left(p) < ticks_left(stc))
			stc = p;
	}

	if (!current || current->status == PROCESS_BLOCKED) {
		list_del_init(&stc->list);
		return stc;
	}

	if (ticks_left(stc) < ticks_left(current)) {
		list_add(&current->list, &readyqueue);
		list_del_init(&stc->list);
		return stc;
	} else {
		if (ticks_left(current) > 0)
			return current;
		else {
			list_del_init(&stc->list);
			return stc;
		}
	}
}

struct scheduler stcf_scheduler = {
	.name = "Shortest Time-to-Complete First",
	.acquire = fcfs_acquire, /* Use the default FCFS acquire() */
	.release = fcfs_release, /* Use the default FCFS release() */
	.schedule = stcf_schedule,

	/* You need to check the newly created processes to implement STCF.
	 * Have a look at @forked() callback.
	 */

	/* Obviously, you should implement stcf_schedule() and attach it here */
};

/***********************************************************************
 * Round-robin scheduler
 ***********************************************************************/
static struct process* rr_schedule(void)
{
	struct process* next = list_empty(&readyqueue) ? NULL : list_first_entry_or_null(&readyqueue, struct process, list);
	bool current_uncompleted = current && current->status != PROCESS_BLOCKED && ticks_left(current) > 0;

	if (next == NULL)
		return current_uncompleted ? current : NULL;

	if (current_uncompleted)
		list_add_tail(&current->list, &readyqueue);

	list_del_init(&next->list);
	return next;
}

struct scheduler rr_scheduler = {
	.name = "Round-Robin",
	.acquire = fcfs_acquire, /* Use the default FCFS acquire() */
	.release = fcfs_release, /* Use the default FCFS release() */
	.schedule = rr_schedule,

	/* Obviously, ... */
};

/***********************************************************************
 * Priority scheduler
 ***********************************************************************/
static bool prio_acquire(int resource_id)
{
	struct resource* r = resources + resource_id;

	/* If the resource is not owned by anyone, just acquire it */
	if (!r->owner) {
		r->owner = current;
		return true;
	}

	/* If the resource is owned by someone, check the priority */
	if (r->owner->prio < current->prio) {
		/* Priority: owner < current => preempt */
		r->owner->status = PROCESS_BLOCKED;
		list_add_tail(&r->owner->list, &r->waitqueue);
		r->owner = current;
		return true;
	} else {
		/* Priority: owner >= current => wait */
		current->status = PROCESS_BLOCKED;
		list_add_tail(&current->list, &r->waitqueue);
		return false;
	}
}

static void prio_release(int resource_id)
{
	struct resource* r = resources + resource_id;

	assert(r->owner == current);
	r->owner = NULL;

	if (list_empty(&r->waitqueue))
		return;

	/* Find the highest priority process in the wait queue */
	struct process* waiter = list_first_entry(&r->waitqueue, struct process, list);
	struct process* p = NULL;
	list_for_each_entry(p, &r->waitqueue, list)
	{
		if (p->prio > waiter->prio)
			waiter = p;
	}

  assert(waiter->status == PROCESS_BLOCKED);

	list_del_init(&waiter->list);
	waiter->status = PROCESS_READY;
	list_add_tail(&waiter->list, &readyqueue);
}

static struct process* prio_schedule(void)
{
	/**
	 * Implement your own schedule function to make the priority scheduler
	 * correct.
	 */
  // List is Empty
  if (list_empty(&readyqueue)) {
    if (ticks_left(current) > 0)
      return current;
    else
      return NULL;
  }

  // List is not empty
  // Find the highest priority process
  struct process *next = list_first_entry(&readyqueue, struct process, list);
  struct process *p = NULL;
  list_for_each_entry(p, &readyqueue, list) {
    if (!next || p->prio > next->prio) {
      next = p;
    }
  }
  if (!current || current->status == PROCESS_BLOCKED) {
    list_del_init(&next->list);
    return next;
  }
  // Compare the priority
  if (next->prio > current->prio) {
    // Preempt
    list_add_tail(&current->list, &readyqueue);
    list_del_init(&next->list);
    return next;
  } 
  else {
    // Wait
    if (ticks_left(current) > 0)
      return current;
    else {
      list_del_init(&next->list);
      return next;
    }
  }
}

struct scheduler prio_scheduler = {
	.name = "Priority", .acquire = prio_acquire, .release = prio_release, .schedule = prio_schedule,
	/**
	 * Implement your own acqure/release function to make the priority
	 * scheduler correct.
	 */
	/* Implement your own prio_schedule() and attach it here */
};

/***********************************************************************
 * Priority scheduler with aging
 ***********************************************************************/
struct scheduler pa_scheduler = {
	.name = "Priority + aging",
	/**
	 * Ditto
	 */
};

/***********************************************************************
 * Priority scheduler with priority ceiling protocol
 ***********************************************************************/
struct scheduler pcp_scheduler = {
	.name = "Priority + PCP Protocol",
	/**
	 * Ditto
	 */
};

/***********************************************************************
 * Priority scheduler with priority inheritance protocol
 ***********************************************************************/
struct scheduler pip_scheduler = {
	.name = "Priority + PIP Protocol",
	/**
	 * Ditto
	 */
};
