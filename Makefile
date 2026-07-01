CROSS ?= riscv64-unknown-elf-

CFLAGS = -march=rv64gc -mabi=lp64 \
    -mcmodel=medany \
    -msmall-data-limit=0 \
    -ffreestanding \
    -nostdlib -nostartfiles \
    -fno-stack-protector \
    -Wall -Wextra -Iinclude

OBJS = start.o trap_entry.o context.o \
	main.o task.o scheduler.o uart.o string.o memory.o \
	fs.o block.o

all:
	$(CROSS)gcc $(CFLAGS) -c boot/start.S
	$(CROSS)gcc $(CFLAGS) -c boot/trap_entry.S
	$(CROSS)gcc $(CFLAGS) -c kernel/context.S
	$(CROSS)gcc $(CFLAGS) -c kernel/main.c
	$(CROSS)gcc $(CFLAGS) -c kernel/task.c
	$(CROSS)gcc $(CFLAGS) -c kernel/scheduler.c
	$(CROSS)gcc $(CFLAGS) -c kernel/uart.c
	$(CROSS)gcc $(CFLAGS) -c kernel/string.c
	$(CROSS)gcc $(CFLAGS) -c kernel/memory.c
	$(CROSS)gcc $(CFLAGS) -c kernel/fs.c
	$(CROSS)gcc $(CFLAGS) -c drivers/block.c
	$(CROSS)ld -T linker.ld $(OBJS) -o kernel.elf

run: all
	qemu-system-riscv64 -machine virt -m 128M -nographic -bios default -kernel kernel.elf

clean:
	rm -f *.o kernel.elf
