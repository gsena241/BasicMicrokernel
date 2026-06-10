#include "trap.h"
#include "timer.h"
#include "scheduler.h"
#include "uart.h"

void trap_handler(uint64_t *frame)
{
    uint64_t scause;
    asm volatile("csrr %0, scause" : "=r" (scause));
 
    if (scause == 0x8000000000000005ULL)
    {
        timer_next();
        schedule_from_trap(frame);
    }
    else
    {
        //uart_print("Unhandled trap\n");
        while (1);
    }
}

void uart_print_hex(uint64_t val) {
    char hex[17];
    const char *digits = "0123456789abcdef";
    for (int i = 15; i >= 0; i--) {
        hex[i] = digits[val & 0xF];
        val >>= 4;
    }
    hex[16] = '\0';
    uart_print(hex);
    uart_print("\n");
}