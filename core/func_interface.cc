

#include "deterministic_scheduler.hpp" /* ScheduleClient*/

#include <string>
extern "C" {
#include "func_interface.h" /* call_func */
#include "internal_pthread.h"
void call_func(struct func *func) {
  func->record.status = 1;
  ++(func->record.round);
  static_cast<Invoker *>(func->invoker)->runFunc();
  func->record.status = 2;
}

int destroy_func_array(struct func_array *func_array) {
  if (!func_array) errRet("destroy_func_array");
  for (int i = 0; i < func_array->size; ++i) {
    struct func *func = func_array->funcs + i;
    delete static_cast<Invoker *>(func->invoker);
  }
  free(func_array->funcs);
  free(func_array);
  return 0;
}
}

int ScheduleClient::allocateTaskArray(int size) {
  func_arr_ = create_func_array(size);
  return func_arr_ != NULL;
}

int ScheduleClient::setTaskParam(int task_index, const std::string &short_name,
                                 int burst_time_ms,
                                 const std::string &dependency, int period_ms,
                                 int deadline_ms, const HowTrig &how,
                                 int external_sig) {
  if (valid_func_index(func_arr_, task_index) < 0) {
    printf("setTaskParam failed, invalid param\n");
    return -1;
  }
  if (valid_name(short_name.c_str()) < 0) {
    printf("task name is too long\n");
    return -1;
  }

  if (set_func_time_ms(func_arr_, task_index, short_name.c_str(), burst_time_ms,
                       period_ms, deadline_ms) < 0)
    return -1;
  if (!burst_time_ms) {
    func_arr_->burst_time_test_flag = 1;
    return 0;
  }
  if (how == HowTrig::external_signal) {
    if (set_func_trig(func_arr_, task_index, external_sig) < 0) return -1;
  } else if (how == HowTrig::timer) {
    if (set_func_timer_ms(func_arr_, task_index, period_ms) < 0) return -1;
  }
  if (!dependency.empty()) {
    if (set_func_dep(func_arr_, task_index, dependency.c_str()) < 0) return -1;
  }
  return 0;
}

int ScheduleClient::spin() {
  if (!func_arr_) return -1;
  if (set_sig_int_handler() < 0) return -1;
  if (load_shm() < 0) return -1;
  if (run_funcs(func_arr_) < 0) return -1;
  if (destroy_func_array(func_arr_) < 0) return -1;
  return 0;
}

int ScheduleClient::enableScheduler() { return enable_scheduler_routine(); }

int ScheduleClient::printShm(int tick) { return print_shm_routine(tick); }

int ScheduleClient::testBurstTime() {
  if (!func_arr_) return -1;
  if (test_funcs(func_arr_) < 0) return -1;
  if (destroy_func_array(func_arr_) < 0) return -1;
  return 0;
}
