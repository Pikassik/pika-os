#pragma once

//#define inb(port, input) { \
//  asm volatile ( \
//      "inb " ## #port ##"%1, %%al\n\t" \
//      "movb %0, %%al\n\t" \
//    : "=r" (input) \
//    : \
//    : "%eax" \
//  ); \
//}
//
//#define outb(port, value) { \
//  asm volatile ( \
//  "outb %0, $"## #port ##"\n\t" \
//    : \
//    : \
//    : \
//  ); \
//}

inline void cli() {
  asm volatile ("cli\n\t");
}

inline void sti() {
  asm volatile ("sti\n\t");
}