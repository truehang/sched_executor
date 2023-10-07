#ifndef INTERNAL_PTHREAD_HEADER
#define INTERNAL_PTHREAD_HEADER
#include <pthread.h>
#include <sched.h>
#include <signal.h>
#include <stdint.h>
#include <sys/types.h>

#include "func_interface.h"
#include "posix_shm.h"
struct watch;
struct internal_func_array {
  pthread_key_t pthread_key;
  pid_t main_tid;
  int load_shm;
  int burst_time_test_flag;     /* to test burst time*/
  struct func *funcs;           /* To shm, free by client*/
  struct task_node *tasks;      /* From shm, free by unload shm*/
  uint8_t *idx_in_shm_task_arr; /* self maintained*/
  struct watch *watch;          /* self maintained*/
  pthread_t *ptids;             /* self maintained*/
  int *idx;                     /* self maintained*/
  int size;
};

int set_sig_int_handler();
int load_shm();
int init_internal_func_array(struct func_array *user_provided);
void create_threads();
void wait_threads();
int destroy_internal_func_array();

void set_thread_id(struct thread_node *node);
void catcher(int sig, siginfo_t *info, void *value);
void *thread_routine(void *args);
void cpy_thread_node(struct thread_node *to, struct thread_node *from);

int print_shm_routine(int tick);
int enable_scheduler_routine();
int test_burst_time();

#endif