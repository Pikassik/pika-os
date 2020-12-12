#pragma once

inline void cli() {
  asm volatile ("cli\n\t");
}

inline void sti() {
  asm volatile ("sti\n\t");
}