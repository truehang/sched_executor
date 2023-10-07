#include "timestamp.h"

#include <stdio.h>
#include <time.h>

#include "common_define.h"

int start_watch(struct watch *wch) {
  if (clock_gettime(CLOCK_MONOTONIC, &(wch->ts_start)) < 0)
    errExit("clock_gettime");
  return 0;
}

int stop_watch(struct watch *wch) {
  if (clock_gettime(CLOCK_MONOTONIC, &(wch->ts_end)) < 0)
    errExit("clock_gettime");
  wch->ts_diff = wch->ts_end.tv_nsec - wch->ts_start.tv_nsec +
                 (wch->ts_end.tv_sec - wch->ts_start.tv_sec) * 1000000000UL;

  return 0;
}

void print_time_diff(struct watch *wch) {
  printf("elapse %ld ns\n", wch->ts_diff);
}

int64_t get_nano_time() {
  struct timespec ts;
  if (clock_gettime(CLOCK_MONOTONIC, &ts) < 0) errExit("clock_gettime");
  return ts.tv_nsec + ts.tv_sec * 1000000000UL;
}
