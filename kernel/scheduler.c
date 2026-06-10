#include "scheduler.h"
#include "task.h"

extern void context_switch(void*, void*);

static int current = 0;

/*   Round-Robin padrão   */

static int round_robin()
{
    return (current + 1) % task_count;
}

/*   Algoritmo atual   */

static sched_algo_t current_algo = round_robin;

void scheduler_set_algorithm(sched_algo_t algo)
{
    if (algo)
        current_algo = algo;
}

/*   Yield   */

void yield()
{
    int prev = current;
    int next = current_algo();

    current = next;

    context_switch(tasks[prev].regs,
                   tasks[next].regs);
}

/* Escalonamento Preemptivo via Timer (A nova mágica)   */
void schedule_from_trap(uint64_t *frame)
{
    int prev = current;
    int next = current_algo();

    /* 1. Copiar frame (trap_entry) -> tasks[prev].regs */
    for (int i = 0; i < 31; i++) {
        tasks[prev].regs[i] = frame[i];
    }

    /* 2. Salvar o sepc da task atual */
    uint64_t prev_pc;
    asm volatile("csrr %0, sepc" : "=r" (prev_pc));
    tasks[prev].pc = prev_pc;

    /* 3. Troca a tarefa ativa */
    current = next;

    /* 4. Copiar tasks[next].regs -> frame (trap_entry) */
    for (int i = 0; i < 31; i++) {
        frame[i] = tasks[next].regs[i];
    }

    uint64_t next_pc = tasks[next].pc;
    if (next_pc == 0) {
        next_pc = (uint64_t)tasks[next].entry;
    }
    
    /* 5. Restaurar o sepc da próxima task */
    asm volatile("csrw sepc, %0" : : "r" (next_pc));
}
 
/*   Início   */

void scheduler_start()
{
    if (task_count == 0)
        return;

    tasks[0].entry();
}