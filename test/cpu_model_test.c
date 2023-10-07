#include "posix_shm.h"

int main() {
  struct cpu cpu;
  init_cpu(&cpu, 4);
  struct task_array tasks;
  memset(&tasks, 0, sizeof(tasks));
  tasks.size = 5;

  dump_cpu(&cpu, &tasks);
  if (cpu.n_online != 4) errRet("wrong");

  printf("task 0 takes cpu\n");
  take_given_cpu(&cpu, 0, &tasks, 0);
  dump_cpu(&cpu, &tasks);
  if (cpu.n_online != 3) errRet("wrong");

  printf("task 1 takes cpu\n");
  take_given_cpu(&cpu, 1, &tasks, 1);
  dump_cpu(&cpu, &tasks);
  if (cpu.n_online != 2) errRet("wrong");

  printf("task 2 peempts task 1 \n");
  take_given_cpu(&cpu, 1, &tasks, 2);
  dump_cpu(&cpu, &tasks);
  if (cpu.n_online != 2) errRet("wrong");

  printf("task 3 takes cpu\n");
  take_given_cpu(&cpu, 3, &tasks, 3);
  dump_cpu(&cpu, &tasks);
  if (cpu.n_online != 1) errRet("wrong");
  printf("task 4 takes cpu but fail \n");
  if (take_given_cpu(&cpu, 4, &tasks, 4) != -1) errRet("take_cpu");
  dump_cpu(&cpu, &tasks);
  if (cpu.n_online != 1) errRet("wrong");
  printf("task 4 takes cpu another try\n");
  if (take_given_cpu(&cpu, 2, &tasks, 4) != 0) errRet("take_cpu");
  dump_cpu(&cpu, &tasks);
  if (cpu.n_online != 0) errRet("wrong");

  printf("task 1 releases cpu fail\n");
  if (release_cpu(&cpu, &tasks, 1) != -1) errRet("release_cpu");
  dump_cpu(&cpu, &tasks);
  if (cpu.n_online != 0) errRet("wrong");

  printf("task 0 releases cpu\n");
  if (release_cpu(&cpu, &tasks, 0) != 0) errRet("release_cpu");
  dump_cpu(&cpu, &tasks);
  if (cpu.n_online != 1) errRet("wrong");

  printf("task 4 releases cpu\n");
  if (release_cpu(&cpu, &tasks, 4) != 0) errRet("take_cpu");
  dump_cpu(&cpu, &tasks);
  if (cpu.n_online != 2) errRet("wrong");

  printf("success\n");
  return 0;
}