C_FLAGS=-std=gnu99 -ffreestanding -O2 -Wall -Wextra
AS=gcc -m32 -c -g -mgeneral-regs-only
CC=gcc -m32 -g -mgeneral-regs-only -mno-red-zone -std=gnu99 -ffreestanding -fno-pie -Wall -Wextra -O2
LD=gcc -m32 -fno-pic
OBJCOPY=objcopy

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
    detect_memory.o \
    paging.o

image: build
	mkdir -p isodir/boot/grub
	cp grub.cfg isodir/boot/grub
	cp kernel.bin isodir/boot && grub-mkrescue -o kernel.iso isodir
	rm -rf isodir

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
	$(CC) -c detect_memory.c -o detect_memory.o
	$(CC) -c kernel.c -o kernel.o
	$(CC) -c paging.c -o paging.o
	$(AS) boot.s -o boot.o
	$(LD) -T linker.ld -o kernel.bin -ffreestanding -O2 -nostdlib $(OBJ) -lgcc
	$(OBJCOPY) --only-keep-debug kernel.bin kernel.sym
	$(OBJCOPY) --strip-debug kernel.bin

clean:
	rm kernel.bin $(OBJ)

.PHONY: build
.DEFAULT: build
