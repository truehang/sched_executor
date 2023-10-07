/* This file is defining struct that used in posix shm for scheduling.*/
#ifndef DETERMINISM_EXE_POSIX_SHM
#define DETERMINISM_EXE_POSIX_SHM

#include <fcntl.h>
#include <pthread.h>
#include <stddef.h> /* NULL*/
#include <stdint.h>
#include <stdio.h>       /* printf */
#include <string.h>      /* memset*/
#include <sys/syscall.h> /* __NR_gettid*/
#include <sys/types.h>
#include <unistd.h> /* sleep*/

#include "common_define.h"
#include "signal_agreement.h"

#define gettid() \
  syscall(__NR_gettid) /* get linux thread id, or pid of a thread*/
#define getnprocs() sysconf(_SC_NPROCESSORS_CONF) /* get total processors*/
#define tkill(tid, sig) syscall(SYS_tkill, tid, sig)

#define TASKVOLUME 100
#define MAXTASKPARALLEL 64
#define TASKNAMELENGTH 32
#define task_time_t int64_t
#define DEPENDENCYLENGTH 20
#define TTSDEADLINELENGTH 10
/* 1st part: struct in schedule*/

struct thread_node {
  pid_t tid;
};

struct task_node {
  char name[TASKNAMELENGTH];
  struct thread_node thread;
  /* associated params, fixed value*/
  task_time_t d;         /* deadline */
  task_time_t bt;        /* burst time*/
  task_time_t leto;      /* latest execution time offset*/
  int8_t last_cpu_index; /* -1 means invalid*/
  uint8_t is_source;     /* 0 for none-source task, 1 for source task*/

  /* do not add none-fixed value before 'active' filed (or fixed value after
   * 'active'), for reset_stask() will reset every element followed by 'active'
   * to 0*/
  /* none-fixed value*/
  /* 0 not ready
     1 in waiting status
     2 in running status*/
  uint8_t active;
  /* most important params in scheduling*/
  task_time_t wtb; /* waiting time budget*/
  task_time_t cet; /* cumulative execution time*/
  task_time_t wt;  /* waiting time*/
  /* arrival time offset (ato), value will be updated real time*/
  /* ato[cur_task] = ato + wt + cet|[pre_task], when pre_task->cur_task*/
  task_time_t ato;
};

struct task_array {
  struct task_node array[TASKVOLUME];
  uint8_t size;
};

struct source_sig {
  uint8_t idx_in_task_arr;
  int trigger_sig;
};

struct source_task {
  struct source_sig
      array[TASKVOLUME]; /* when TASKVOLUME no more than 256, use 8 bits*/
  uint8_t size;
};

struct task_dependency {
  /* connection[row][column],
    for connection[i][j],
    current task index is i,
    next task index is j,
    connection[i][j] > 0 if  dependency i -> j*/
  uint8_t connection[TASKVOLUME][TASKVOLUME];
};

struct priority_queue {
  uint8_t queue[TASKVOLUME]; /* Store index in task array*/
  uint8_t size;
  uint8_t max_heap; /* max heap or min heap. 0 for min, 1 for max*/
};

struct cpu_core {
  uint8_t idx;
  uint8_t occupied; /* true or false*/
  uint8_t cur_task_idx;
};

struct cpu {
  struct cpu_core core[MAXTASKPARALLEL];
  uint8_t size;
  uint8_t n_online;       /* number of online (available) cpu*/
  uint8_t n_occupied;     /* number of occupied cpu*/
  uint64_t occupied_mask; /* bit set of occupied cpu core index*/
};

struct schedule_data {
  struct task_array tasks;
  struct source_task source_tasks;
  struct task_dependency dependencies;
  struct cpu cpu;
  struct priority_queue wait_queue;    /* min heap*/
  struct priority_queue running_tasks; /* max heap*/
  int max_parallel;
  task_time_t last_time;
};
/*==========1st part (struct in schedule) end*/

/* 2nd part: User input preschedule information.*/
struct timer_trigger {
  uint8_t effective; /* */
  timer_t timer;
  struct itimerspec time;
};
struct preschedule_task_info {
  struct thread_node thread;
  char name[TASKNAMELENGTH];
  /* time no more than 10 sec*/
  task_time_t bt; /* burst time*/
  task_time_t p;  /*period */
  task_time_t d;  /*deadline */
  int trigger_sig;
  struct timer_trigger timer_trigger;
};

struct preschedule_task_info_array {
  struct preschedule_task_info array[TASKVOLUME];
  uint8_t size;
};

struct preschedule_dependency {
  char data[DEPENDENCYLENGTH];
};

struct preschedule_dependency_array {
  struct preschedule_dependency array[TASKVOLUME];
  uint8_t size;
};

struct preschedule_tts_deadline {
  char data[TTSDEADLINELENGTH];
};

struct preschedule_tts_deadline_array {
  struct preschedule_tts_deadline array[TASKVOLUME];
  uint8_t size;
};

struct preschedule_input_data {
  struct preschedule_task_info_array task_infos;
  struct preschedule_dependency_array dependencies;
  struct preschedule_tts_deadline_array tts_deadlines;
};
/*========== 2nd part (user input preschedule information) end*/

/* 3rd part: the whole struct in shm look like*/
union data {
  struct schedule_data ready_data;
  struct preschedule_input_data pre_data;
};

struct shared_data {
  /* status:
   0: Not prepared.
   1: Input done.
   2：After preprocess.
   3: In scheduling.
*/
  uint8_t current_status;
  union data data;
};

struct shared {
  pthread_mutex_t lock;
  int i; /* i is reserved for read/write data consistency test*/
  struct shared_data sched_info;
};
/*========== 3rd part (the whole struct in shm) end*/

void dump_ready_data(struct schedule_data *data);
void dump_pre_data(struct preschedule_input_data *data);
void shm_dump();

int load();
int unload();
int set_name(char target[TASKNAMELENGTH], const char *source);
int valid_name(const char *source);
int set_dep(char target[DEPENDENCYLENGTH], const char *source);
int reschedule_source_task(uint8_t task_index);
int reschedule_task_done(uint8_t task_index);
int reg_task_info(struct preschedule_task_info *new_func_info,
                  struct preschedule_dependency *dep,
                  struct preschedule_tts_deadline *tts_deadline,
                  uint8_t *idx_in_shm_task_arr);
int enable_scheduler(int max_max_parallel);
/* for test*/
int plus_operation();
int sub_operation();

void init_cpu(struct cpu *cpu, int size);
void dump_cpu(struct cpu *cpu, struct task_array *tasks);
int is_cpu_occupied(struct cpu *cpu, int core_index);
int take_given_cpu(struct cpu *cpu, int core_index, struct task_array *tasks,
                   int task_index);
int release_cpu(struct cpu *cpu, struct task_array *tasks, int task_index);

#endif