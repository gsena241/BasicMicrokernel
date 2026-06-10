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

    /* 1. Configura o registrador stvec para apontar para o tratador em Assembly */
    asm volatile("csrw stvec, %0" : : "r" ((uint64_t)trap_entry));

    /* 2. Inicializa o timer do hardware (ex: dispara a cada 100.000 ticks) */
    timer_init(100000);

    /* 3. Cria as tarefas usando a função nativa do microkernel */
    xTaskCreate(task1, 2048, 1);
    xTaskCreate(task2, 2048, 1);

    scheduler_start();

    while (1);
}