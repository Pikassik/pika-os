#include "paging.h"
#include "utils.h"
#include "panic.h"
#include "detect_memory.h"

#include <stdint.h>

uint32_t early_pgdir[1024] __attribute__((aligned(4096)));

struct fl_entry {
  struct fl_entry* next;
  size_t blocks_cnt;
};

struct kalloc_head {
  struct fl_entry* freelist_head;
};

struct kalloc_head kalloc_head;

__attribute__((section(".boot.text"))) void setup_paging_early() {
  // 0x0...4Mb identity mapped
  // KERNEL_HIGH...KERNEL_HIGH+4Mb mapped to 0x0...4Mb
  early_pgdir[0] = PT_PRESENT | PT_WRITEABLE | PT_PAGE_SIZE;
  early_pgdir[PGDIR_IDX(&KERNEL_HIGH)] = PT_PRESENT | PT_WRITEABLE | PT_PAGE_SIZE;
}

struct physical_segment {
  uint32_t start;
  uint32_t end;
};

uint32_t MMIO_after_memory = 0;

struct physical_segment get_all_memory() {
  int i = 0;
  for (; i < entry_count; ++i) {
    if (mmap_entries[i].type == 1 &&
        mmap_entries[i].base_addr_low >= (1 << 20)) {
      const uint32_t max_len = (uint32_t)0 - (uint32_t)&KERNEL_START[0];
      const uint32_t low_len = mmap_entries[i].length_low;
      uint32_t len = low_len < max_len ? low_len : max_len;
      if (mmap_entries[i].length_high != 0) {
        len = max_len;
      }
      uint32_t start = mmap_entries[i].base_addr_low;
      uint32_t end = start + len;

      MMIO_after_memory =
        (i + 1) < entry_count ? mmap_entries[i + 1].base_addr_low : 0xc0000000;

      return (struct physical_segment){start, end};
    }
  }

  panic("memory not found");
}

void* virt2phys(void* addr) {
  return (void*)((uint32_t)addr - (uint32_t)&KERNEL_HIGH[0]);
}

void* phys2virt(void* addr) {
  return (void*)((uint32_t)addr + (uint32_t)&KERNEL_HIGH[0]);
}

void init_kalloc_early() {
  struct physical_segment all_mem = get_all_memory();
  if (!((phys2virt((void*)all_mem.start) < (void*)&KERNEL_END) &&
        ((void*)&KERNEL_END < phys2virt((void*)all_mem.end)))) {
    panic("unexpected memory mapping");
  }
  void* start = (void*)ROUNDUP((uint32_t)&KERNEL_END);
  kalloc_head.freelist_head = NULL;
  void* end = ROUNDDOWNBIG(phys2virt((void*)all_mem.end));
  if ((uint32_t)end > MMIO_after_memory) {
    end = ROUNDDOWNBIG(MMIO_after_memory);
  }
  struct fl_entry* entry = (struct fl_entry*)start;
  entry->next = NULL;
  kalloc_head.freelist_head = entry;
  kalloc_head.freelist_head->blocks_cnt = (end - start) / PAGE_SIZE;
}

void* kalloc(size_t size) {
  size_t size_in_blocks = (uint32_t)ROUNDUP((void*)size) / PAGE_SIZE;
  struct fl_entry* cur = kalloc_head.freelist_head;
  struct fl_entry** prev_to_cur = &kalloc_head.freelist_head;
  while (cur != NULL) {
    if (cur->blocks_cnt > size_in_blocks) {
      struct fl_entry* new_node = (void*)cur + PAGE_SIZE * size_in_blocks;
      new_node->blocks_cnt = cur->blocks_cnt - size_in_blocks;
      new_node->next = cur->next;
      *prev_to_cur = new_node;
      return cur;
    } else if (cur->blocks_cnt == size_in_blocks) {
      *prev_to_cur = cur->next;
      return cur;
    }
    prev_to_cur = &cur->next;
    cur = cur->next;
  }
  return NULL;
}

void kfree(void* ptr, size_t size) {
  size_t size_in_blocks = (uint32_t)ROUNDUP((void*)size) / PAGE_SIZE;
  struct fl_entry* cur = kalloc_head.freelist_head;
  struct fl_entry* new_node = ptr;
  if (cur == NULL) {
    kalloc_head.freelist_head = new_node;
    new_node->next = NULL;
  }

  while (cur != NULL) {
    if (cur < new_node && (cur->next > new_node || cur->next == NULL)) {
      int concat_left = ((void*)cur + cur->blocks_cnt * PAGE_SIZE) == new_node;
      int concat_right =
        cur->next && ((void*)new_node + size_in_blocks * PAGE_SIZE) == cur->next;
      if (concat_left && !concat_right) {
        cur->blocks_cnt += size_in_blocks;
      } else if (!concat_left && concat_right) {
        new_node->next = cur->next->next;
        new_node->blocks_cnt = cur->next->blocks_cnt + size_in_blocks;
        cur->next = new_node;
      } else if (concat_left && concat_right) {
        cur->next = cur->next->next;
        cur->blocks_cnt += size_in_blocks + cur->next->blocks_cnt;
      } else {
        new_node->next = cur->next;
        new_node->blocks_cnt = size_in_blocks;
        cur->next = new_node;
      }
      return;
    }
    cur->next = new_node;
    new_node->next = cur->next;
  }
}

uint32_t* alloc_page(uint32_t* pgdir, void* addr) {
  uint32_t* page_table = NULL;
  if (pgdir[PGDIR_IDX(addr)] & PT_PRESENT) {
    page_table = phys2virt((void*)ROUNDDOWN(pgdir[PGDIR_IDX(addr)]));
  } else {
    page_table = kalloc(1);
    if (!page_table) {
      return NULL;
    }
    for (int i = 0; i < 1024; i++) {
      page_table[i] = 0;
    }
  }

  pgdir[PGDIR_IDX(addr)] = ((uint32_t)virt2phys(page_table)) | PT_PRESENT
                                                             | PT_WRITEABLE;
  return &page_table[PT_IDX(addr)];
}

void map_continous(uint32_t* pgdir, void* addr, size_t size,
                   void* phys_addr, int writeable) {
  addr = (void*)ROUNDDOWN((uint32_t)addr);
  phys_addr = (void*)ROUNDDOWN((uint32_t)phys_addr);
  size = ROUNDUP(size);

  while (size > 0) {
    uint32_t* pte = alloc_page(pgdir, addr);
    *pte = ((uint32_t)phys_addr) | PT_PRESENT;
    if (writeable) {
      *pte |= PT_WRITEABLE;
    }
    addr += PAGE_SIZE;
    phys_addr += PAGE_SIZE;
    size -= PAGE_SIZE;
  }
}

uint32_t* kernel_pgdir = NULL;

void init_kernel_paging() {
  kernel_pgdir = kalloc(1);
  memset(kernel_pgdir, '\0', PAGE_SIZE);
  for (void* ptr = KERNEL_HIGH;
       ptr < (void*)(MMIO_after_memory - BIG_PAGE_SIZE);
       ptr = ptr + BIG_PAGE_SIZE) {
    kernel_pgdir[PGDIR_IDX(ptr)] =
      (uint32_t)virt2phys((void*)(ptr)) | PT_PRESENT
                                        | PT_WRITEABLE
                                        | PT_PAGE_SIZE;
  }
  load_cr3(virt2phys(kernel_pgdir));
}

void identity_map(void* addr, size_t sz) {
  map_continous(kernel_pgdir, addr, sz, addr, 1);
}
