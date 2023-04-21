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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
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

#define SMALLEST (-1)
#define LARGEST 1
#define EQUAL 0

struct process* find_process(struct list_head* head, int condition, int (*comparator)(struct process*))
{
	struct process* result = NULL;
	if (!list_empty(head)) {
		result = list_first_entry(head, struct process, list);
		struct process* pos = NULL;
		list_for_each_entry(pos, head, list)
		{
			if (condition < 0) {
				if (comparator(pos) < comparator(result))
					result = pos;
			} else if (condition > 0) {
				if (comparator(pos) > comparator(result))
					result = pos;
			} else {
				if (comparator(pos) == comparator(result))
					result = pos;
			}
		}
	}
	return result;
}

int ticks_left(struct process* p) { return p->lifespan - p->age; }

/***********************************************************************
 * Default FCFS resource acquisition function
 *
 * DESCRIPTION
 *   This is the default resource acquisition function which is called back
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
	.schedule = fifo_schedule,
};

/***********************************************************************
 * SJF scheduler
 ***********************************************************************/
int lifespan(struct process* p) { return p->lifespan; }

static struct process* sjf_schedule(void)
{
	if (current && ticks_left(current) > 0)
		return current;
	else {
		struct process* sj = find_process(&readyqueue, SMALLEST, lifespan);
		if (sj == NULL)
			return NULL;
		list_del_init(&sj->list);
		return sj;
	}
}

struct scheduler sjf_scheduler = {
	.name = "Shortest-Job First",
	.acquire = fcfs_acquire,
	.release = fcfs_release,
	.schedule = sjf_schedule,
};

/***********************************************************************
 * STCF scheduler
 ***********************************************************************/
static struct process* stcf_schedule(void)
{
	// if no job is ready in queue, return current process
	if (list_empty(&readyqueue)) {
		return ticks_left(current) > 0 ? current : NULL;
	}

	// Find STC Process
	struct process* stc = find_process(&readyqueue, SMALLEST, ticks_left);

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
	.acquire = fcfs_acquire,
	.release = fcfs_release,
	.schedule = stcf_schedule,
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
	.acquire = fcfs_acquire,
	.release = fcfs_release,
	.schedule = rr_schedule,
};

/***********************************************************************
 * Priority scheduler
 ***********************************************************************/

int prio(struct process* p) { return p->prio; }

static void prio_release(int resource_id)
{
	struct resource* r = resources + resource_id;
	assert(r->owner == current);
	r->owner = NULL;

	if (list_empty(&r->waitqueue))
		return;

	/* Find the highest priority process in the wait queue */
	struct process* waiter = find_process(&r->waitqueue, LARGEST, prio);

	assert(waiter->status == PROCESS_BLOCKED);

	list_del_init(&waiter->list);
	waiter->status = PROCESS_READY;
	list_add_tail(&waiter->list, &readyqueue);
}

static struct process* prio_schedule(void)
{
	// List is Empty
	if (list_empty(&readyqueue)) {
		assert(list_empty(&current->list));
		return ticks_left(current) > 0 ? current : NULL;
	}

	// List is not empty
	// Find the highest priority process
	struct process* highest_prio = find_process(&readyqueue, LARGEST, prio);
	// highest_prio is not null
	assert(highest_prio != NULL);
	// status in readyqueue must be PROCESS_READY
	assert(highest_prio->status == PROCESS_READY);

	if (!current || current->status == PROCESS_BLOCKED) {
		list_del_init(&highest_prio->list);
		return highest_prio;
	}

	assert(list_empty(&current->list));

	// Compare the priority
	if (highest_prio->prio >= current->prio) {
		// Preempt
		// If ticks_left(current) > 0 is not checked, process will be added to readyqueue
		// even though process is completed which results assertion on exit
		if (ticks_left(current) > 0)
			list_add_tail(&current->list, &readyqueue);
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
	.acquire = fcfs_acquire,
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
		if (p != except)
			p->prio++;
	}
}

static struct process* pa_schedule(void)
{
	struct process* next = prio_schedule();
	if (next != NULL) {
		prio_aging_except(next);
		next->prio = next->prio_orig;
	}

	return next;
}

struct scheduler pa_scheduler = {
	.name = "Priority + aging",
	.acquire = fcfs_acquire,
	.release = prio_release,
	.schedule = pa_schedule,
};

static void prio_init_release(int resource_id)
{
	struct resource* r = resources + resource_id;

	r->owner->prio = r->owner->prio_orig;

	prio_release(resource_id);
}

/***********************************************************************
 * Priority scheduler with priority ceiling protocol
 ***********************************************************************/

static bool pcp_acquire(int resource_id)
{
	struct resource* r = resources + resource_id;

	if (!r->owner)
		current->prio = MAX_PRIO;
	else
		r->owner->prio = MAX_PRIO;

	return fcfs_acquire(resource_id);
}

struct scheduler pcp_scheduler = {
	.name = "Priority + PCP Protocol",
	.acquire = pcp_acquire,
	.release = prio_init_release,
	.schedule = prio_schedule,
};

/***********************************************************************
 * Priority scheduler with priority inheritance protocol
 ***********************************************************************/

static bool pip_acquire(int resource_id)
{
	struct resource* r = resources + resource_id;

	if (r->owner && r->owner->prio < current->prio) {
		r->owner->prio = current->prio;
	}

	return fcfs_acquire(resource_id);
}

struct scheduler pip_scheduler = {
	.name = "Priority + PIP Protocol",
	.acquire = pip_acquire,
	.release = prio_init_release,
	.schedule = prio_schedule,
};
