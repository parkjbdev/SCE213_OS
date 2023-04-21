# Project #2: Simulating Processor Schedulers

[![wakatime](https://wakatime.com/badge/user/6f2f57ae-ce04-4c16-80e7-660166fb783d/project/b7573b33-44af-4347-a49c-57af64070dfb.svg)](https://wakatime.com/badge/user/6f2f57ae-ce04-4c16-80e7-660166fb783d/project/b7573b33-44af-4347-a49c-57af64070dfb)

## 0415

### Refactor: Simplify Logic of SJF Scheduler

Simplified Control Path

```c
		if (!current || current->status == PROCESS_BLOCKED) {
			list_del_init(&next->list);
			return next;
		} else {
			if (next == NULL)
				return NULL;
			list_del_init(&next->list);
			return next;
		}
```

```c
		if (next == NULL) return NULL;
		list_del_init(&next->list);
		return next;
```

### Bug Possibility

- Does STCF really needs `forked` callback?

### Implement rr_scheduler

Implemented Round Robin Scheduling

## 0419

### Implement Priority Scheduler

#### `prio_queue`

```c
  if (list_empty(&r->waitqueue)) {
      if (current->prio > r->owner->prio) {
          r->owner->status = PROCESS_BLOCKED;
          list_add_tail_safe(&r->owner->list, &r->waitqueue);
          return true;
      } else {
          current->status = PROCESS_BLOCKED;
          list_add_tail_safe(&current->list, &r->waitqueue);
          return false;
      }
  }
```

code above which should be at 348 line (after assertion) is integrated by initializing `highest_prio` as `r->owner`

### `rqueue`

- used for debugging
- similar to `list_entry` or `container_of`

### `list_add_safe` and `list_add_tail_safe`

- check if the list_head to add is empty

### Bug Possibility

#### `prio_release`

- doesn't release all resources when its preempted
- maybe `assert(r->owner, current)` should work even though in case of process preempted by higher priority process
  and acquired back again

### Testcase Status

- [X] testcases/single
- [X] testcases/multi
- [X] testcases/prio
- [X] testcases/resources-basic
- [X] testcases/resources-prio
- [X] testcases/resources-adv1
- [ ] testcases/resources-adv2

## TODO

- [X] SJF scheduler: 20pts (tested using `multi`)
- [X] STCF scheduler: 50pts (`multi`)
- [X] RR scheduler:  50pts (`multi` and `prio`)
- [ ] Priority scheduler: 60pts (`prio` and `resources-prio`)
- [ ] Priority scheduler + aging: 100pts (`prio`)
- [ ] Priority scheduler + PCP: 70pts (`resources-basic`)
- [ ] Priority scheduler + PIP: 150pts (`resources-adv1` and `resources-adv2`)

## 0419

### Priority Scheduler Policy Update

- Implemented Round Robin Scheduling when there are processes with same priority
- Bugfix & Passed all testcases

### BugFix

- Force release resources when process is preempted by higher priority process
- Check if `ticks_left() > 0` before returning from `prio_schedule()`

### Minor Changes

- list_add_safe and list_add_tail_safe are now force adding list_head to the list

### Testcase Status

- [X] testcases/single
- [X] testcases/multi
- [X] testcases/prio
- [X] testcases/resources-basic
- [X] testcases/resources-prio
- [X] testcases/resources-adv1
- [X] testcases/resources-adv2

## 0419

### Priority Aging Scheduler Implementation

- Defined `prio_aging_except` function
- [Link to Diffcheck Results between priority & pa](https://www.diffchecker.com/P226p6IR/)

### TODO

- [X] SJF scheduler: 20pts (tested using `multi`)
- [X] STCF scheduler: 50pts (`multi`)
- [X] RR scheduler:  50pts (`multi` and `prio`)
- [X] Priority scheduler: 60pts (`prio` and `resources-prio`)
- [X] Priority scheduler + aging: 100pts (`prio`)
- [ ] Priority scheduler + PCP: 70pts (`resources-basic`)
- [ ] Priority scheduler + PIP: 150pts (`resources-adv1` and `resources-adv2`)

# 0420

## Add Test Automation Scripts

## Refactor

- Implement `find_process` for compare and find process matching condition

# 0421

## Successfully Fixed Prio Scheduler

- `prio_acquire` was JUST SAME AS `fcfs_acquire`
- `prio_release` was same as `fcfs_release` except for `waiter` being the highest priority process

## Implement PA

- Aging priority of process when it hasn't been scheduled
- Initialize Age when it's scheduled

## Implement PCP

- Raise priority of process when it acquires resource

## Implement PIP

- Inherit priority of process when it acquires resource

## Conclusions for priority scheduler
- Quite easy to implement PA, PCP, PIP once Priority Scheduler is successfully implemented

## All Jobs Done

- [X] SJF scheduler: 20pts (tested using `multi`)
- [X] STCF scheduler: 50pts (`multi`)
- [X] RR scheduler:  50pts (`multi` and `prio`)
- [X] Priority scheduler: 60pts (`prio` and `resources-prio`)
- [X] Priority scheduler + aging: 100pts (`prio`)
- [X] Priority scheduler + PCP: 70pts (`resources-basic`)
- [X] Priority scheduler + PIP: 150pts (`resources-adv1` and `resources-adv2`)

## Every Test Exited with 0 Status as shown
```
FIFO Test
✅  passed: ./testcases/multi
✅  passed: ./testcases/prio
✅  passed: ./testcases/resources-adv1
✅  passed: ./testcases/resources-adv2
✅  passed: ./testcases/resources-basic
✅  passed: ./testcases/resources-prio
✅  passed: ./testcases/single
Priority + Aging Test
✅  passed: ./testcases/multi
✅  passed: ./testcases/prio
✅  passed: ./testcases/resources-adv1
✅  passed: ./testcases/resources-adv2
✅  passed: ./testcases/resources-basic
✅  passed: ./testcases/resources-prio
✅  passed: ./testcases/single
Priority + PCP Protocol Test
✅  passed: ./testcases/multi
✅  passed: ./testcases/prio
✅  passed: ./testcases/resources-adv1
✅  passed: ./testcases/resources-adv2
✅  passed: ./testcases/resources-basic
✅  passed: ./testcases/resources-prio
✅  passed: ./testcases/single
Priority + PIP Protocol Test
✅  passed: ./testcases/multi
✅  passed: ./testcases/prio
✅  passed: ./testcases/resources-adv1
✅  passed: ./testcases/resources-adv2
✅  passed: ./testcases/resources-basic
✅  passed: ./testcases/resources-prio
✅  passed: ./testcases/single
Priority Test
✅  passed: ./testcases/multi
✅  passed: ./testcases/prio
✅  passed: ./testcases/resources-adv1
✅  passed: ./testcases/resources-adv2
✅  passed: ./testcases/resources-basic
✅  passed: ./testcases/resources-prio
✅  passed: ./testcases/single
RR Test
✅  passed: ./testcases/multi
✅  passed: ./testcases/prio
✅  passed: ./testcases/resources-adv1
✅  passed: ./testcases/resources-adv2
✅  passed: ./testcases/resources-basic
✅  passed: ./testcases/resources-prio
✅  passed: ./testcases/single
SJF Test
✅  passed: ./testcases/multi
✅  passed: ./testcases/prio
✅  passed: ./testcases/resources-adv1
✅  passed: ./testcases/resources-adv2
✅  passed: ./testcases/resources-basic
✅  passed: ./testcases/resources-prio
✅  passed: ./testcases/single
STCF Test
✅  passed: ./testcases/multi
✅  passed: ./testcases/prio
✅  passed: ./testcases/resources-adv1
✅  passed: ./testcases/resources-adv2
✅  passed: ./testcases/resources-basic
✅  passed: ./testcases/resources-prio
✅  passed: ./testcases/single
```