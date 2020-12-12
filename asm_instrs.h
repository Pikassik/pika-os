#pragma once

/**
 * @brief disable interrupts
 */
static inline void cli() {
  asm volatile ("cli\n\t");
}

/**
 * @brief enable interrupts
 */
static inline void sti() {
  asm volatile ("sti\n\t");
}

static inline void hlt() {
  asm volatile ("hlt\n\t");
}

static inline void outb(uint16_t port, uint8_t data) {
  asm volatile("out %0,%1" : : "a" (data), "d" (port));
}

static inline uint8_t inb(uint16_t port) {
  uint8_t data;
  asm volatile("in %1, %0" :"=a" (data) : "d" (port) :);
  return data;
}

static inline uint32_t read_cr2() {
    uint32_t val = 0;
    asm volatile (
        "mov %%cr2, %0\n"
        : "=r"(val)
    );
    return val;
}

static inline uint32_t read_cr3() {
    uint32_t val = 0;
    asm volatile (
        "mov %%cr3, %0\n"
        : "=r"(val)
    );
    return val;
}

static inline void load_cr3(uint32_t* pgdir) {
    asm volatile (
        "mov %0, %%cr3\n"
        :
        : "r"(pgdir)
        : "memory"
    );
}