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

한가지 의문에 남았던 것은, 기존에 주어진 STCF Scheduler 의 주석에 forked 가 필요할 것이라고 하였지만, 본인은 forked 함수의 구현 없이 scheduler 를 완성하였다는 점이다.
현재 구현에서는 매 스케줄링마다 조건에 맞는 process 를 `readyqueue` 에서 순회하며 찾아서 scheduling 하였지만, forked 함수가 이용되는 로직이라면, 정렬된 `readyqueue` 를 이용하는 방식일 것으로 생각한다.
프로세스가 생성될때마다 정렬된 `readyqueue` 에서 올바른 위치에 fork 된 프로세스를 옮겨주면, schedule 함수에서 `readyqueue` 를 순회하며 조건에 맞는 process 를 찾는 과정 없이 first_entry 를 뽑아 `current` process 와의 우선순위를 비교하여 scheduling 할 수 있기 때문이다.
다만, 이렇게 `readyqueue` 를 관리한다면, SJF Scheduler 에 대해서도 마찬가지로 forked 함수가 필요할 것이다.

## RR Scheduler

RR Scheduler는 round-robin scheduler 의 약자로, 각 프로세스에게 동일한 time quantum 을 할당하여 scheduling 하는 scheduler 이다. 이번 프로젝트에서는 1tick을 하나의 time quantum으로 이용하였다. 기본적으로는 들어온 순서대로 1tick 씩 처리하며, process가 끝나지 않았다면 다음 차례에도 1tick씩 실행하기 위해서 `readyqueue`에 다시 배치된다.
이러한 scheduling 방식으로 인해 round-robin scheduler는 preemptive 하며, starvation이 일어나지 않는다는 특징이 존재한다. 또, process의 response time 측면에서 이득을 볼 수 있다는 장점이 존재한다.

이러한 특징을 가진 RR Scheduler의 구현은 STCF보다도 쉽게 구현할 수 있었다.
preemptive한 fifo scheduler라고 생각할 수 있기 때문에, 큰 틀은 이와 유사하게 구현할 수 있었으며, 대신 1 tick 씩만 실행되고 `readyqueue`의 tail에 다시 저장할 수 있도록 구현하였다.

