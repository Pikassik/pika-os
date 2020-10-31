#include "isr.h"
#include "utils.h"
#include "apic.h"
#include "kbd.h"
#include "terminal.h"

__attribute__ ((interrupt)) void syscall_entry(struct iframe* frame) {
  (void)frame;
  cli();
  printf_("Syscall!\n");
  sti();
}

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

__attribute__ ((interrupt)) void keyboard_isr(struct iframe* frame) {
  (void)frame;
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

static int milliseconds = 0;

__attribute__ ((interrupt)) void timer_isr(struct iframe* frame) {
  (void)frame;
  cli();
  ++milliseconds;
  if (milliseconds % (60 * 1000) == 0) {
    printf_("%d minutes elapsed\n", milliseconds / (60 * 1000));
  }
  apic_eoi();
  sti();
}

__attribute__ ((interrupt)) void dummy_isr(struct iframe* frame) {
  (void)frame;
}

__attribute__ ((interrupt)) void spurious_isr(struct iframe* frame) {
  (void)frame;
  cli();
  apic_eoi();
  sti();
}
