#ifndef SIGNAL_AGREEMENT_HEADER
#define SIGNAL_AGREEMENT_HEADER
#include <signal.h> /* sig*/

#define OPERATION_SIG (SIGRTMIN)
#define THREAD_WAIT_INIT_SIG (SIGRTMIN + 1)
#define THREAD_ACTIVE_SIG (SIGRTMIN + 2) /* 36*/
#define TIMER_TRIG_SIG (SIGRTMIN + 3)    /* 37*/

#endif