/**********************************************************************
 * Copyright (c) 2020-2023
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
#include <memory.h>

#include "types.h"
#include "list_head.h"
#include "vm.h"

/**
 * Ready queue of the system
 */
extern struct list_head processes;

/**
 * Currently running process
 */
extern struct process* current;

/**
 * Page Table Base Register that MMU will walk through for address translation
 */
extern struct pagetable* ptbr;

/**
 * TLB of the system.
 */
extern struct tlb_entry tlb[1UL << (PTES_PER_PAGE_SHIFT * 2)];

/**
 * The number of mappings for each page frame. Can be used to determine how
 * many processes are using the page frames.
 */
extern unsigned int mapcounts[];

/**
 * lookup_tlb(@vpn, @rw, @pfn)
 *
 * DESCRIPTION
 *   Translate @vpn of the current process through TLB. DO NOT make your own
 *   data structure for TLB, but should use the defined @tlb data structure
 *   to translate. If the requested VPN exists in the TLB and it has the same
 *   rw flag, return true with @pfn is set to its PFN. Otherwise, return false.
 *   The framework calls this function when needed, so do not call
 *   this function manually.
 *
 * RETURN
 *   Return true if the translation is cached in the TLB.
 *   Return false otherwise
 */
bool lookup_tlb(unsigned int vpn, unsigned int rw, unsigned int* pfn) { return false; }

/**
 * insert_tlb(@vpn, @rw, @pfn)
 *
 * DESCRIPTION
 *   Insert the mapping from @vpn to @pfn for @rw into the TLB. The framework will
 *   call this function when required, so no need to call this function manually.
 *   Note that if there exists an entry for @vpn already, just update it accordingly
 *   rather than removing it or creating a new entry.
 *   Also, in the current simulator, TLB is big enough to cache all the entries of
 *   the current page table, so don't worry about TLB entry eviction. ;-)
 */
void insert_tlb(unsigned int vpn, unsigned int rw, unsigned int pfn) { }

/**
 * alloc_page(@vpn, @rw)
 *
 * DESCRIPTION
 *   Allocate a page frame that is not allocated to any process, and map it
 *   to @vpn. When the system has multiple free pages, this function should
 *   allocate the page frame with the **smallest pfn**.
 *   You may construct the page table of the @current process. When the page
 *   is allocated with ACCESS_WRITE flag, the page may be later accessed for writes.
 *   However, the pages populated with ACCESS_READ should not be accessible with
 *   ACCESS_WRITE accesses.
 *
 * RETURN
 *   Return allocated page frame number.
 *   Return -1 if all page frames are allocated.
 */

int alloc_pf()
{
	for (int i = 0; i < NR_PAGEFRAMES; i++) {
		if (mapcounts[i] == 0) {
			mapcounts[i]++;
			return i;
		}
	}
	return -1;
}

void free_pf(unsigned int pfn) { mapcounts[pfn]--; }

unsigned int alloc_page(unsigned int vpn, unsigned int rw)
{
	unsigned int pd_index = vpn / NR_PTES_PER_PAGE;
	unsigned int pte_index = vpn % NR_PTES_PER_PAGE;

	struct pte_directory** pd = &ptbr->outer_ptes[pd_index];
	if (*pd == NULL) {
		*pd = (struct pte_directory*)malloc(sizeof(struct pte_directory));
	}
	assert(pd != NULL);

	struct pte* pte = &(*pd)->ptes[pte_index];
	unsigned int pfn = alloc_pf();

	pte->pfn = pfn;
	pte->valid = true;
	pte->rw = rw;
	pte->private = rw;

	return pfn;
}

/**
 * free_page(@vpn)
 *
 * DESCRIPTION
 *   Deallocate the page from the current processor. Make sure that the fields
 *   for the corresponding PTE (valid, rw, pfn) is set @false or 0.
 *   Also, consider the case when a page is shared by two processes,
 *   and one process is about to free the page. Also, think about TLB as well ;-)
 */
void free_page(unsigned int vpn)
{
	unsigned int pd_index = vpn / NR_PTES_PER_PAGE;
	unsigned int pte_index = vpn % NR_PTES_PER_PAGE;

	struct pte_directory* pd = ptbr->outer_ptes[pd_index];
	assert(pd != NULL);
	struct pte* pte = &pd->ptes[pte_index];

	// make pte invalid only when it is not shared
	if (mapcounts[pte->pfn] == 1)
		pte->valid = false;

	free_pf(pte->pfn);

	// If every pte is not valid, free pd
	for (int i = 0; i < NR_PTES_PER_PAGE; i++) {
		if (pd->ptes[i].valid)
			return;
	}
	free(pd);
}

/**
 * handle_page_fault()
 *
 * DESCRIPTION
 *   Handle the page fault for accessing @vpn for @rw. This function is called
 *   by the framework when the __translate() for @vpn fails. This implies;
 *   0. page directory is invalid
 *   1. pte is invalid
 *   2. pte is not writable but @rw is for write
 *   This function should identify the situation, and do the copy-on-write if
 *   necessary.
 *
 * RETURN
 *   @true on successful fault handling
 *   @false otherwise
 */
bool handle_page_fault(unsigned int vpn, unsigned int rw)
{
	//	struct current_pagetable* ptbr = &current->pagetable;
	unsigned int pd_index = vpn / NR_PTES_PER_PAGE;
	unsigned int pte_index = vpn % NR_PTES_PER_PAGE;

	/** Case 0. @pd is invalid */
	struct pte_directory** pd = &ptbr->outer_ptes[pd_index];
	if (pd == NULL) {
		// Handle here
	}

	/** Case 1. @pte is invalid */
	struct pte* pte = &(*pd)->ptes[pte_index];
	if (pte == NULL) {
		// Handle here
	}

	/** Case 2. PTE is not writable, but @rw is for write */
	if (pte->rw == ACCESS_READ && rw == ACCESS_WRITE) {
		if (pte->private == ACCESS_READ + ACCESS_WRITE) {
			/** Implement Copy on write */
			if (mapcounts[pte->pfn] > 1) {
				mapcounts[pte->pfn]--;
				pte->pfn = alloc_pf();
				pte->valid = true;
			}
			pte->rw = pte->private;

			return true;
		} else
			return false;
	}

	return false;
}

/**
 * switch_process()
 *
 * DESCRIPTION
 *   If there is a process with @pid in @processes, switch to the process.
 *   The @current process at the moment should be put into the @processes
 *   list, and @current should be replaced to the requested process.
 *   Make sure that the next process is unlinked from the @processes, and
 *   @ptbr is set properly.
 *
 *   If there is no process with @pid in the @processes list, fork a process
 *   from the @current. This implies the forked child process should have
 *   the identical page table entry 'values' to its parent's (i.e., @current)
 *   page table.
 *   To implement the copy-on-write feature, you should manipulate the writable
 *   bit in PTE and mapcounts for shared pages. You may use pte->private for
 *   storing some useful information :-)
 */
struct pagetable* clone_pagetable(const struct pagetable* src)
{
	struct pagetable* pt = (struct pagetable*)malloc(sizeof(struct pagetable));
	assert(pt != NULL);

	for (int i = 0; i < NR_PTES_PER_PAGE; i++) {
		if (src->outer_ptes[i] != NULL) {
			pt->outer_ptes[i] = (struct pte_directory*)malloc(sizeof(struct pte_directory));
			assert(pt->outer_ptes[i] != NULL);
			memcpy(pt->outer_ptes[i], src->outer_ptes[i], sizeof(struct pte_directory));
		} else {
			pt->outer_ptes[i] = NULL;
		}
	}

	return pt;
}

struct process* find_process(unsigned int pid)
{
	struct process* pos = NULL;
	bool found = false;

	list_for_each_entry(pos, &processes, list)
	{
		if (pos->pid == pid) {
			found = true;
			break;
		}
	}

	return found ? pos : NULL;
}
void switch_process(unsigned int pid)
{
	/** Check if process with @pid exists in @processes */
	struct process* process = find_process(pid);

	/** put @current process to @processes */
	list_add_tail(&current->list, &processes);

	/** If process is not found in @processes, fork current process */
	if (process == NULL) {
		process = (struct process*)malloc(sizeof(struct process));
		process->pid = pid;
		process->list.prev = &process->list;
		process->list.next = &process->list;

		/** increase @mapcount and make shared memories only readable before copying @pagetable to new process */
		for (int i = 0; i < NR_PTES_PER_PAGE; i++) {
			struct pte_directory* pd = current->pagetable.outer_ptes[i];
			if (pd == NULL)
				continue;
			for (int j = 0; j < NR_PTES_PER_PAGE; j++) {
				if (pd->ptes[j].valid) {
					pd->ptes[j].rw = ACCESS_READ;
					mapcounts[pd->ptes[j].pfn]++;
				}
			}
		}
		/** deep copy @pagetable */
		process->pagetable = *clone_pagetable(&current->pagetable);
	} else {
		list_del(&process->list);
	}

	current = process;
	ptbr = &current->pagetable;
}
