#pragma once
#include "stdint.h"

struct multiboot_info_t {
  uint32_t flags;
  uint32_t mem_lower;
  uint32_t mem_upper;
  uint32_t boot_device;
  uint32_t cmdline;
  uint32_t mods_count;
  uint32_t mods_addr;
  uint32_t symbols[4];
  uint32_t mmap_length;
  uint32_t mmap_addr;
} __attribute__((packed));

extern struct multiboot_info_t* multiboot_info;