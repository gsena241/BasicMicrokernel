#include "timer.h"

static uint64_t tick_interval = 100000;

/* Implementacao da chamada SBI set_timer utilizando ecall */
static inline void sbi_set_timer(uint64_t stime_value)
{
    asm volatile(
        "mv a0, %0\n"    
        "li a7, 0\n"     
        "ecall"          
        :
        : "r" (stime_value)
        : "a0", "a7", "memory"
    );
}

void timer_next(void)
{
    uint64_t now;
    
    /* Ler o CSR time */
    asm volatile("csrr %0, time" : "=r" (now));
    
    sbi_set_timer(now + tick_interval);
}

void timer_init(uint64_t interval)
{
    if (interval != 0)
        tick_interval = interval;
        
    timer_next();
    
    
    asm volatile("csrs sie, %0" : : "r" (1 << 5));
    

    asm volatile("csrs sstatus, %0" : : "r" (1 << 1));
}