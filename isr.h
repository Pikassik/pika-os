#pragma once

#include <stdint.h>

struct iframe;

__attribute__ ((interrupt)) void timer_isr(struct iframe* frame);
__attribute__ ((interrupt)) void keyboard_isr(struct iframe* frame);
__attribute__ ((interrupt)) void syscall_entry(struct iframe* frame);
__attribute__ ((interrupt)) void dummy_isr(struct iframe* frame);
__attribute__ ((interrupt)) void spurious_isr(struct iframe* frame);
__attribute__ ((interrupt)) void pagefault_isr(struct iframe* frame,
                                               uint32_t error_code);