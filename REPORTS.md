# Project2: Simulating Process Schedulers

## Abstract

이번 과제는 OS의 process scheduler를 구현하는 과제로, SJF scheduling, STCF scheduling, RR scheduling, Priority scheduling (including Basic Scheduling / Aging / Ceiling Protocol / Inheritance Protocol) 을 직접 구현하는 것이 목표이다.
README.md에 주어진 각 스케줄러에 대한 testcases를 포함하여 주어진 모든 testcase에 대해서 올바른 결과가 나올 수 있도록 고안하였다.
추가적으로, `test*.sh` file들은 컴파일과 실행이 정상적으로 종료되는지 확인하기 위해 작성한 스크립트이다.

## FIFO Scheduler

FIFO Scheduler는 first-in-first-out scheduler의 약자로, 프로세스가 ready된 순서대로 scheduling 해주는 policy 이다.
FIFO Scheduler는 non-preemptive하게 동작하며, ready된 순서대로 스케줄링하기 때문에 starvation이 일어나지 않는다는 특징이 있다.
하지만, 순차적으로 실행되는 특징 때문에, 비교적 `lifespan` 이 긴 프로세스가 `lifespan` 이 짧은 프로세스보다 먼저 스케줄링되는 경우, 전반적인 turnaround time 이 길어진다는 단점이 존재한다.
이러한 단점을 보완하기 위해 `lifespan` 이 짧은 프로세스를 우선적으로 실행하는 SJF scheduling 방법을 이용할 수 있다.

non-preemptive 한 스케줄러이므로, `current` process 가 없어 아무것도 실행되고 있지 않은 경우에만 `readyqueue` 에 들어간 순서대로 scheduling 된다.

## SJF Scheduler

SJF Scheduler는 shortest-job-first scheduler의 약자로, 프로세스가 가장 먼저 끝나는 것을 우선적으로 실행하는 scheduling policy 이다.
SJF Scheduler 역시 non-preemptive 하게 동작하는 scheduler 로, 한번 스케줄링되어 실행중인 process에 대해서는 중간에 다른 process 로 switch 하지 않는다. 하지만 FIFO scheduler 와는 달리 process 가 fork 된 순서가 아닌, shortest job process 를 우선적으로 스케줄링한다.
이러한 특징으로 인해, `lifespan`이 짧은 프로세스가 계속해서 fork되었을 경우, `lifespan`이 긴 프로세스는 계속 대기하게 되는 starvation 현상이 나타날 수 있다.

non-preemptive 한 스케줄러이므로, `current` process 가 없어 아무 process 도 실행하지 않을 경우에 대해서만 shortest job을 찾는 과정을 통하여 scheduling 하여 구현은 간단하였다.

## STCF Scheduler

STCF Scheduler는 shortest-time-to-completion scheduler 의 약자로, non-preemptive 한 SJF Scheduler 를 개선한 preemptive 한 SJF scheduler 이다.
STCF Scheduler 역시 SJF Scheduler 와 마찬가지로, `readyqueue` 에 등록된 process 들 중에서 가장 먼저 끝나는 것을 가장 우선적으로 실행하는 scheduler 이다.
따라서 SJF Scheduler 와 마찬가지로, 비교적 `lifespan` 이 짧은 process 가 계속하여 fork 된다면 작업을 끝내기까지 오랜시간이 걸리는 process 들은 계속 순위가 밀려 starvation 이 발생할 수 있다.
Preemptive 한 scheduler 이기 때문에 앞선 두 scheduler 들과 달리, `current` process 가 실행중일 때에도 가장 먼저 끝날 수 있는 process 가 업데이트되어 달라지면, 해당 process 가 더 우선적으로 scheduling 될 수 있도록 하였다.

한가지 의문에 남았던 것은, 기존에 주어진 STCF Scheduler 의 주석에 forked 가 필요할 것이라고 주석을 통해 확인하였지만, 본인은 forked 함수의 구현 없이 scheduler 를 완성하였다는 점이다.
~~현재 구현에서는 매 스케줄링마다 조건에 맞는 process 를 `readyqueue` 에서 순회하며 찾아서 scheduling 하였지만, forked 함수가 이용되는 로직이라면, 정렬된 `readyqueue` 를 이용하는 방식일 것으로 생각한다.
프로세스가 생성될때마다 정렬된 `readyqueue` 에서 올바른 위치에 fork 된 프로세스를 옮겨주면, schedule 함수에서 `readyqueue` 를 순회하며 조건에 맞는 process 를 찾는 과정 없이 first_entry 를 뽑아 `current` process 와의 우선순위를 비교하여 scheduling 할 수 있기 때문이다.
다만, 이렇게 `readyqueue` 를 관리한다면, SJF Scheduler 에 대해서도 마찬가지로 forked 함수가 필요할 것이다.~~
이와 같은 이유로 forked 함수를 사용할 것을 권장하였다고 생각하였지만, 계속된 refactoring 을 진행하면서 어느정도 일정한 패턴이 있음을 확인하였고, 이러한 패턴을 지키기 위해서는 forked 함수가 필요한 것으로 생각된다.
하지만, 위의 언급한 아이디어를 이용한다면, 매 스케줄링마다 순회하는 횟수를 조금이나마 줄여 오버헤드를 줄일 수 있을 것으로 생각한다.

## RR Scheduler

RR Scheduler는 round-robin scheduler 의 약자로, 각 프로세스에게 동일한 time quantum 을 할당하여 scheduling 하는 scheduler 이다. 이번 프로젝트에서는 1tick을 하나의 time quantum으로 이용하였다. 기본적으로는 들어온 순서대로 1tick 씩 처리하며, process가 끝나지 않았다면 다음 차례에도 1tick씩 실행하기 위해서 `readyqueue`에 다시 배치된다.
이러한 scheduling 방식으로 인해 round-robin scheduler는 preemptive 하며, starvation이 일어나지 않는다는 특징이 존재한다. 또, process의 response time 측면에서 이득을 볼 수 있다는 장점이 존재한다.

이러한 특징을 가진 RR Scheduler의 구현은 STCF보다도 쉽게 구현할 수 있었다.
preemptive한 fifo scheduler라고 생각할 수 있기 때문에, 큰 틀은 이와 유사하게 구현할 수 있었으며, 대신 1 tick 씩만 실행되고 `readyqueue`의 tail에 다시 저장할 수 있도록 구현하였다.
초기에 RR Scheduler 를 구현할 때에는 이러한 FIFO Schedule 와의 공통적인 특징을 미처 생각하지 못하고, 논리흐름은 유사하지만 다른 코드로 구현하였다. 하지만, 이러한 공통적인 특징을 파악하고, 논리흐름을 더 직관적으로 이해할 수 있는 FIFO Scheduler 의 코드를 일부 재사용하여 재구현하였고, 출력결과는 모든 테스트케이스에 대해서 같음을 확인하였다. (955d15b1)

## Priority Scheduler

Priority Scheduler는 각 process의 priority 에 따라 scheduling 하는 순서가 결정되는 방식의 scheduler 이다.
이번 프로젝트에서는 `prio` 멤버 변수의 크기가 클수록 더 높은 priority 를 가진 process 로 규정하였으며, `readyqueue` 에 대기하는 process 들 중에서 가장 높은 priority 를 가진 process 를 매 스케줄링마다 뽑아서 반환해주도록 하였다.
즉, preemptive 한 형태의 priority scheduler 이다.
또, 같은 priority 를 가진 process 들에 대해서는 rr scheduling 을 해주는 것이 목표이다.

이번 priority scheduler의 구현에서 혼동되는 개념들과 잘못된 구현으로 인한 메모리 에러 및 assertion failure 때문에 초반에 난항을 많이 겪었다.
특히 resource 를 acquire 하고 release 하는 과정에서 혼동이 많았는데, 처음 구현할 당시에 가장 크게 헷갈렸던 부분은 resource 를 acquire 하고 release 하는 과정에서 어느시점에 release 를 진행해야 하는지였다.

우선 처음 잘못 이해한 priority scheduler 에서는, 만약 리소스가 이미 다른 더 우선순위가 낮은 프로세스에 의해 사용중인 상태일 때에는, 강제로 해당 리소스를 release 시키고서라도 priority 가 더 높은 프로세스에게 우선적으로 scheduling을 진행해 줄 수 있도록 해야한다고 생각하였다.
하지만 이는 priority inversion에 대한 내용을 정확하게 숙지하지 못한 결과이다.
만약, 우선순위가 더 높은 프로세스가 언제든 강제로 우선순위가 더 낮은 프로세스의 resource 를 뺏어와 우선순위가 더 높은 프로세스로 preempt 할 수 있었다면, priority inversion 과 같은 문제는 일어나지 않았을 것이다.
하지만, 리소스의 특성상, 이미 사용중인 리소스에는 다른 프로세스가 접근하여 동시에 사용할 수 없기 때문에 강제로 리소스 사용을 해제하게 된다면, 이전에 리소스를 사용하여 진행하였던 tick들을 다시 진행해야 하는 상황이 발생한다.
또, 더 높은 우선순위를 가진 process A가 resource 를 acquire 하였을 때, 낮은 우선순위를 가진 process B 에 의해 block 당했다면, block 시킨 process B 를 실행시켜야 더 높은 우선순위를 가진 프로세스A 가 우선적으로 실행될 수 있다고 생각하여, process B 로 스케줄링을 진행하려 하였다.
하지만, 이런 스케줄러가 존재한다면, priority inversion 이 존재하지 않는 스케줄러가 되었을 것이다. 이 역시 priority inversion 에 대해서 잘못 이해하여 생긴 결과였다.

두번째로 또 잘못 구현하였던 것은 `readyqueue`에서 `PROCESS_READY` state 에 있는 프로세스와 `r->waitqueue` 에서 `PROCESS_BLOCKED` state 에 있는 프로세스들을 혼용한 점이다.
resource 를 요청하였지만 블락당한 프로세스들은 `r->waitqueue` 에 등록을 해주어 해당 리소스가 release 될 때에 다시 `readyqueue` 에 추가되어 scheduler 에서 자연스럽게 scheduling 될 수 있도록 해주어야 하는데, priority inversion에 대한 미숙한 이해와 이미 구현된 framework 의 이해 부족으로 scheduler 에서 직접 리소스들에 접근하려는 방법역시 시도하였지만, 사용이 제한된 함수여서 그렇게 하지 못하였다.

그렇게 resource의 release 와 acquire 등에 대한 고민들에 대해서 다시 공부하고 QnA 등을 통해 해결하여 성공적으로 testcase를 통과할 수 있었다.
막상 구현을 완성하고 모든 testcase를 통과하였더니 생각했던 것보다는 단순한 코드를 확인할 수 있었다. 이때의 논리흐름을 가독성있게 다시 정리하여 표현하여 보았더니 (1e77af93), `rr_schedule` 과 한라인을 제외하고 사실상 동일한 코드가 나옴을 확인할 수 있었다.
`rr_schedule` 에서는 priority 에 대한 고려없이 스케줄링을 해주어 단순히 `readyqueue` 의 첫번째 엔트리를 스케줄링 하였지만, `prio_schedule` 에서는 priority 가 가장 높은 프로세스를 직접 구현한 `find_process` 함수를 통해 찾아 스케줄링 해주는 부분이 유일하게 다른 점이였다.

위의 이해한 내용들을 바탕으로 resource를 acquire 하는 함수 `prio_acquire` 를 구현해 본 결과, 사실상 `fcfs_acquire` 와 같은 코드를 쓰고 있는 것을 발견하게 되었다.
acquire 의 경우 resource 의 waitqueue에 등록될때 우선순위가 바뀌는 과정이 존재하지 않는다면, (ex. PIP/PCP Schedulin, aging의 경우 스케줄링 되면 `prio`가 초기상태로 돌아가므로 제외..) 그 과정은
>
> 1. 리소스 점유중이지 않으면 할당
>
> 2. 리소스가 점유되고 있는 중이면 해당 리소스의 `waitqueue`에 등록

과 같은 과정으로 동일하게 나타난다.

resource를 release 하는 함수 `prio_release` 역시 `fcfs_release` 와 거의 유사한 형태로, `fcfs_release` 는 단순히 `waitqueue` 에서 가장 첫번째 엔트리를 `readyqueue` 에 등록해준 반면, `prio_release` 는 `waitqueue` 에서 가장 우선순위가 높은 process를 `readyqueue` 에 등록해준다는 점에서만 차이가 존재하였다.

이러한 시행착오들을 거쳐 구현을 완성하여 모든 testcase들을 통과할 수 있었다.

## Priority + Aging

Priority Scheduling with Aging은 말 그대로, 1 tick 이 흐를 때마다 `readyqueue` 에서 대기중인 프로세스들의 `prio` 를 1씩 높여주고, 스케줄링된 process의 경우 원래의 `prio`로 다시 초기화하는 방식을 채택한 priority scheduler 이다.
이러한 방법을 이용하면, 기존의 priority scheduler에 비해서 starvation 을 예방할 수 있다는 장점이 존재한다.
기본적인 틀은 기존의 priority scheduler와 동일하며, `pa_schedule` 에서도 `prio_schedule` 을 이용하여 다음에 실행될 프로세스를 scheduling 한다.
다만, 스케줄링 받지 못한 프로세스들은 모두 `prio` 를 aging 시킨다는 점이 유일한 차이이다.
기존의 priority scheduler를 이용한 scheduler 인지라, 구현은 간단하였다.
구현 초반에 prio가 모두 1씩 높아지면, 당연히 서로의 우선순위간의 차이가 발생하지 않을 것이라 생각하고, 새로운 process가 fork 되는것이 아닌이상, aging이 무슨 의미가 있는것인지 생각하였는데, current를 제외하여 aging되며, scheduling 받는순간 원래의 priority로 돌아간다는 사실을 간과하고 있어서 발생한 문제였다.
이를 인지하고, testcase들을 직접 계산해보았더니 이러한 priority aging scheduling을 이용하면, 아무리 priority가 제일 낮은 프로세스더라도, 계속하여 그 priority가 높아져 1tick 이라도 실행하여 starvation을 방지하는 것을 확인할 수 있었다.

## Priority + Ceiling Protocol

Priority Scheduling with Ceiling Protocol은 priority inversion을 방지하기 위한 해결책 중 하나로, priority가 더 높은 process가 요청한 resource가 block 당할 경우 priority inversion이 일어나지 않도록 기존에 resource를 점유하고 있는 process의 priority를 일시적으로 최대 priority로 boosting 하는 기법이다.
또, 기존에 점유하고 있던 resource가 없어 acquire 하였다면, 자신의 priority 역시 최대로 boosting 하여 가장 우선적으로 끝날 수 있도록 한다.
일시적인 priority boosting 이므로, 점유하고 있던 리소스를 release 할 때에는 원래의 priority로 돌아가게 된다.

## Priority + Inheritance Protocol

Priority Scheduling with Inheritance Protocol 역시 Ceiling Protocol와 함께 priority inversion을 방지하기 위한 해결책으로, block 시킨 더 낮은 우선순위를 가진 프로세스를 ceiling protocol과는 달리 최대 priority로 boosting 하지 않고, resource를 요청한 더 높은 프로세스의 priority를 상속받아 boosting하는 기법이다.
pip scheduling에서도 pcp scheduling과 마찬가지로, resource를 release 할 때에는 원래의 priority로 돌아와서 원래의 우선순위에 맞게 scheduling 될 수 있도록 한다.

## Lessons Learned

- Resource Acquistion 과 priority inversion 에 대한 이해가 부족하였지만, 이번 프로젝트를 통해 언제 resource 를 acquire 하고, release 할 때의 동작, block 당했을 때의 후속 동작 등을 직접 구현하면서 priority inversion 을 완벽하게 이해할 수 있었다.
- priority aging을 구현하면서 각 프로세스들의 priority 가 점점 높아져 1tick이라도 scheduling 되는 모습을 확인하였다. 이를 통해 starvation을 방지하는 과정을 확인하였다.
- 프로그램을 작성할 때에, `goto` 문의 사용을 지양하고 기피하였는데, 이번 프로젝트에서는 이를 사용하지 않았을 때 오히려 가독성이 떨어지고 logic 의 흐름을 알아차기가 어려운 면이 존재하였다. 따라서 `goto` 문의 무조건적인 기피보다 필요에 따라 적절히 활용하여 더 효율적으로 작성이 가능하다면 이를 채택하고 사용하여도 되겠다는 생각이 들었다.
