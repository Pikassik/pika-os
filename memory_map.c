#include "memory_map.h"
#include "panic.h"
#include "utils.h"
#include "multiboot_info.h"

#include <stdint.h>
#include <stddef.h>

typedef struct memory_map
{
  uint32_t size;
  uint32_t base_addr_low;
  uint32_t base_addr_high;
  uint32_t length_low;
  uint32_t length_high;
  uint32_t type;
} __attribute__((packed)) memory_map_t;

typedef struct SMAP_entry {

  uint32_t BaseL; // base address uint64_t
  uint32_t BaseH;
  uint32_t LengthL; // length uint64_t
  uint32_t LengthH;
  uint32_t Type; // entry Type
  uint32_t ACPI; // extended

}__attribute__((packed)) SMAP_entry_t;

#define ENTRY_SIZE 1024
static int entry_count = 0;
static memory_map_t mmap_entries[ENTRY_SIZE];
static const char* segment_types_labels[] = {
  NULL,
  "Usable (normal) RAM",
  "Reserved - unusable",
  "ACPI reclaimable memory",
  "ACPI NVS memory",
  "Area containing bad memory"
};


void detect_memory() {
  if ((multiboot_info->flags & (1 << 6)) == 0) {
    panic("multiboot doesn't support memory info");
  }

  printf_("Memory mapping:\n");
  printf_("Base Address       | Length             | Type\n");

  for (memory_map_t* mmap = (memory_map_t *) multiboot_info->mmap_addr;
       (uint32_t) mmap <
           (multiboot_info->mmap_addr + multiboot_info->mmap_length);
       mmap = (memory_map_t *) ((int32_t) mmap
         + mmap->size + sizeof (mmap->size))) {

    mmap_entries[entry_count] = *mmap;

    printf_("0x%08x%08x | 0x%08x%08x | %s (%d)\n",
      mmap->base_addr_high, mmap->base_addr_low,
      mmap->length_high, mmap->length_low,
      segment_types_labels[mmap->type], mmap->type
    );

    ++entry_count;
  }
}