#ifndef DETERMINISTIC_SCHEDULER_USER_SIGNAL_HEADER
#define DETERMINISTIC_SCHEDULER_USER_SIGNAL_HEADER
#include <signal.h> /* sig*/

/* SIGRTMIN ~ SIGRTMIN + 3 is reserved for scheduling capability.
In case of abnormal functioning, use signal starting from SIGRTMIN + 4*/
#define SOURCE_TRIG_SIG_1 (SIGRTMIN + 4)
#define SOURCE_TRIG_SIG_2 (SIGRTMIN + 5)
#define SOURCE_TRIG_SIG_3 (SIGRTMIN + 6)
#define SOURCE_TRIG_SIG_4 (SIGRTMIN + 7)
#define SOURCE_TRIG_SIG_5 (SIGRTMIN + 8)
#define SOURCE_TRIG_SIG_6 (SIGRTMIN + 9)

#endif