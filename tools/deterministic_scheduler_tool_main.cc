#include <iostream> /* printf*/

#include "deterministic_scheduler.hpp" /* ScheduleClient*/
extern "C" {
#include <unistd.h> /*getopt*/
}

int main(int argc, char *argv[]) {
  if (argc == 1) {
    printf(
        "Usage: %s [-p] |[-e]\n -p\tprint information in share memory\n "
        "-e\tenable scheduler\n",
        argv[0]);
    return 0;
  }
  int opt;
  int once_done = 0;
  ScheduleClient dc;
  while ((opt = getopt(argc, argv, "pe")) != -1) {
    switch (opt) {
      case 'p':
        once_done = 1;
        /* print scheduler information every 4 seconds*/
        dc.printShm(4);
        break;
      case 'e':
        once_done = 1;
        /* enable scheduler*/
        dc.enableScheduler();
        break;
      default: /* '?' */
        printf(
            "Usage: %s[-p] |[-e]\n -p\tprint information in share memory\n "
            "-e\tenable scheduler\n",
            argv[0]);
        break;
    }
    if (once_done) break;
  }

  return 0;
}