#include "internal_sched.h"

#include <sched.h>
#include <stdio.h>
const struct internal_thread_sched not_ready = {.policy = SCHED_FIFO,
                                                .param.sched_priority = 78};

const struct internal_thread_sched waiting = {.policy = SCHED_FIFO,
                                              .param.sched_priority = 79};

const struct internal_thread_sched running = {.policy = SCHED_FIFO,
                                              .param.sched_priority = 80};

const struct internal_thread_sched source_trigger = {
    .policy = SCHED_FIFO, .param.sched_priority = 81};

int set_task_prior(pid_t tid, int flag) {
  switch (flag) {
    case 0:
      if (sched_setscheduler(tid, not_ready.policy, &(not_ready.param)) < 0)
        errExit("sched_setscheduler");
      break;
    case 1:
      if (sched_setscheduler(tid, waiting.policy, &(waiting.param)) < 0)
        errExit("sched_setscheduler");
      break;
    case 2:
      if (sched_setscheduler(tid, running.policy, &(running.param)) < 0)
        errExit("sched_setscheduler");
      break;
    case 3:
      if (sched_setscheduler(tid, source_trigger.policy,
                             &(source_trigger.param)) < 0)
        errExit("sched_setscheduler");
      break;
    default:
      errRet("wrong param, set_task_prior");
      break;
  }
  return 0;
}

void print_cur_thread_sched() {
  switch (sched_getscheduler(0)) {
    case SCHED_OTHER:
      printf("SCHED_OTHER\n");
      break;
    case SCHED_FIFO:
      printf("SCHED_FIFO\n");
      break;
    case SCHED_RR:
      printf("SCHED_RR\n");
      break;
    default:
      printf("unknown\n");
      break;
  }
}

int set_task_affinity(pid_t tid, int cpu_core_index) {
  cpu_set_t set;
  CPU_ZERO(&set);
  CPU_SET(cpu_core_index, &set);
  if (sched_setaffinity(tid, sizeof(cpu_set_t), &set) < 0)
    errExit("sched_setaffinity");
  return 0;
}

int test_task_affinity(pid_t tid, int cpu_core_index) {
  cpu_set_t set;
  CPU_ZERO(&set);
  if (sched_getaffinity(tid, sizeof(cpu_set_t), &set) < 0)
    errExit("sched_getaffinity");
  return CPU_ISSET(cpu_core_index, &set);
}