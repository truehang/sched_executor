#ifndef FUNC_INTERFACE_HEADER
#define FUNC_INTERFACE_HEADER

#include <stdint.h> /* uint8_t*/

#include "posix_shm.h"

struct func_record {
  int status; /* 0 init val, 1 in execution, 2 done*/
  int round;  /* the round-th execution*/
};
struct burst_time_test {
  uint8_t effective;
  uint8_t count;
  task_time_t total_time;
  task_time_t average_time;
  task_time_t max_time;
  task_time_t min_time;
};

struct func {
  void *invoker;             /* function invoker, managed by ourself*/
  struct func_record record; /* run time status record*/
  struct burst_time_test
      burst_time_test; /* to test this function burst time statics*/
  struct preschedule_task_info descr; /*description */
  /* required dependency: e.g. A or A->B
  if you know current task's dependency, fill A->B form;
  otherwise just fill current task name, like A form*/
  struct preschedule_dependency dep;
  struct preschedule_tts_deadline ttsddl; /* optional tts_deadline*/
};

struct func_array {
  struct func *funcs;
  int index;
  int size;
  int burst_time_test_flag; /* if test burst time then set this flag*/
};

struct func_array *create_func_array(int size);
int destroy_func_array(struct func_array *func_array);
int valid_func_index(struct func_array *func_array, int index_in_func_array);
/*return index in func_array, invoker stores the information about user function
 */
int add_func(struct func_array *func_array, void *invoker);

/* meanwhile set dependency as "name" */
int set_func_time(struct func_array *func_array, int index_in_func_array,
                  const char *name, task_time_t burst_time, task_time_t period,
                  task_time_t deadline);

int set_func_time_ms(struct func_array *func_array, int index_in_func_array,
                     const char *name, int burst_time_ms, int period_ms,
                     int deadline_ms);

int set_func_trig(struct func_array *func_array, int index_in_func_array,
                  int external_signal);
int set_func_dep(struct func_array *func_array, int index_in_func_array,
                 const char *dependency);
int set_func_timer(struct func_array *func_array, int index_in_func_array,
                   task_time_t period);

int set_func_timer_ms(struct func_array *func_array, int index_in_func_array,
                      int period_ms);

int run_funcs(struct func_array *func_arr);

void call_func(struct func *func);

int test_funcs(struct func_array *func_arr);

void add_burst_time(struct burst_time_test *burst_time_record,
                    task_time_t burst_time);
#endif