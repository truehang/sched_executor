#include <pthread.h>
#include <signal.h>
#include <stdatomic.h> /* atomic_int*/
#include <stdio.h>
#include <stdlib.h> /* strtol*/
#include <time.h>
#include <unistd.h>

// #include "internal_pthread.h"
// #include "posix_shm.h"
#include "common_define.h"
#include "deterministic_scheduler_user_signal.h"

void *thread1(void *args) {
  atomic_int run_flag = 1;
  while (run_flag--) {
    union sigval val;
    if (sigqueue(strtol(args, NULL, 10), SOURCE_TRIG_SIG_1, val) < 0) {
      perror("sigqueue");
      usleep(30000);
    }
  }
}

void *thread2(void *args) {
  atomic_int run_flag = 1;
  while (run_flag--) {
    union sigval val;
    usleep(10000);
    if (sigqueue(strtol(args, NULL, 10), SOURCE_TRIG_SIG_2, val) < 0) {
      perror("sigqueue");
      usleep(30000);
    }
  }
}

int main(int argc, char *argv[]) {
  pthread_t ptid1;
  pthread_t ptid2;
  if (argc == 2) {
    if (pthread_create(&ptid1, NULL, thread1, argv[1]) < 0)
      errRet("pthread_create");
    pthread_join(ptid1, NULL);
  } else if (argc > 2) {
    // if (set_sig_int_handler() < 0) errRet("set_sig_int_handler");
    // refer load_example_funcs()

    if (pthread_create(&ptid1, NULL, thread1, argv[1]) < 0)
      errRet("pthread_create");
    if (pthread_create(&ptid2, NULL, thread2, argv[2]) < 0)
      errRet("pthread_create");

    pthread_join(ptid1, NULL);
    pthread_join(ptid2, NULL);
  }

  return 0;
}