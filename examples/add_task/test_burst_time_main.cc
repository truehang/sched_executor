#include "deterministic_scheduler.hpp"
#include "user_cpp_func_define.hpp"

extern "C" {
#include "user_func_define.h"
}

int main() {
  unsigned a = 0;
  unsigned b = 1;
  ScheduleClient dc;
  /* load 7 tasks to test their burst time*/
  dc.allocateTaskArray(7);
  int task_index;
  task_index = dc.addTask(cmp, a, b);
  /* When test burst time, we only need to add function name, no any time
   * parameter or dependency*/
  dc.setTaskParam(task_index, "A");
  task_index = dc.addTask(printf_hello);
  dc.setTaskParam(task_index, "B");
  struct functor_type_n ftype(4);
  task_index = dc.addTask(ftype);
  dc.setTaskParam(task_index, "C");
  int x = 1, y = 2;
  // Variable x is captured by value, y by reference.
  auto lmd1 = [x, &y]() mutable {
    x *= 2;
    ++y;
    for (int i = 1000000; i > 0; --i)
      ;
    printf("x * y %d\n", x * y);
  };
  task_index = dc.addTask(lmd1);
  dc.setTaskParam(task_index, "D");

  /* load 3 pure c functions*/
  int func1_param = 10;
  task_index = dc.addTask(func1, &func1_param);
  dc.setTaskParam(task_index, "E");

  long func2_param = -10;
  task_index = dc.addTask(func2, &func2_param);
  dc.setTaskParam(task_index, "F");

  int func3_param = 2;
  task_index = dc.addTask(func3, &func3_param);
  dc.setTaskParam(task_index, "G");
  /* At last call testBurstTime not spin*/
  dc.testBurstTime();
}