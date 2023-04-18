# 0415

## Refactor: Simplify Logic of SJF Scheduler

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

## Bug Possibility

- Does STCF really needs `forked` callback?

## Implement rr_scheduler

Implemented Round Robin Scheduling

# 0419

## Implement Priority Scheduler

### `prio_queue`

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

## `rqueue`

- used for debugging
- similar to `list_entry` or `container_of`

## `list_add_safe` and `list_add_tail_safe`

- check if the list_head to add is empty

## Bug Possibility

### `prio_release`

- doesn't release all resources when its preempted
- maybe `assert(r->owner, current)` should work even though in case of process preempted by higher priority process
  and acquired back again

## Testcase Status

- [X] testcases/single
- [X] testcases/multi
- [X] testcases/prio
- [X] testcases/resources-basic
- [X] testcases/resources-prio
- [X] testcases/resources-adv1
- [ ] testcases/resources-adv2

# TODO

- [X] SJF scheduler: 20pts (tested using `multi`)
- [X] STCF scheduler: 50pts (`multi`)
- [X] RR scheduler:  50pts (`multi` and `prio`)
- [ ] Priority scheduler: 60pts (`prio` and `resources-prio`)
- [ ] Priority scheduler + aging: 100pts (`prio`)
- [ ] Priority scheduler + PCP: 70pts (`resources-basic`)
- [ ] Priority scheduler + PIP: 150pts (`resources-adv1` and `resources-adv2`)
