#include "deterministic_scheduler.hpp"
#include "user_cpp_func_define.hpp"

extern "C" {
#include "user_func_define.h"
}

int main() {
  unsigned a = 0;
  unsigned b = 1;
  ScheduleClient dc;
  /* load 7 tasks*/
  dc.allocateTaskArray(7);
  int task_index;
  task_index = dc.addTask(cmp, a, b);
  dc.setTaskParam(task_index, "A", 10, "", 30, 30,
                  ScheduleClient::HowTrig::external_signal, SOURCE_TRIG_SIG_1);
  task_index = dc.addTask(printf_hello);
  dc.setTaskParam(task_index, "B", 10, "A -> B");
  struct functor_type_n ftype(4);
  task_index = dc.addTask(ftype);
  dc.setTaskParam(task_index, "C", 20, "", 40, 40,
                  ScheduleClient::HowTrig::external_signal, SOURCE_TRIG_SIG_2);
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
  dc.setTaskParam(task_index, "D", 20, "", 4000, 0,
                  ScheduleClient::HowTrig::timer, 0);

  /* load 3 pure c functions*/
  int func1_param = 10;
  task_index = dc.addTask(func1, &func1_param);
  dc.setTaskParam(task_index, "E", 5, "", 40, 0,
                  ScheduleClient::HowTrig::external_signal, SOURCE_TRIG_SIG_3);

  long func2_param = -10;
  task_index = dc.addTask(func2, &func2_param);
  dc.setTaskParam(task_index, "F", 1, "E->F");

  int func3_param = 2;
  task_index = dc.addTask(func3, &func3_param);
  dc.setTaskParam(task_index, "G", 10, "", 30, 0,
                  ScheduleClient::HowTrig::external_signal, SOURCE_TRIG_SIG_4);

  dc.spin();
}