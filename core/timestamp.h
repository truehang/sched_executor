#ifndef TIMESTAMP_HEADER
#define TIMESTAMP_HEADER
#include <stdint.h> /* int64_t*/
#include <time.h>   /* for timestamp, link with -lrt*/

struct watch {
  struct timespec ts_start;
  struct timespec ts_end;
  int64_t ts_diff;
};

int start_watch(struct watch *wch);

int stop_watch(struct watch *wch);

void print_time_diff(struct watch *wch);

int64_t get_nano_time();
#endif