/*usar git push -u origin trabalho-m3-simplefat*/
#include "task.h"
#include "scheduler.h"
#include "memory.h"
#include "fs.h"

extern void uart_print(const char*);
extern void uart_print_uint(uint64_t);

static void print_status(const char *msg, int value)
{
    uart_print(msg);
    uart_print(": ");

    if (value >= 0) {
        uart_print("OK");
        if (value > 0) {
            uart_print(" (");
            uart_print_uint((uint64_t)value);
            uart_print(")");
        }
    } else {
        uart_print("ERRO");
    }

    uart_print("\n");
}

static void clear_buffer(char *buffer, unsigned int size)
{
    for (unsigned int i = 0; i < size; i++) {
        buffer[i] = 0;
    }
}

void simplefat_task(void)
{
    char buffer[64];
    int fd;
    int result;

    uart_print("\n=== Teste do SimpleFAT ===\n");

    result = fs_init();
    print_status("Inicializacao do sistema de arquivos", result);

    result = fs_create("notas.txt");
    print_status("Criacao de notas.txt", result);

    fd = fs_open("notas.txt");
    print_status("Abertura de notas.txt", fd);

    result = fs_write(fd, "Bem-vindo ao SimpleFAT", 22);
    print_status("Escrita em notas.txt", result);

    clear_buffer(buffer, sizeof(buffer));
    result = fs_read(fd, buffer, 22);
    print_status("Leitura de notas.txt", result);
    uart_print("Conteudo lido: ");
    uart_print(buffer);
    uart_print("\n");

    result = fs_close(fd);
    print_status("Fechamento de notas.txt", result);

    result = fs_delete("notas.txt");
    print_status("Remocao de notas.txt", result);

    result = fs_create("novo.txt");
    print_status("Criacao de novo.txt para testar reutilizacao", result);

    fd = fs_open("novo.txt");
    print_status("Abertura de novo.txt", fd);

    result = fs_write(fd, "Espaco reaproveitado", 20);
    print_status("Escrita em novo.txt", result);

    clear_buffer(buffer, sizeof(buffer));
    result = fs_read(fd, buffer, 20);
    print_status("Leitura de novo.txt", result);
    uart_print("Conteudo lido: ");
    uart_print(buffer);
    uart_print("\n");

    fs_close(fd);
    fs_debug_dump();

    uart_print("\nTeste finalizado. O kernel continuara ativo.\n");

    while (1) {
        yield();
    }
}

void kernel_main(void)
{
    memory_init();

    uart_print("\n=== Kernel com SimpleFAT ===\n");
    uart_print("Memoria livre antes da task: ");
    uart_print_uint(memory_free());
    uart_print(" bytes\n");

    xTaskCreate(simplefat_task, 2048, 1);
    scheduler_start();

    while (1) {
    }
}
