#ifndef SCHEDULER_H
#define SCHEDULER_H

#include <stdint.h>

typedef int (*sched_algo_t)(void);

void scheduler_set_algorithm(sched_algo_t algo);
void scheduler_start(void);
void yield(void);

void schedule_from_trap(uint64_t *frame);

int scheduler_current_task(void);

#endif
