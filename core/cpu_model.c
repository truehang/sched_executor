
#include <stdint.h>

#include "posix_shm.h"

#define BYTE_TO_BINARY_PATTERN "%c%c%c%c%c%c%c%c"
#define BYTE_TO_BINARY(byte)                                \
  (byte & 0x80 ? '1' : '0'), (byte & 0x40 ? '1' : '0'),     \
      (byte & 0x20 ? '1' : '0'), (byte & 0x10 ? '1' : '0'), \
      (byte & 0x08 ? '1' : '0'), (byte & 0x04 ? '1' : '0'), \
      (byte & 0x02 ? '1' : '0'), (byte & 0x01 ? '1' : '0')

int is_cpu_occupied(struct cpu *cpu, int core_index) {
  /*To check a bit, shift the number n to the right, then bitwise AND it*/
  return (cpu->occupied_mask >> core_index) & 1U;
}

static inline void set_cpu_occupied(struct cpu *cpu, int core_index) {
  /*Use the bitwise OR operator (|) to set a bit.*/
  cpu->occupied_mask |= 1UL << core_index;
}

static inline void clr_cpu_occupied(struct cpu *cpu, int core_index) {
  /*Use the bitwise AND operator (&) to clear a bit.*/
  cpu->occupied_mask &= ~(1UL << core_index);
}

static inline void dump_cpu_core(struct cpu_core *core,
                                 struct task_array *tasks) {
  printf("index: %d, occupied %s, current task: %s\n", core->idx,
         (core->occupied ? "yes" : "no"),
         tasks->array[core->cur_task_idx].name);
}

void init_cpu(struct cpu *cpu, int size) {
  if (size <= 0 || size > MAXTASKPARALLEL) return;
  memset(cpu, 0, sizeof(struct cpu));
  for (int i = 0; i < size; ++i) {
    cpu->core[i].idx = i;
  }
  cpu->size = size;
  cpu->n_online = size;
}

void dump_cpu(struct cpu *cpu, struct task_array *tasks) {
  if (cpu->size <= 0) {
    printf("cpu not initialized\n");
    return;
  }
  printf("totally %d cpu cores, occupied %d cores as " BYTE_TO_BINARY_PATTERN
         "\n",
         cpu->size, cpu->n_occupied, BYTE_TO_BINARY(cpu->occupied_mask));
  for (int i = 0; i < cpu->size; ++i) {
    if (is_cpu_occupied(cpu, i)) {
      dump_cpu_core(&(cpu->core[i]), tasks);
    }
  }
}

int take_given_cpu(struct cpu *cpu, int core_index, struct task_array *tasks,
                   int task_index) {
  if (core_index >= cpu->size || core_index < 0) return -1;
  if (!is_cpu_occupied(cpu, core_index)) {
    /*not taken*/
    set_cpu_occupied(cpu, core_index);
    --cpu->n_online;
    ++cpu->n_occupied;
  } else {
    /*preemptive*/
    int cur_task_idx = cpu->core[core_index].cur_task_idx;
    tasks->array[cur_task_idx].last_cpu_index = -1;
  }

  tasks->array[task_index].last_cpu_index = core_index;
  cpu->core[core_index].occupied = 1;
  cpu->core[core_index].cur_task_idx = task_index;
  return 0;
}

int release_cpu(struct cpu *cpu, struct task_array *tasks, int task_index) {
  int8_t target_core_idx = tasks->array[task_index].last_cpu_index;
  if (target_core_idx < 0 || !is_cpu_occupied(cpu, target_core_idx)) return -1;
  clr_cpu_occupied(cpu, target_core_idx);
  cpu->core[target_core_idx].occupied = 0;
  ++cpu->n_online;
  --cpu->n_occupied;
  return 0;
}
