#include "internal_pthread.h"

#include <fcntl.h>
#include <pthread.h> /* For pthread*/
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h> /* atoi*/
#include <unistd.h>

#include "func_interface.h" /* struct func*/
#include "posix_shm.h"
#include "timestamp.h"
#define TEST
#ifdef TEST
#include "internal_sched.h"
#endif
static int run_flag = 1;
static struct internal_func_array g_funcs = {.main_tid = 0,
                                             .load_shm = 0,
                                             .burst_time_test_flag = 0,
                                             .funcs = 0,
                                             .tasks = 0,
                                             .idx_in_shm_task_arr = 0,
                                             .watch = 0,
                                             .ptids = 0,
                                             .idx = 0,
                                             .size = 0};
static void unload_shm() {
  if (!g_funcs.load_shm)
    return;
  unload();
  g_funcs.load_shm = 0;
};

int init_internal_func_array(struct func_array *user_provided) {
  if (!user_provided || user_provided->size <= 0) {
    errRet("init_internal_func_array");
  }
  struct internal_func_array *arr = &g_funcs;

  int size = user_provided->size;

  arr->idx_in_shm_task_arr = malloc(sizeof(uint8_t) * size);
  memset(arr->idx_in_shm_task_arr, 0, sizeof(uint8_t) * size);
  arr->watch = malloc(sizeof(struct watch) * size);
  memset(arr->watch, 0, sizeof(struct watch) * size);
  arr->ptids = malloc(sizeof(pthread_t) * size);
  memset(arr->ptids, 0, sizeof(pthread_t) * size);
  arr->idx = malloc(sizeof(int) * size);
  for (int i = 0; i < size; ++i)
    arr->idx[i] = i;
  arr->size = size;
  if (pthread_key_create(&(arr->pthread_key), NULL) != 0)
    errExit("pthread_key_create");
  arr->funcs = user_provided->funcs;
  arr->burst_time_test_flag = user_provided->burst_time_test_flag;
  return 0;
}

int destroy_internal_func_array() {
  struct internal_func_array *arr = &g_funcs;
  if (!arr)
    errRet("destroy_internal_func_array");
  free(arr->idx_in_shm_task_arr);
  free(arr->watch);
  free(arr->ptids);
  free(arr->idx);
  arr->size = 0;
  arr->main_tid = 0;
  return 0;
}

void cpy_thread_node(struct thread_node *to, struct thread_node *from) {
  memcpy(to, from, sizeof(struct thread_node));
}

void catcher(int sig, siginfo_t *info, void *value) {
  (void)info;
  (void)value;
#ifdef TRACE
  printf("pid %d receive signal %d\n", getpid(), sig);
#endif
  long cur_tid = gettid();

  printf("Current tid: %ld receive SIGINT\n", cur_tid);

  if (cur_tid == g_funcs.main_tid) {
    unload_shm();
    run_flag = 0;
    for (int i = 0; i < g_funcs.size; ++i) {
      tkill(g_funcs.funcs[i].descr.thread.tid, SIGINT);
    }
  }
}

static void op_catcher(int sig, siginfo_t *info, void *value) {
  (void)info;
  (void)value;
  pid_t tid = gettid();
  printf("Current tid: %d receive sig %d\n", tid, sig);
#ifdef TEST
  print_cur_thread_sched();
#endif
  int *idx = pthread_getspecific(g_funcs.pthread_key);
  if (idx == NULL)
    printf("tid %d unset pthread key\n", tid);
  else {
    start_watch(&(g_funcs.watch[*idx]));
    call_func(&(g_funcs.funcs[*idx]));
    stop_watch(&(g_funcs.watch[*idx]));
    printf("func %s finished, elapse time %ld ns in the %d-th run\n",
           g_funcs.funcs[*idx].descr.name, g_funcs.watch[*idx].ts_diff,
           g_funcs.funcs[*idx].record.round);
    reschedule_task_done(g_funcs.idx_in_shm_task_arr[*idx]);
  }
}

static void source_trigger_catcher(int sig, siginfo_t *info, void *value) {
  (void)info;
  (void)value;
  pid_t tid = gettid();
  printf("Current tid: %d receive source trigger signal %d\n", tid, sig);
  int *idx = pthread_getspecific(g_funcs.pthread_key);
  if (idx == NULL)
    printf("tid %d unset pthread key\n", tid);
  else {
    reschedule_source_task(g_funcs.idx_in_shm_task_arr[*idx]);
  }
}

static void timer_trigger_catcher(int sig, siginfo_t *info, void *value) {
  (void)info;
  (void)value;
  pid_t tid = gettid();
  printf("Current tid: %d receive timer trigger signal %d\n", tid, sig);
  int *idx = pthread_getspecific(g_funcs.pthread_key);
  if (idx == NULL)
    printf("tid %d unset pthread key\n", tid);
  else {
    reschedule_source_task(g_funcs.idx_in_shm_task_arr[*idx]);
  }
}

void *thread_routine(void *args) {
  /* set thread id*/
  int idx = *(int *)args;
  g_funcs.funcs[idx].descr.thread.tid = gettid();
  if (pthread_setspecific(g_funcs.pthread_key, args) != 0)
    perror("pthread_setspecific");

  /* set signal handler*/
  sigset_t set;
  if (pthread_sigmask(SIG_BLOCK, NULL, &set) < 0)
    perror("pthread_sigmask");

  struct sigaction sigact;
  memset(&sigact, 0, sizeof(struct sigaction));
  /* set op_catcher as the signal handler for
                                       THREAD_ACTIVE_SIG*/
  sigact.sa_sigaction = op_catcher;
  sigemptyset(&(sigact.sa_mask));
  sigact.sa_flags = SA_SIGINFO;
  sigaction(THREAD_ACTIVE_SIG, &sigact, NULL);
  if (sigdelset(&set, THREAD_ACTIVE_SIG) < 0)
    perror("sigdelset");

  /* set source task trigger signal handler*/
  if (g_funcs.funcs[idx].descr.trigger_sig) {
    memset(&sigact, 0, sizeof(struct sigaction));
    /* set source_trigger_catcher as the signal handler for
                      certain source task trig sig*/
    sigact.sa_sigaction = source_trigger_catcher;
    sigact.sa_flags = SA_SIGINFO;
    sigemptyset(&(sigact.sa_mask));
    // sigaddset(&(sigact.sa_mask),
    //           THREAD_ACTIVE_SIG); /* while handle Source Trig sig, block
    //                                  THREAD_ACTIVE_SIG*/
    sigaction(g_funcs.funcs[idx].descr.trigger_sig, &sigact, NULL);
    if (sigdelset(&set, g_funcs.funcs[idx].descr.trigger_sig) < 0)
      perror("sigdelset");
  } else if (g_funcs.funcs[idx].descr.timer_trigger.effective) {
    /* set timer trigger task handler*/
    memset(&sigact, 0, sizeof(struct sigaction));
    sigact.sa_flags = SA_SIGINFO;
    sigact.sa_sigaction = timer_trigger_catcher;
    sigemptyset(&(sigact.sa_mask));
    sigaction(TIMER_TRIG_SIG, &sigact, NULL);
    if (sigdelset(&set, TIMER_TRIG_SIG) < 0)
      perror("sigdelset");
  }

  if (pthread_sigmask(SIG_SETMASK, &set, NULL) < 0)
    perror("pthread_sigmask");

  /* set tid in func*/
  struct thread_node *thread_ptr = &(g_funcs.funcs[idx].descr.thread);
  thread_ptr->tid = gettid();

  /*send reg information.*/
  reg_task_info(&(g_funcs.funcs[idx].descr), &(g_funcs.funcs[idx].dep),
                &(g_funcs.funcs[idx].ttsddl),
                &(g_funcs.idx_in_shm_task_arr[idx]));

  /* suspend current thread*/
  printf("child tid %d load func %s\n", thread_ptr->tid,
         g_funcs.funcs[idx].descr.name);
#ifdef TEST
  print_cur_thread_sched();
#endif
  /* if set timer then start timer.*/
  if (g_funcs.funcs[idx].descr.timer_trigger.effective) {
    struct sigevent te;
    te.sigev_notify = SIGEV_SIGNAL;
    te.sigev_signo = TIMER_TRIG_SIG;
    timer_create(CLOCK_REALTIME, &te,
                 &(g_funcs.funcs[idx].descr.timer_trigger.timer));
    timer_settime(g_funcs.funcs[idx].descr.timer_trigger.timer, 0,
                  &(g_funcs.funcs[idx].descr.timer_trigger.time),
                  NULL); /* relative time*/
  }
  while (run_flag) {
    sigsuspend(&(set));
  }
}

void create_threads() {
  struct internal_func_array *funcs = &g_funcs;
  for (int i = 0; i < funcs->size; ++i) {
    int *idx = &(funcs->idx[i]);
    if (pthread_create(funcs->ptids + i, NULL, thread_routine, idx) != 0)
      perror("pthread_create");
  }
}

void wait_threads() {
  struct internal_func_array *funcs = &g_funcs;
  for (int i = 0; i < funcs->size; ++i) {
    pthread_join(funcs->ptids[i], NULL);
  }
}

int set_sig_int_handler() {
  sigset_t set;
  sigemptyset(&set);
  /* Block all the rt signal that will be used for internal communication.*/
  for (int blk_sig = THREAD_ACTIVE_SIG; blk_sig <= SIGRTMAX; ++blk_sig)
    sigaddset(&set, blk_sig);
  if (pthread_sigmask(SIG_SETMASK, &set, NULL) < 0)
    errExit("pthread_sigmask");

  struct sigaction sigact;
  /* set catcher as the signal handler for SIGINT*/
  /* handle SIGINT won't be interupted by other signals*/
  sigact.sa_sigaction = catcher;
  memcpy(&(sigact.sa_mask), &set, sizeof(sigset_t));
  sigact.sa_flags = SA_SIGINFO;
  if (sigaction(SIGINT, &sigact, NULL) < 0)
    errExit("sigaction");
  return 0;
}

int load_shm() {
  if (load() < 0)
    errRet("load shm");
  g_funcs.load_shm = 1;
  g_funcs.main_tid = gettid();
  return 0;
}

/* every tick second, print out information.*/
int print_shm_routine(int tick) {
  if (set_sig_int_handler() < 0)
    errRet("set_sig_int_handler");
  if (load_shm() < 0)
    errRet("load_shm");
  while (run_flag) {
    shm_dump();
    sleep(tick);
  }
  return 0;
}

int enable_scheduler_routine() {
  if (set_sig_int_handler() < 0)
    errRet("set_sig_int_handler");
  if (load_shm() < 0)
    errRet("load_shm");
  if (enable_scheduler(getnprocs()) < 0)
    errRet("enable_scheduler");
  unload_shm();
  return 0;
}

int test_burst_time() {
  if (!g_funcs.burst_time_test_flag)
    return -1;
  /* set prior*/
  if (set_task_prior(gettid(), 3) < 0)
    return -1;         /* highest prior*/
  int test_times = 10; /* run 10 times for every function*/
  for (int i = 0; i < g_funcs.size; ++i) {
    struct func *func = g_funcs.funcs + i;
    for (int j = 0; j < test_times; ++j) {
      start_watch(g_funcs.watch + i);
      call_func(func);
      stop_watch(g_funcs.watch + i);
      add_burst_time(&(func->burst_time_test), g_funcs.watch[i].ts_diff);
      usleep(10000); /*sleep 10 ms*/
    }
  }

  /* print out the result*/
  printf("We made %d times to test each function's burst time (microsec) :\n",
         test_times);
  for (int i = 0; i < g_funcs.size; ++i) {
    struct func *func = g_funcs.funcs + i;
    if (func->burst_time_test.effective) {
      printf("name %s  max_time %ld average time %ld "
             "min_time %ld\n",
             func->descr.name, func->burst_time_test.max_time / 1000,
             func->burst_time_test.average_time / 1000,
             func->burst_time_test.min_time / 1000);
    }
  }
  return 0;
}