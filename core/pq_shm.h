#ifndef PQ_SHM_HEADER
#define PQ_SHM_HEADER

#include <stdint.h>

#include "posix_shm.h"

int pq_insert(struct priority_queue *pq, int index_at_task_array,
              struct task_array *tasks);
int pq_extract(struct priority_queue *pq, struct task_array *tasks);
int pq_remove_key(struct priority_queue *q, int index_at_task_array,
                  struct task_array *tasks);
void pq_dump(struct priority_queue *rq, struct task_array *tasks);

int pq_top(struct priority_queue *rq);

#endif