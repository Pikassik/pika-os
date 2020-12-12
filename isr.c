#include "isr.h"
#include "utils.h"
#include "apic.h"
#include "kbd.h"
#include "terminal.h"
#include "panic.h"
#include "sched.h"

uint8_t getchar_from_keyboard() {
  static uint32_t shift;
  static uint8_t *charcode[4] = {
    normalmap, shiftmap, ctlmap, ctlmap
  };
  uint32_t st, data, c;

  st = inb(KBSTATP);
  if ((st & KBS_DIB) == 0) {
    return -1;
  }
  data = inb(KBDATAP);

  if (data == 0xE0) {
    shift |= E0ESC;
    return -1;
  } else if (data & 0x80) {
    // Key released
    data = (shift & E0ESC ? data : data & 0x7F);
    shift &= ~(shiftcode[data] | E0ESC);
    return -1;
  } else if (shift & E0ESC) {
    // Last character was an E0 escape; or with 0x80
    data |= 0x80;
    shift &= ~E0ESC;
  }

  shift |= shiftcode[data];
  shift ^= togglecode[data];
  c = charcode[shift & (CTL | SHIFT)][data];
  if(shift & CAPSLOCK){
    if('a' <= c && c <= 'z')
      c += 'A' - 'a';
    else if('A' <= c && c <= 'Z')
      c += 'a' - 'A';
  }

  return c;
};

void keyboard_irq(struct regs* regs) {
  (void)regs;
  cli();
  uint8_t c = getchar_from_keyboard();
  if (c != (uint8_t)-1) {
    if (c == KEY_UP) {
      terminal_cursor_up();
    } else if (c == KEY_DN) {
      terminal_cursor_down();
    } else {
      printf_("%c", c);
    }
  }
  apic_eoi();
  sti();
}


volatile uint64_t timer_ticks;

void timer_irq(struct regs* regs) {
  (void)regs;
  __sync_fetch_and_add(&timer_ticks, 1);
  apic_eoi();
  scheduler_tick(regs);
}

void dummy_irq(struct regs* regs) {
  (void)regs;
}

void spurious_irq(struct regs* regs) {
  (void)regs;
  apic_eoi();
}


void pagefault_irq(struct regs* regs) {
  (void)regs;
  if (!current_task) {
    printf_("pagefault in kernel :(\n"
            "    cr2=0x%x, eip=0x%x, err_code=%d\n",
        read_cr2(), regs->eip, regs->error_code);
    panic("pagefault :(");
  }

  current_task->exitcode = 0x10000; // "terminated"
  current_task->state = TASK_TERMINATED;
  reschedule();
}
