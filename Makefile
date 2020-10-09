AS=gcc -m32 -c -g -mgeneral-regs-only -mno-red-zone
CC=gcc -m32 -g -mgeneral-regs-only -mno-red-zone
LD=gcc -m32
BUILD_DIR=build
C_FLAGS=-std=gnu99 -ffreestanding -O2 -Wall -Wextra

OBJ=interrupts.o \
    boot.o \
    kernel.o \
    terminal.o \
    putchar.o \
    printf.o

all: build

build: kernel.bin


clean:
	rm kernel.bin $(OBJ)

putchar.o: putchar.c terminal.o
	$(CC) -c putchar.c -o putchar.o $(C_FLAGS)

printf.o: utils/printf.c terminal.o putchar.o
	$(CC) -c utils/printf.c -o printf.o $(C_FLAGS) \
    -D PRINTF_DISABLE_SUPPORT_FLOAT \
    -D PRINTF_DISABLE_SUPPORT_EXPONENTIAL \
    -D PRINTF_DISABLE_SUPPORT_LONG_LONG

boot.o: boot.s
	$(AS) boot.s -o boot.o

kernel.o: kernel.c terminal.o
	$(CC) -c kernel.c -o kernel.o $(C_FLAGS)

interrupts.o: interrupts.c
	$(CC) -c interrupts.c -o interrupts.o $(C_FLAGS)

terminal.o: terminal.c
	$(CC) -c terminal.c -o terminal.o $(C_FLAGS)

kernel.bin: kernel.o boot.o interrupts.o printf.o
	$(LD) -T linker.ld -o kernel.bin -ffreestanding -O2 -nostdlib $(OBJ) -lgcc

.PHONY: all
