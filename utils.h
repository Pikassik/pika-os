#pragma once

#include "utils/printf.h"
#include "utils/string.h"
#include "asm_instrs.h"

extern volatile uint64_t timer_ticks;
extern int interrupts_mode;
#define ARRAY_SIZE(x) (sizeof(x)/sizeof(x[0]))
#define INTERRUPTS_MODE_ON()    \
  do {                          \
    interrupts_mode = 1;        \
  } while (0)

#define INTERRUPTS_MODE_OFF()   \
  do {                          \
    interrupts_mode = 0;        \
  } while (0)

#define INTERRUPTS_ON()         \
  do {                          \
    if (interrupts_mode) { \
      sti();                    \
    }                           \
  } while (0)

#define INTERRUPTS_OFF()        \
  do {                          \
    if (interrupts_mode) {      \
      cli();                    \
    }                           \
  } while (0)

#define UTILS____