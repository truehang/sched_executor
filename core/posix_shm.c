#include "posix_shm.h"

#include <bits/types/sigset_t.h>
#include <fcntl.h>    /* For O_* constants, file control */
#include <pthread.h>  /*pthread_mutex_lock */
#include <signal.h>   /* sigqueue*/
#include <stdio.h>    /* perror*/
#include <string.h>   /* memccpy*/
#include <sys/mman.h> /* For shm, link with -lrt*/
#include <sys/stat.h> /* For mode constants */
#include <sys/types.h>
#include <unistd.h> /* ftruncate pause*/

#include "encapsulation_in_c.h" /* preprocess*/
#include "internal_pthread.h"
#include "internal_sched.h"
#include "pq_shm.h"    /* pq_insert*/
#include "timestamp.h" /* get_nano_time*/

static char shm_name[] = "/determinism_sched";
static struct shared *share_memory = NULL;

struct activate_pid_arr {
  struct thread_node arr[16]; /* no more than max_parallel*/
  int size;
};

void dump_ready_data(struct schedule_data *data) {
  if (data->tasks.size == 0)
    printf("No task infos\n");
  else {
    for (int i = 0; i < data->tasks.size; ++i) {
      printf("name: %s\t", data->tasks.array[i].name);
      printf(", tid %d", data->tasks.array[i].thread.tid);
      printf(", active: %d\t", data->tasks.array[i].active);
      printf(", wtb: %ld", data->tasks.array[i].wtb);
      printf(", cet: %ld", data->tasks.array[i].cet);
      printf(", wt: %ld", data->tasks.array[i].wt);
      printf(", d: %ld", data->tasks.array[i].d);
      printf(", bt: %ld", data->tasks.array[i].bt);
      printf(", leto: %ld", data->tasks.array[i].leto);
      printf(", ato: %ld", data->tasks.array[i].ato);
      printf(", post task:");
      for (int j = 0; j < data->tasks.size; ++j) {
        if (data->dependencies.connection[i][j] != 0)
          printf("%s\t", data->tasks.array[j].name);
      }
      printf("\n");
    }
  }

  if (data->source_tasks.size > 0) {
    printf("source task:");
    for (int i = 0; i < data->source_tasks.size; ++i) {
      int idx = data->source_tasks.array[i].idx_in_task_arr;
      printf("%s\t", data->tasks.array[idx].name);
    }
    printf("\n");
  } else {
    printf("no source task\n");
  }

  dump_cpu(&(data->cpu), &(data->tasks));

  if (data->wait_queue.size == 0) {
    printf("empty wait queue\n");
  } else {
    printf("wait queue: ");
    pq_dump(&(data->wait_queue), &(data->tasks));
  }

  if (data->running_tasks.size == 0) {
    printf("no running tasks\n");
  } else {
    printf("running tasks: ");
    pq_dump(&(data->running_tasks), &(data->tasks));
  }

  printf("max parallel: %d\n", data->max_parallel);
  printf("last time (nanosec): %ld\n", data->last_time);
}

void dump_pre_data(struct preschedule_input_data *data) {
  if (data->task_infos.size == 0)
    printf("No task infos\n");
  else {
    for (int i = 0; i < data->task_infos.size; ++i) {
      printf(" name: %s\t", data->task_infos.array[i].name);
      printf(", tid %d", data->task_infos.array[i].thread.tid);
      printf(", bt: %ld\t", data->task_infos.array[i].bt);
      printf(", p: %ld\n", data->task_infos.array[i].p);
    }
  }

  if (data->dependencies.size == 0)
    printf("No Dependencies\n");
  else {
    for (int i = 0; i < data->dependencies.size; ++i) {
      printf("%s\t", data->dependencies.array[i].data);
    }
    printf("\n");
  }

  if (data->tts_deadlines.size == 0)
    printf("No tts deadline\n");
  else {
    for (int i = 0; i < data->tts_deadlines.size; ++i) {
      printf("%s\t", data->tts_deadlines.array->data);
    }
    printf("\n");
  }
}

void shm_dump() {
  printf("======= i: %d\n", share_memory->i);
  if (share_memory->sched_info.current_status < 2) {
    dump_pre_data(&(share_memory->sched_info.data.pre_data));

  } else {
    dump_ready_data(&(share_memory->sched_info.data.ready_data));
  }
}

int load() {
  size_t fd_sz = sizeof(struct shared);
  /* Create shared memory object and set its size to the size
                  of our structure. */

  /* Create new*/
  int shm_fd;
  shm_fd = shm_open(shm_name, O_CREAT | O_EXCL | O_RDWR, 00666);
  if (shm_fd < 0) {
    /* Open exist*/
    shm_fd = shm_open(shm_name, O_RDWR, 00666);
    if (shm_fd < 0) errExit("shm_open");
  } else {
    printf("Create new shm\n");
    if (ftruncate(shm_fd, fd_sz) == -1) errExit("ftruncate");
  }

#ifdef TRACE
  struct stat file_stat;
  fstat(shm_fd, &file_stat);
  printf("fd %d shm file size: %ld sizeof(struct shared): %ld\n", shm_fd,
         file_stat.st_size, sizeof(struct shared));
#endif
  /* Map the object into the caller's address space. */
  void *tmp_ptr =
      mmap(NULL, fd_sz, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
  if (tmp_ptr == MAP_FAILED) errExit("mmap");
  share_memory = tmp_ptr;
  return 0;
}

int unload() {
  if (munmap(NULL, sizeof(struct shared)) == -1) errExit("munmap");
  /* Unlink the shared memory object. Even if the peer process
                  is still using the object, this is okay. The object will
                  be removed only after all open references are closed. */
  shm_unlink(shm_name);
  return 0;
}

int set_name(char target[TASKNAMELENGTH], const char *source) {
  if (memccpy(target, source, 0, TASKNAMELENGTH) == NULL) {
    target[TASKNAMELENGTH - 1] = '\0';
    errRet("set_name");
  }
  return 0;
}

int valid_name(const char *source) {
  if (strlen(source) >= TASKNAMELENGTH) return -1;
  return 0;
}

int set_dep(char target[DEPENDENCYLENGTH], const char *source) {
  if (memccpy(target, source, 0, DEPENDENCYLENGTH) == NULL) {
    target[DEPENDENCYLENGTH - 1] = '\0';
    errRet("set_dep");
  }
  return 0;
}

int activate_tid(struct thread_node *target_tid) {
  if (tkill(target_tid->tid, THREAD_ACTIVE_SIG) < 0) errExit("tkill");
  return 0;
}

int reschedule_source_task(uint8_t task_index) {
  if (!share_memory ||
      task_index >= share_memory->sched_info.data.ready_data.tasks.size)
    errRet("invalid input param, reschedule");
  if (share_memory->sched_info.current_status < 2)
    errRet("preprocess not finish, reschedule");

  pthread_mutex_lock(&(share_memory->lock));

  /*for better logic understanding, place less significant local variables
   * here*/
  struct task_array *task_array_p =
      &(share_memory->sched_info.data.ready_data.tasks);
  struct cpu spare_cpu;
  memset(&spare_cpu, 0, sizeof(struct cpu));
  int spare_idx = 0;
  struct activate_pid_arr pid_arr;
  memset(&pid_arr, 0, sizeof(pid_arr));
  int max_parall = share_memory->sched_info.data.ready_data.max_parallel;
  if (max_parall <= 0) printf("invalid max_parall\n");
  task_time_t delta_time;
  task_time_t now = get_nano_time();

  /* get delta time and update global timestamp in shm*/
  if (share_memory->sched_info.current_status == 2) {
    /* the first ever source task*/
    delta_time = 0;
    share_memory->sched_info.current_status = 3;
  } else {
    delta_time = now - share_memory->sched_info.data.ready_data.last_time;
  }
  share_memory->sched_info.data.ready_data.last_time = now;

  /*update param in wait queue*/
  struct priority_queue *wq =
      &(share_memory->sched_info.data.ready_data.wait_queue);
  for (uint8_t i = 0; i < wq->size; ++i) {
    uint8_t tsk_idx = wq->queue[i];
    struct task_node *tsk_ptr = &(task_array_p->array[tsk_idx]);
    tsk_ptr->wt += delta_time;
    tsk_ptr->wtb -= delta_time;
  }

  /* update param in cpu*/
  struct cpu *cpu = &(share_memory->sched_info.data.ready_data.cpu);
  for (uint8_t i = 0; i < cpu->size; ++i) {
    if (is_cpu_occupied(cpu, i)) {
      uint8_t tsk_idx = cpu->core[i].cur_task_idx;
      struct task_node *tsk_ptr = &(task_array_p->array[tsk_idx]);
      tsk_ptr->cet += delta_time;
      tsk_ptr->wtb -= delta_time;
    } else {
      /* store spare cpu core*/
      memcpy(spare_cpu.core + spare_cpu.size, cpu->core + i,
             sizeof(struct cpu_core));
      ++spare_cpu.size;
    }
  }

  /* add current task to wait queue*/
  struct task_node *cur_node = &(task_array_p->array[task_index]);
  cur_node->active = 1;
  cur_node->ato = 0;
  task_time_t default_wtb = cur_node->d - cur_node->bt;
  task_time_t suggested_wtb = cur_node->leto - cur_node->ato;
  cur_node->wtb = (default_wtb > suggested_wtb)
                      ? suggested_wtb
                      : default_wtb; /* wtb is a fixed value for source task*/
  cur_node->cet = 0;
  cur_node->wt = 0;
  pq_insert(wq, task_index, task_array_p);
  set_task_prior(cur_node->thread.tid, 1); /*set wait prior*/

  /* LWTBF scheduling*/
  struct priority_queue *rt =
      &(share_memory->sched_info.data.ready_data.running_tasks);
  while (wq->size > 0) {
    int i = pq_top(wq);
    if (rt->size < max_parall) {
      pq_insert(rt, i, task_array_p);
      pq_extract(wq, task_array_p);
      /*binding cpu and set prior for i*/
      int spare_core_index = spare_cpu.core[spare_idx++].idx;
      take_given_cpu(cpu, spare_core_index, task_array_p, i);
      cpy_thread_node(&(pid_arr.arr[pid_arr.size++]),
                      &(task_array_p->array[i].thread));
      set_task_prior(task_array_p->array[i].thread.tid, 2); /*set run prior*/
      set_task_affinity(task_array_p->array[i].thread.tid,
                        spare_core_index); /* set cpu affinity*/

    } else {
      /*preemptive part*/
      int j = pq_top(rt);
      if (task_array_p->array[i].wtb < task_array_p->array[j].wtb) {
        /* remove j from running tasks, and add it to waiting queue, and then
         * set prior*/
        int core_index = task_array_p->array[j].last_cpu_index;
        pq_insert(wq, j, task_array_p);
        pq_extract(rt, task_array_p);
        release_cpu(cpu, task_array_p, j);
        set_task_prior(task_array_p->array[j].thread.tid, 1); /*set wait prior*/

        /*arrange i to cpu, and binding cpu*/
        pq_insert(rt, i, task_array_p);
        pq_extract(wq, task_array_p);
        take_given_cpu(cpu, core_index, task_array_p, i);
        cpy_thread_node(&(pid_arr.arr[pid_arr.size++]),
                        &(task_array_p->array[i].thread));
        set_task_prior(task_array_p->array[i].thread.tid, 2); /*set run prior*/
        set_task_affinity(task_array_p->array[i].thread.tid,
                          core_index); /* set cpu affinity*/
      } else
        break; /*preemption done*/
    }
  }

  pthread_mutex_unlock(&(share_memory->lock));
  for (int i = 0; i < pid_arr.size; ++i) activate_tid(&(pid_arr.arr[i]));
  return 0;
}

static inline void reset_stask(struct task_node *cur_node) {
  memset(&(cur_node->active), 0,
         sizeof(*cur_node) -
             ((uint8_t *)&(cur_node->active) - (uint8_t *)cur_node));
}

int reschedule_task_done(uint8_t task_index) {
  if (!share_memory ||
      task_index >= share_memory->sched_info.data.ready_data.tasks.size)
    errRet("invalid input param, reschedule");
  if (share_memory->sched_info.current_status < 3)
    errRet("not from source task, reschedule");

  pthread_mutex_lock(&(share_memory->lock));

  /*for better logic understanding, place less significant local variables
   * here*/
  struct task_array *task_array_p =
      &(share_memory->sched_info.data.ready_data.tasks);
  struct cpu spare_cpu;
  memset(&spare_cpu, 0, sizeof(struct cpu));
  int spare_idx = 0;
  struct activate_pid_arr pid_arr;
  memset(&pid_arr, 0, sizeof(pid_arr));
  int max_parall = share_memory->sched_info.data.ready_data.max_parallel;
  if (max_parall <= 0) printf("invalid max_parall\n");
  task_time_t delta_time;
  task_time_t now = get_nano_time();

  /* get delta time and update global timestamp in shm*/
  delta_time = now - share_memory->sched_info.data.ready_data.last_time;
  share_memory->sched_info.data.ready_data.last_time = now;

  /* update param in wait queue*/
  struct priority_queue *wq =
      &(share_memory->sched_info.data.ready_data.wait_queue);
  for (uint8_t i = 0; i < wq->size; ++i) {
    uint8_t tsk_idx = wq->queue[i];
    struct task_node *tsk_ptr = &(task_array_p->array[tsk_idx]);
    tsk_ptr->wt += delta_time;
    tsk_ptr->wtb -= delta_time;
    // printf("%s wtb: %ld\n", tsk_ptr->name, tsk_ptr->wtb);
  }

  /* update param in cpu*/
  struct cpu *cpu = &(share_memory->sched_info.data.ready_data.cpu);
  for (uint8_t i = 0; i < cpu->size; ++i) {
    if (is_cpu_occupied(cpu, i)) {
      uint8_t tsk_idx = cpu->core[i].cur_task_idx;
      struct task_node *tsk_ptr = &(task_array_p->array[tsk_idx]);
      tsk_ptr->cet += delta_time;
      tsk_ptr->wtb -= delta_time;
    } else {
      /* store spare cpu core*/
      memcpy(spare_cpu.core + spare_cpu.size, cpu->core + i,
             sizeof(struct cpu_core));
      ++spare_cpu.size;
    }
  }

  /* add post tasks to wait queue*/
  // calculate current task actural turn around time
  struct task_node *cur_node = &(task_array_p->array[task_index]);
  task_time_t actural_tat = cur_node->cet + cur_node->wt;
  uint8_t task_size = task_array_p->size;
  // add post tasks to wait queue
  for (uint8_t i = 0; i < task_size; ++i) {
    if (share_memory->sched_info.data.ready_data.dependencies
            .connection[task_index][i] > 0) {
      struct task_node *nx_node = &(task_array_p->array[i]);
      nx_node->active = 1;
      nx_node->ato = cur_node->ato + actural_tat;
      task_time_t default_wtb = nx_node->d - nx_node->bt;
      task_time_t suggested_wtb = nx_node->leto - nx_node->ato;
      nx_node->wtb =
          (default_wtb > suggested_wtb) ? suggested_wtb : default_wtb;
      nx_node->wt = 0;
      nx_node->cet = 0;
      // printf("%s wtb: %ld\n", nx_node->name, nx_node->wtb);
      pq_insert(wq, i, task_array_p);
      set_task_prior(nx_node->thread.tid, 1); /*set wait prior*/
    }
  }

  /* get running tasks*/
  struct priority_queue *rt =
      &(share_memory->sched_info.data.ready_data.running_tasks);

  /* set current task done*/
  int release_core_index = task_array_p->array[task_index].last_cpu_index;
  pq_remove_key(rt, task_index, task_array_p);
  release_cpu(cpu, task_array_p, task_index);
  /* store current cpu as a spare cpu*/
  memcpy(spare_cpu.core + spare_cpu.size, cpu->core + release_core_index,
         sizeof(struct cpu_core));
  ++spare_cpu.size;
  reset_stask(cur_node);
  if (cur_node->is_source)
    set_task_prior(cur_node->thread.tid, 3); /*set source trigger prior*/
  else
    set_task_prior(cur_node->thread.tid,
                   0); /*set none-source task not ready prior*/

  /* LWTBF scheduling*/
  while (wq->size > 0) {
    int i = pq_top(wq);
    if (rt->size < max_parall) {
      pq_insert(rt, i, task_array_p);
      pq_extract(wq, task_array_p);
      /*binding cpu and set prior for i*/
      int spare_core_index = spare_cpu.core[spare_idx++].idx;
      take_given_cpu(cpu, spare_core_index, task_array_p, i);
      cpy_thread_node(&(pid_arr.arr[pid_arr.size++]),
                      &(task_array_p->array[i].thread));
      set_task_prior(task_array_p->array[i].thread.tid, 2); /*set run prior*/
      set_task_affinity(task_array_p->array[i].thread.tid,
                        spare_core_index); /* set cpu affinity*/
    } else {
      /*preemptive part*/
      int j = pq_top(rt);
      if (task_array_p->array[i].wtb < task_array_p->array[j].wtb) {
        /* remove j from running tasks, and add it to waiting queue, and then
         * set prior*/
        int core_index = task_array_p->array[j].last_cpu_index;
        pq_insert(wq, j, task_array_p);
        pq_extract(rt, task_array_p);
        release_cpu(cpu, task_array_p, j);
        set_task_prior(task_array_p->array[j].thread.tid, 1); /*set wait prior*/

        /*arrange i to cpu, and binding cpu*/
        pq_insert(rt, i, task_array_p);
        pq_extract(wq, task_array_p);
        take_given_cpu(cpu, core_index, task_array_p, i);
        cpy_thread_node(&(pid_arr.arr[pid_arr.size++]),
                        &(task_array_p->array[i].thread));
        set_task_prior(task_array_p->array[i].thread.tid, 2); /*set run prior*/
        set_task_affinity(task_array_p->array[i].thread.tid,
                          core_index); /* set cpu affinity*/
      } else
        break; /*preemption done*/
    }
  }

  pthread_mutex_unlock(&(share_memory->lock));
  for (int i = 0; i < pid_arr.size; ++i) activate_tid(&(pid_arr.arr[i]));
  return 0;
}

int plus_operation() {
  if (!share_memory) {
    printf("shm ptr is null");
    return -1;
  }
  pthread_mutex_lock(&(share_memory->lock));
  ++(share_memory->i);
  // printf("i: %d\n", share_memory->i);
  pthread_mutex_unlock(&(share_memory->lock));
  return 0;
}

int sub_operation() {
  if (!share_memory) {
    printf("shm ptr is null");
    return -1;
  }
  pthread_mutex_lock(&(share_memory->lock));
  --(share_memory->i);
  // printf("i: %d\n", share_memory->i);
  pthread_mutex_unlock(&(share_memory->lock));
  return 0;
}

/* normal op*/
int reg_task_info(struct preschedule_task_info *new_func_info,
                  struct preschedule_dependency *dep,
                  struct preschedule_tts_deadline *tts_deadline,
                  uint8_t *idx_in_shm_task_arr) {
  if (!new_func_info) errRet("invalid input params, reg_task_info");
  if (share_memory->sched_info.data.ready_data.tasks.size >= TASKVOLUME)
    errRet("task over flow, reg_task_info");
  if (share_memory->sched_info.current_status != 0)
    errRet("wrong shared_data status, reg_task_info");

  pthread_mutex_lock(&(share_memory->lock));

  struct preschedule_input_data *pre_data =
      &(share_memory->sched_info.data.pre_data);
  struct preschedule_task_info_array *task_info_arr = &(pre_data->task_infos);

  memcpy(task_info_arr->array + task_info_arr->size, new_func_info,
         sizeof(struct preschedule_task_info));
  *idx_in_shm_task_arr = task_info_arr->size;
  ++task_info_arr->size;

  if (dep && (*dep).data[0] != '\0') {
    struct preschedule_dependency_array *dep_arr = &(pre_data->dependencies);
    memcpy(dep_arr->array + dep_arr->size, dep,
           sizeof(struct preschedule_tts_deadline));
    ++dep_arr->size;
  }

  if (tts_deadline && (*tts_deadline).data[0] != '\0') {
    struct preschedule_tts_deadline_array *tts_d_arr =
        &(pre_data->tts_deadlines);
    memcpy(tts_d_arr->array + tts_d_arr->size, tts_deadline,
           sizeof(struct preschedule_tts_deadline));
    ++tts_d_arr->size;
  }

  pthread_mutex_unlock(&(share_memory->lock));
  return 0;
}

int enable_scheduler(int max_parallel) {
  if (max_parallel <= 0) errRet("invalid input params, enable_scheduler");
  if (share_memory->sched_info.current_status > 1)
    errRet("wrong status, enable_scheduler");
  pthread_mutex_lock(&(share_memory->lock));
  share_memory->sched_info.current_status = 1;
  int ret = preprocess(&(share_memory->sched_info.data));
  if (ret == 0) {
    struct cpu *cpu = &(share_memory->sched_info.data.ready_data.cpu);
    init_cpu(cpu, max_parallel);
    share_memory->sched_info.data.ready_data.max_parallel = max_parallel;
    share_memory->sched_info.current_status = 2; /* input done*/
  }
  pthread_mutex_unlock(&(share_memory->lock));
  return ret;
}
