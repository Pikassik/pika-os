#include "detect_memory.h"
#include "panic.h"
#include "utils.h"
#include "multiboot_info.h"
#include "paging.h"

#include <stdint.h>
#include <stddef.h>

#define ENTRY_SIZE 1024
int entry_count = 0;
memory_map_t mmap_entries[ENTRY_SIZE];
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

  memory_map_t* mmap = (memory_map_t *) multiboot_info->mmap_addr;
  for ( ;
       (uint32_t) mmap <
           (multiboot_info->mmap_addr + multiboot_info->mmap_length);
       mmap = (memory_map_t *) ((uint32_t) mmap
         + mmap->size + sizeof (mmap->size))) {
    mmap_entries[entry_count] = *mmap;

    printf_("0x%08x.%08x | 0x%08x.%08x | %s (%d)\n",
      mmap->base_addr_high, mmap->base_addr_low,
      mmap->length_high, mmap->length_low,
      segment_types_labels[mmap->type], mmap->type
    );

    ++entry_count;
  }
}