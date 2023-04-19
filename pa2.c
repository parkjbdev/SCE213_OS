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

struct process* rqueue(int idx)
{
	struct list_head* pos = (&readyqueue)->next;

	for (int i = 0; i < idx; i++) {
		pos = pos->next;
	}
	return (struct process*)((char*)pos - offsetof(struct process, list));
}

void list_add_safe(struct list_head* new, struct list_head* head)
{
	assert(list_empty(new));
	list_add(new, head);
}

void list_add_tail_safe(struct list_head* new, struct list_head* head)
{
	assert(list_empty(new));
	list_add_tail(new, head);
}

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

	assert(list_empty(&r->owner->list));

	// Find Highest Priority Process
	struct process* highest_prio = r->owner;
	struct process* pos = NULL;
	list_for_each_entry(pos, &r->waitqueue, list)
	{
		if (pos->prio > highest_prio->prio)
			highest_prio = pos;
	}

	if (current->prio > highest_prio->prio) {
		if (highest_prio == r->owner) {
			r->owner->status = PROCESS_BLOCKED;
			list_add_tail_safe(&r->owner->list, &r->waitqueue);
		}
		// Just for safety
		assert(list_empty(&current->list));
		r->owner = current;
		return true;
	} else {
		current->status = PROCESS_BLOCKED;
		list_add_tail_safe(&current->list, &r->waitqueue);
		// Just for safety
		assert(list_empty(&highest_prio->list));
		r->owner = highest_prio;

		return false;
	}
}

static void prio_release(int resource_id)
{
	struct resource* r = resources + resource_id;
	// Could be NULL if the process is preempted before, therefore r->owner could already be NULL
	//	assert(r->owner == current);
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
	list_add_safe(&waiter->list, &readyqueue);
}

static struct process* prio_schedule(void)
{
	/**
	 * Implement your own schedule function to make the priority scheduler
	 * correct.
	 */
	// List is Empty
	if (list_empty(&readyqueue)) {
		assert(list_empty(&current->list));
		return ticks_left(current) > 0 ? current : NULL;
	}

	// List is not empty
	// Find the highest priority process
	struct process* highest_prio = list_first_entry(&readyqueue, struct process, list);
	struct process* p = NULL;
	list_for_each_entry(p, &readyqueue, list)
	{
		if (p->prio > highest_prio->prio) {
			highest_prio = p;
		}
	}

	if (!current || current->status == PROCESS_BLOCKED) {
		list_del_init(&highest_prio->list);
		return highest_prio;
	}

	assert(list_empty(&current->list));

	// Compare the priority
	if (highest_prio->prio >= current->prio) {
		// Preempt
		// Force Release Resource if current is preempted
		for (int i = 0; i < NR_RESOURCES; i++) {
			struct resource* r = resources + i;
			if (r->owner == current)
				r->owner = NULL;
		}
		// Bug Fix
		// If ticks_left(current) > 0 is not checked, process will be added to readyqueue even though process is
		// completed
		if (ticks_left(current) > 0)
			list_add_tail_safe(&current->list, &readyqueue);
		list_del_init(&highest_prio->list);
		return highest_prio;
	} else {
		if (ticks_left(current) > 0)
			return current;
		else {
			list_del_init(&highest_prio->list);
			return highest_prio;
		}
	}
}

struct scheduler prio_scheduler = {
	.name = "Priority",
	.acquire = prio_acquire,
	.release = prio_release,
	.schedule = prio_schedule,
};

/***********************************************************************
 * Priority scheduler with aging
 ***********************************************************************/

static void prio_aging_except(struct process* except)
{
	struct process* p = NULL;
	list_for_each_entry(p, &readyqueue, list)
	{
		if (p != except) {
			p->prio++;
			if (!quiet)
				fprintf(stderr, "Aging priority %d from %d to %d\n", p->pid, p->prio_orig, p->prio);
		}
	}
}

static struct process* pa_schedule(void)
{
	/**
	 * Implement your own schedule function to make the priority scheduler
	 * correct.
	 */
	// List is Empty
	if (list_empty(&readyqueue)) {
		assert(list_empty(&current->list));
		if (ticks_left(current) > 0) {
			prio_aging_except(current);
			return current;
		} else
			return NULL;
	}

	// List is not empty
	// Find the highest priority process
	struct process* highest_prio = list_first_entry(&readyqueue, struct process, list);
	struct process* p = NULL;
	list_for_each_entry(p, &readyqueue, list)
	{
		if (p->prio > highest_prio->prio) {
			highest_prio = p;
		}
	}

	if (!current || current->status == PROCESS_BLOCKED) {
		list_del_init(&highest_prio->list);
		prio_aging_except(current);
		return highest_prio;
	}

	assert(list_empty(&current->list));

	// Compare the priority
	if (highest_prio->prio >= current->prio) {
		// Preempt
		// Force Release Resource if current is preempted
		for (int i = 0; i < NR_RESOURCES; i++) {
			struct resource* r = resources + i;
			if (r->owner == current)
				r->owner = NULL;
		}
		// Bug Fix
		// If ticks_left(current) > 0 is not checked, process will be added to readyqueue even though process is
		// completed
		if (ticks_left(current) > 0)
			list_add_tail_safe(&current->list, &readyqueue);
		list_del_init(&highest_prio->list);
		prio_aging_except(current);
		return highest_prio;
	} else {
		if (ticks_left(current) > 0) {
			prio_aging_except(current);
			return current;
		} else {
			list_del_init(&highest_prio->list);
			prio_aging_except(current);
			return highest_prio;
		}
	}
}

struct scheduler pa_scheduler = {
	.name = "Priority + aging", .acquire = prio_acquire, .release = prio_release, .schedule = pa_schedule,
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
