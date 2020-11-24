#pragma once

#include <stdint.h>

typedef struct memory_map
{
  uint32_t size;
  uint32_t base_addr_low;
  uint32_t base_addr_high;
  uint32_t length_low;
  uint32_t length_high;
  uint32_t type;
} __attribute__((packed)) memory_map_t;

void detect_memory();

extern int entry_count;
extern memory_map_t mmap_entries[];