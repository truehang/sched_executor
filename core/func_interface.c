
#include "func_interface.h" /* */

#include <stdlib.h> /* malloc*/

#include "common_define.h"    /* errExit*/
#include "internal_pthread.h" /* g_funcs*/
#include "posix_shm.h"        /* set_name*/
struct func_array *create_func_array(int size) {
  if (size <= 0) return NULL;
  struct func_array *res = malloc(sizeof(struct func_array));
  memset(res, 0, sizeof(struct func_array));
  res->funcs = malloc(sizeof(struct func) * size);
  memset(res->funcs, 0, sizeof(struct func) * size);
  res->size = size;
  res->index = 0;
  return res;
}

int add_func(struct func_array *func_array, void *invoker) {
  if (!func_array || func_array->index >= func_array->size || !invoker)
    errRet("add_func");
  struct func *func = func_array->funcs + func_array->index;
  func->invoker = invoker;
  int index = func_array->index;
  ++func_array->index;
  return index;
}

int set_func_time(struct func_array *func_array, int index_in_func_array,
                  const char *name, task_time_t burst_time, task_time_t period,
                  task_time_t deadline) {
  if (!func_array || index_in_func_array >= func_array->size || !name)
    return -1;
  struct func *func = func_array->funcs + index_in_func_array;
  if (set_name(func->descr.name, name) < 0) errRet("name too long, set_name");
  func->descr.bt = burst_time;
  func->descr.p = period;
  func->descr.d = deadline;
  if (set_dep(func->dep.data, name) < 0) errRet("name too long, set_dep");
  return 0;
}

int valid_func_index(struct func_array *func_array, int index_in_func_array) {
  if (!func_array || index_in_func_array >= func_array->size) return -1;
  return 0;
}

int set_func_timer(struct func_array *func_array, int index_in_func_array,
                   task_time_t period) {
  if (!func_array || index_in_func_array >= func_array->size || period <= 0)
    return -1;

  struct func *func = func_array->funcs + index_in_func_array;
  func->descr.timer_trigger.effective = 1;
  func->descr.trigger_sig = 0;
  task_time_t ns_unit = 1000000000;
  func->descr.timer_trigger.time.it_value.tv_sec = period / ns_unit;
  func->descr.timer_trigger.time.it_value.tv_nsec = period % ns_unit;
  func->descr.timer_trigger.time.it_interval.tv_sec = period / ns_unit;
  func->descr.timer_trigger.time.it_interval.tv_nsec = period % ns_unit;
  return 0;
}

int set_func_timer_ms(struct func_array *func_array, int index_in_func_array,
                      int period_ms) {
  task_time_t ms_unit = 1000000;
  return set_func_timer(func_array, index_in_func_array, period_ms * ms_unit);
}

int set_func_trig(struct func_array *func_array, int index_in_func_array,
                  int external_signal) {
  if (!func_array || index_in_func_array >= func_array->size ||
      external_signal < 0)
    return -1;
  struct func *func = func_array->funcs + index_in_func_array;
  func->descr.trigger_sig = external_signal;
  return 0;
}

int set_func_dep(struct func_array *func_array, int index_in_func_array,
                 const char *dependency) {
  if (!func_array || index_in_func_array >= func_array->size || !dependency)
    return -1;
  struct func *func = func_array->funcs + index_in_func_array;
  if (set_dep(func->dep.data, dependency) < 0)
    errRet("dependency too long, set_dep");
}

int set_func_time_ms(struct func_array *func_array, int index_in_func_array,
                     const char *name, int burst_time_ms, int period_ms,
                     int deadline_ms) {
  task_time_t ms_unit = 1000000;
  return set_func_time(func_array, index_in_func_array, name,
                       burst_time_ms * ms_unit, period_ms * ms_unit,
                       deadline_ms * ms_unit);
}

int run_funcs(struct func_array *user_provided) {
  if (init_internal_func_array(user_provided) < 0)
    errRet("init_internal_func_array");

  create_threads();
  wait_threads();
  destroy_internal_func_array();
  return 0;
}

int test_funcs(struct func_array *user_provided) {
  if (init_internal_func_array(user_provided) < 0)
    errRet("init_internal_func_array");
  if (test_burst_time() < 0) errRet("test_burst_time");
  if (destroy_internal_func_array() < 0) errRet("test_burst_time");
  return 0;
}

void add_burst_time(struct burst_time_test *burst_time_record,
                    task_time_t burst_time) {
  if (!burst_time_record || burst_time <= 0) return;
  burst_time_record->effective = 1;
  burst_time_record->total_time += burst_time;
  if (!burst_time_record->count) {
    burst_time_record->max_time = burst_time;
    burst_time_record->min_time = burst_time;
  } else {
    if (burst_time > burst_time_record->max_time)
      burst_time_record->max_time = burst_time;
    if (burst_time < burst_time_record->min_time)
      burst_time_record->min_time = burst_time;
  }
  ++burst_time_record->count;
  burst_time_record->average_time =
      burst_time_record->total_time / burst_time_record->count;
}