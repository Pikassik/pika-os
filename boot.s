.set ALIGN,    1<<0
.set MEMINFO,  1<<1
.set FLAGS,    ALIGN | MEMINFO
.set MAGIC,    0x1BADB002
.set CHECKSUM, -(MAGIC + FLAGS)

.section .multiboot, "a"
.align 4
.long MAGIC
.long FLAGS
.long CHECKSUM

.section .bss, "aw", @nobits
.align 16
stack_bottom:
.skip 16384
stack_top:
.skip 32
multiboot_info:
    .skip 32

.global multiboot_info

.extern kernel_main
.extern setup_paging_early
.extern early_pgdir

.section .boot.text, "ax"
.global _start_early
.type _start_early, @function
_start_early:
    cli
    mov  $stack_top, %esp
    push %ebx

    call setup_paging_early

    pop %ebx

    mov $early_pgdir, %eax
    mov %eax, %cr3

    # Enable 4 MiB pages.
    mov %cr4, %eax
    or $(1 << 4), %eax
    mov %eax, %cr4



    # Enable paging.
    mov %cr0, %eax
    or $(1 << 31), %eax
    mov %eax, %cr0

    mov %ebx, multiboot_info

    mov $kernel_main, %eax
    jmp *%eax

.size _start_early, . - _start_early
