AS=gcc -m32 -c -g -mgeneral-regs-only -mno-red-zone
C_FLAGS=-std=gnu99 -ffreestanding -O2 -Wall -Wextra
CC=gcc -m32 -g -mgeneral-regs-only -mno-red-zone $(C_FLAGS)
LD=gcc -m32

OBJ=interrupts.o \
    boot.o \
    kernel.o \
    terminal.o \
    putchar.o \
    printf.o \
    apic.o \
    panic.o \
    acpi.o \
    string.o \
    isr.o \
    gdt.o \
    load_gdt.o \
    memory_map.o

build:
	$(CC) -c utils/string.c -o string.o
	$(CC) -c gdt.c -o gdt.o
	$(AS) -c gdt.s -o load_gdt.o
	$(CC) -c apic.c -o apic.o
	$(CC) -c panic.c -o panic.o
	$(CC) -c acpi.c -o acpi.o
	$(CC) -c terminal.c -o terminal.o
	$(CC) -c putchar.c -o putchar.o
	$(CC) -c isr.c -o isr.o
	$(CC) -c interrupts.c -o interrupts.o
	$(CC) -c utils/printf.c -o printf.o \
    -D PRINTF_DISABLE_SUPPORT_FLOAT \
    -D PRINTF_DISABLE_SUPPORT_EXPONENTIAL \
    -D PRINTF_DISABLE_SUPPORT_LONG_LONG
	$(CC) -c memory_map.c -o memory_map.o
	$(CC) -c kernel.c -o kernel.o
	$(AS) boot.s -o boot.o
	$(LD) -T linker.ld -o kernel.bin -ffreestanding -O2 -nostdlib $(OBJ) -lgcc

clean:
	rm kernel.bin $(OBJ)

.PHONY: build
.DEFAULT: build