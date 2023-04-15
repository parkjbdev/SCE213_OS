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

## Implement rr_scheduler

Implemented Round Robin Scheduling

## TODO

- [X] SJF scheduler: 20pts (tested using `multi`)
- [X] STCF scheduler: 50pts (`multi`)
- [X] RR scheduler:  50pts (`multi` and `prio`)
- [ ] Priority scheduler: 60pts (`prio` and `resources-prio`)
- [ ] Priority scheduler + aging: 100pts (`prio`)
- [ ] Priority scheduler + PCP: 70pts (`resources-basic`)
- [ ] Priority scheduler + PIP: 150pts (`resources-adv1` and `resources-adv2`)