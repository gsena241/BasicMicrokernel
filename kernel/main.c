#include "task.h"
#include "scheduler.h"
#include "memory.h"
#include "timer.h" 
#include "trap.h"  

extern void uart_print(const char*);

extern void trap_entry(void);

/*   Tasks   */

void task1()
{
    while (1)
    {
        uart_print("Task 1 running\n");

        uart_print("Memory used: ");
        uart_print_uint(memory_used());
        uart_print(" bytes\n");

        uart_print("Memory free: ");
        uart_print_uint(memory_free());
        uart_print(" bytes\n\n");

        for (volatile int i = 0; i < 5000000; i++);
    }
}

void task2()
{
    while (1)
    {
        uart_print("Task 2 running\n");

        uart_print("Memory used: ");
        uart_print_uint(memory_used());
        uart_print(" bytes\n");

        uart_print("Memory free: ");
        uart_print_uint(memory_free());
        uart_print(" bytes\n\n");

        for (volatile int i = 0; i < 5000000; i++);
    }
}

/*   Kernel   */

void kernel_main()
{
    memory_init();   // OBRIGATÓRIO

    uart_print("\n=== Kernel final ===\n");


    asm volatile("csrw stvec, %0" : : "r" ((uint64_t)trap_entry));

    
    timer_init(100000);

    
    xTaskCreate(task1, 2048, 1);
    xTaskCreate(task2, 2048, 1);

    scheduler_start();

    while (1);
}