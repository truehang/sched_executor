#include "deterministic_scheduler.hpp"

int main() {
  ScheduleClient dc;
  /* print scheduler information every 4 seconds*/
  dc.printShm(4);
  return 0;
}