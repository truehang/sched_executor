#include "pq_shm.h"

#include "posix_shm.h"

int test1() {
  struct task_array tasks;
  struct priority_queue q;
  tasks.size = 10;
  memcpy(tasks.array[0].name, "A", sizeof("A"));
  tasks.array[0].wtb = 0;

  memcpy(tasks.array[1].name, "B", sizeof("B"));
  tasks.array[1].wtb = -6;

  memcpy(tasks.array[1].name, "C", sizeof("C"));
  tasks.array[2].wtb = 30000000;

  tasks.array[3].wtb = 3;

  tasks.array[4].wtb = 4;

  tasks.array[5].wtb = 5;

  tasks.array[6].wtb = 6;

  tasks.array[7].wtb = 7;

  tasks.array[8].wtb = 8;

  tasks.array[9].wtb = 9;

  memset(&q, 0, sizeof(q));
  pq_insert(&q, 0, &tasks);
  if (q.queue[0] != 0) errRet("fail");

  pq_insert(&q, 1, &tasks);
  if (q.queue[0] != 1) errRet("fail");
  pq_insert(&q, 2, &tasks);
  if (q.queue[0] != 1) errRet("fail");

  pq_extract(&q, &tasks);
  if (q.queue[0] != 0) errRet("fail");

  pq_insert(&q, 5, &tasks);
  if (q.queue[0] != 0) errRet("fail");

  pq_insert(&q, 4, &tasks);
  if (q.queue[0] != 0) errRet("fail");

  pq_insert(&q, 3, &tasks);
  if (q.queue[0] != 0) errRet("fail");

  pq_extract(&q, &tasks);
  if (q.queue[0] != 3) errRet("fail");

  pq_extract(&q, &tasks);
  if (q.queue[0] != 4) errRet("fail");

  pq_insert(&q, 6, &tasks);
  if (q.queue[0] != 4) errRet("fail");

  pq_insert(&q, 7, &tasks);
  if (q.queue[0] != 4) errRet("fail");

  pq_remove_key(&q, 5, &tasks);
  if (q.queue[0] != 4) errRet("fail");

  pq_extract(&q, &tasks);
  if (q.queue[0] != 6) errRet("fail");
  pq_extract(&q, &tasks);
  if (q.queue[0] != 7) errRet("fail");

  pq_remove_key(&q, 7, &tasks);
  if (q.queue[0] != 2) errRet("fail");

  pq_insert(&q, 9, &tasks);
  if (q.queue[0] != 9) errRet("fail");

  pq_insert(&q, 8, &tasks);
  if (q.queue[0] != 8) errRet("fail");

  pq_extract(&q, &tasks);
  if (q.queue[0] != 9) errRet("fail");

  pq_extract(&q, &tasks);
  if (q.queue[0] != 2) errRet("fail");

  if (pq_extract(&q, &tasks) < 0) errRet("fail");

  /*=========================================================*/
  memset(&q, 0, sizeof(q));
  q.max_heap = 1;
  pq_insert(&q, 0, &tasks);
  if (q.queue[0] != 0) errRet("fail");

  pq_insert(&q, 1, &tasks);
  if (q.queue[0] != 0) errRet("fail");
  pq_insert(&q, 2, &tasks);
  if (q.queue[0] != 2) errRet("fail");

  pq_extract(&q, &tasks);
  if (q.queue[0] != 0) errRet("fail");

  pq_insert(&q, 5, &tasks);
  if (q.queue[0] != 5) errRet("fail");

  pq_insert(&q, 4, &tasks);
  if (q.queue[0] != 5) errRet("fail");

  pq_insert(&q, 3, &tasks);
  if (q.queue[0] != 5) errRet("fail");

  pq_extract(&q, &tasks);
  if (q.queue[0] != 4) errRet("fail");

  pq_extract(&q, &tasks);
  if (q.queue[0] != 3) errRet("fail");

  pq_insert(&q, 6, &tasks);
  if (q.queue[0] != 6) errRet("fail");

  pq_insert(&q, 7, &tasks);
  if (q.queue[0] != 7) errRet("fail");

  pq_remove_key(&q, 3, &tasks);
  if (q.queue[0] != 7) errRet("fail");

  pq_extract(&q, &tasks);
  if (q.queue[0] != 6) errRet("fail");
  pq_extract(&q, &tasks);
  if (q.queue[0] != 0) errRet("fail");

  pq_remove_key(&q, 0, &tasks);
  if (q.queue[0] != 1) errRet("fail");

  pq_insert(&q, 9, &tasks);
  if (q.queue[0] != 9) errRet("fail");

  pq_insert(&q, 8, &tasks);
  if (q.queue[0] != 9) errRet("fail");

  pq_extract(&q, &tasks);
  if (q.queue[0] != 8) errRet("fail");

  pq_extract(&q, &tasks);
  if (q.queue[0] != 1) errRet("fail");

  if (pq_extract(&q, &tasks) < 0) errRet("fail");
  printf("test1 success\n");
  return 0;
}

int test2() {
  struct task_array tasks;
  tasks.size = 8;

  memcpy(tasks.array[1].name, "T1", sizeof("T1"));
  tasks.array[1].wtb = 7;
  memcpy(tasks.array[2].name, "T2", sizeof("T2"));
  tasks.array[2].wtb = 6;
  memcpy(tasks.array[3].name, "T3", sizeof("T3"));
  tasks.array[3].wtb = 2;
  memcpy(tasks.array[4].name, "T4", sizeof("T4"));
  tasks.array[4].wtb = 3;
  memcpy(tasks.array[5].name, "T5", sizeof("T5"));
  tasks.array[5].wtb = 4;
  memcpy(tasks.array[6].name, "T6", sizeof("T6"));
  tasks.array[6].wtb = 5;
  memcpy(tasks.array[7].name, "T7", sizeof("T7"));
  tasks.array[7].wtb = 6;

  struct priority_queue RT; /* RT is a max heapq_dump(&RT, &tasks)p*/
  memset(&RT, 0, sizeof(RT));
  RT.max_heap = 1;

  struct priority_queue WQ; /* WQ is a min heap*/
  memset(&WQ, 0, sizeof(WQ));

  /* Insert T1-T2 in RT, T3-T7 in WQ*/
  pq_insert(&RT, 1, &tasks);
  pq_insert(&RT, 2, &tasks);
  pq_insert(&WQ, 3, &tasks);
  pq_insert(&WQ, 4, &tasks);
  pq_insert(&WQ, 5, &tasks);
  pq_insert(&WQ, 6, &tasks);
  pq_insert(&WQ, 7, &tasks);
  pq_dump(&RT, &tasks);
  pq_dump(&WQ, &tasks);

  int cores = 4;
  int preemptive = 1;

  /* LWTBF Scheduling*/
  while (WQ.size > 0) {
    int i = pq_top(&WQ);
    if (RT.size < cores) {
      pq_insert(&RT, i, &tasks);
      pq_extract(&WQ, &tasks);
      /*arrange i to cpu, and binding cpu*/
    } else if (preemptive) {
      int j = pq_top(&RT);
      if (tasks.array[i].wtb < tasks.array[j].wtb) {
        pq_insert(&WQ, j, &tasks);
        pq_extract(&RT, &tasks);
        pq_insert(&RT, i, &tasks);
        pq_extract(&WQ, &tasks);
        /*arrange j to cpu, and binding cpu*/
        /* set i's prior as lowest*/
      } else
        break; /*preemption done*/
    } else
      break; /* Not support preemption*/
  }

  pq_dump(&RT, &tasks);
  pq_dump(&WQ, &tasks);

  if (pq_top(&RT) != 6) errRet("wrong");
  pq_extract(&RT, &tasks);
  if (pq_top(&RT) != 5) errRet("wrong");
  pq_extract(&RT, &tasks);
  if (pq_top(&RT) != 4) errRet("wrong");
  pq_extract(&RT, &tasks);
  if (pq_top(&RT) != 3) errRet("wrong");
  if (pq_extract(&RT, &tasks) < 0) errRet("wrong");

  if (!(pq_top(&WQ) == 2 || pq_top(&WQ) == 7)) errRet("wrong");
  pq_extract(&WQ, &tasks);
  if (!(pq_top(&WQ) == 2 || pq_top(&WQ) == 7)) errRet("wrong");
  pq_extract(&WQ, &tasks);
  if (pq_top(&WQ) != 1) errRet("wrong");
  if (pq_extract(&WQ, &tasks) < 0) errRet("wrong");

  printf("test2 success\n");
  return 0;
}

int main() {
  test1();
  test2();
  return 0;
}