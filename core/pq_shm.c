#include <string.h>

#include "posix_shm.h"

/* Related action about priority_queue*/
inline static void swap(uint8_t i, uint8_t j, struct priority_queue *pq) {
  uint8_t tmp = pq->queue[i];
  pq->queue[i] = pq->queue[j];
  pq->queue[j] = tmp;
}

inline static task_time_t wtb_at(uint8_t index, struct priority_queue *pq,
                                 struct task_array *tasks) {
  return tasks->array[pq->queue[index]].wtb;
};

/* sink the biggest/smallest among (p, l, r), down to the tail*/
static void heapify(struct priority_queue *pq, uint8_t i,
                    struct task_array *tasks) {
  int size = pq->size;
  while (1) {
    /* i * 2 + 1 is the left (or 1st) child of i;
       i * 2 + 2 is the right (or 2nd) child of i*/
    int l = i * 2 + 1;
    int r = i * 2 + 2;
    int exchange_pos = i;
    if (pq->max_heap) {
      /* i   (if any child > i, then exchange)------->       tail*/
      if (l < size && wtb_at(l, pq, tasks) > wtb_at(i, pq, tasks))
        exchange_pos = l;
      if (r < size && wtb_at(r, pq, tasks) > wtb_at(exchange_pos, pq, tasks))
        exchange_pos = r;

    } else {
      /* i   (if any child < i, then exchange)------->       tail*/
      if (l < size && wtb_at(l, pq, tasks) < wtb_at(i, pq, tasks))
        exchange_pos = l;
      if (r < size && wtb_at(r, pq, tasks) < wtb_at(exchange_pos, pq, tasks))
        exchange_pos = r;
    }
    if (exchange_pos == i) break;
    /* swap i exchange_pos in pq*/
    swap(i, exchange_pos, pq);
    i = exchange_pos;
  }
}

/* return -1 on failure, 0 on success*/
int pq_insert(struct priority_queue *pq, int index_at_task_array,
              struct task_array *tasks) {
  if (pq->size == TASKVOLUME) return -1;
  /* Append index_at_task_array*/
  pq->queue[pq->size] = index_at_task_array;
  int i = pq->size;

  /* Rise the smaller(or bigger) between i and i's parent to the root */
  /* (i - 1) / 2 is the parent of i, short for p(i) */
  if (pq->max_heap) {
    /* root               <----(if i > p(i))     i*/
    while (i > 0 && wtb_at(i, pq, tasks) > wtb_at((i - 1) / 2, pq, tasks)) {
      swap(i, (i - 1) / 2, pq);
      i = (i - 1) / 2;
    }
  } else {
    /* root               <----(if i < p(i))     i*/
    while (i > 0 && wtb_at(i, pq, tasks) < wtb_at((i - 1) / 2, pq, tasks)) {
      swap(i, (i - 1) / 2, pq);
      i = (i - 1) / 2;
    }
  }

  /* After proper process, increase element count in prior_queue*/
  ++pq->size;
  return 0;
}

int pq_extract(struct priority_queue *pq, struct task_array *tasks) {
  if (pq->size == 0) return -1;
  --pq->size;
  pq->queue[0] = pq->queue[pq->size];
  heapify(pq, 0, tasks);
  return 0;
}

int pq_remove_key(struct priority_queue *q, int index_at_task_array,
                  struct task_array *tasks) {
  struct priority_queue tmp;
  memset(&tmp, 0, sizeof(tmp));
  tmp.max_heap = q->max_heap; /* for efficiency benefit*/
  int found = -1;

  /* pop out keys to check whether contains "index_at_task_array",
   use a tmp pq to store these keys*/
  while (q->size > 0) {
    if (q->queue[0] != index_at_task_array) {
      pq_insert(&tmp, q->queue[0], tasks);
      pq_extract(q, tasks);
    } else {
      found = 0;
      break;
    }
  }

  if (found == 0) {
    /* found the key, and remove the key*/
    pq_extract(q, tasks);
  }

  /* re-insert the pop out keys  */
  while (tmp.size > 0) {
    pq_insert(q, tmp.queue[0], tasks);
    pq_extract(&tmp, tasks);
  }

  return found;
}

void pq_dump(struct priority_queue *rq, struct task_array *tasks) {
  if (rq->size == 0) return;
  for (int i = 0; i < rq->size; ++i)
    printf("%s, ", tasks->array[rq->queue[i]].name);
  printf("\n");
}

int pq_top(struct priority_queue *rq) {
  if (rq->size <= 0) return -1;
  return rq->queue[0];
}