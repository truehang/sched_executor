#ifndef INTERNAL_SCHED_HEADER
#define INTERNAL_SCHED_HEADER
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
/* to enable CPU_SET place _GNU_SOURCE macro before <sched.h> and other header
 * files*/

#include <fcntl.h>
#include <sched.h>
#include <stdint.h>

#include "common_define.h"
struct internal_thread_sched {
  int policy;
  /*struct sched_param {int sched_priority;};*/
  struct sched_param param;
};

/* flag: 0 not ready, 1 waiting, 2 running, 3 source trigger */
int set_task_prior(pid_t tid, int flag);
void print_cur_thread_sched();
int set_task_affinity(pid_t tid, int cpu_core_index);
int test_task_affinity(pid_t tid, int cpu_core_index);

#endif