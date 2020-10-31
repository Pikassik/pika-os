#pragma once

struct iframe;

__attribute__ ((interrupt)) void timer_isr(struct iframe* frame);
__attribute__ ((interrupt)) void keyboard_isr(struct iframe* frame);
__attribute__ ((interrupt)) void syscall_entry(struct iframe* frame);
__attribute__ ((interrupt)) void dummy_isr(struct iframe* frame);
__attribute__ ((interrupt)) void spurious_isr(struct iframe* frame);