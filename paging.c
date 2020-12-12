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

#define VIRT2PHYS(addr) (void*)((uint32_t)(addr) - (uint32_t)&KERNEL_HIGH[0])

__attribute__((section(".boot.text"))) void setup_paging_early() {
  // 0x0...4Mb identity mapped
  // KERNEL_HIGH...KERNEL_HIGH+4Mb mapped to 0x0...4Mb
  early_pgdir[0] = PT_PRESENT | PT_WRITEABLE | PT_PAGE_SIZE;
  early_pgdir[PGDIR_IDX(&KERNEL_HIGH)] = PT_PRESENT | PT_WRITEABLE | PT_PAGE_SIZE;
  early_pgdir[PGDIR_IDX(&USERSPACE_START)] = (uint32_t)ROUNDDOWNBIG(VIRT2PHYS(&USERSPACE_START)) | PT_PRESENT | PT_WRITEABLE | PT_PAGE_SIZE;
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
  return (struct physical_segment){};
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
  printf_("available memory: [0x%x:0x%x]\n", start, end);
}

/*!
 * @brief not "threadsafe"
 */
void* kalloc(size_t size) {
  size_t size_in_blocks = (uint32_t)ROUNDUP_SZ(size) / PAGE_SIZE;
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

/*!
 * @brief not "threadsafe"
 */
void kfree(void* ptr, size_t size) {
  size_t size_in_blocks = ROUNDUP_SZ(size) / PAGE_SIZE;
  BUG_ON(ptr < (void*)KERNEL_HIGH);
  struct fl_entry* cur = kalloc_head.freelist_head;
  struct fl_entry* new_node = ptr;
  struct fl_entry* new_node_end = ptr + size_in_blocks * PAGE_SIZE;
  new_node->next = NULL;
  new_node->blocks_cnt = size_in_blocks;
  if (cur == NULL || new_node_end <= cur) {
    kalloc_head.freelist_head = new_node;
    if (new_node_end == cur) {
      new_node->blocks_cnt = size_in_blocks + cur->blocks_cnt;
      new_node->next = cur->next;
    } else {
      new_node->blocks_cnt = size_in_blocks;
      new_node->next = cur;
    }
    return;
  }

  while (cur->next && new_node > cur->next) {
    cur = cur->next;
  }

  BUG_ON(cur->next && cur->next < new_node_end);
  if ((void*)new_node < ((void*)cur + cur->blocks_cnt * PAGE_SIZE)) {
    BUG_ON_REACH();
  }

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
    uint32_t old_next_size = cur->next->blocks_cnt;
    cur->next = cur->next->next;
    cur->blocks_cnt += size_in_blocks + old_next_size;
  } else {
    new_node->next = cur->next;
    new_node->blocks_cnt = size_in_blocks;
    cur->next = new_node;
  }
}

uint32_t* alloc_page(uint32_t* pgdir, void* addr, int user) {
  uint32_t* page_table = NULL;
  if (pgdir[PGDIR_IDX(addr)] & PT_PRESENT) {
    page_table = phys2virt((void*)ROUNDDOWN(pgdir[PGDIR_IDX(addr)]));
  } else {
    page_table = kalloc(PAGE_SIZE);
    if (!page_table) {
      return NULL;
    }
    for (int i = 0; i < 1024; i++) {
      page_table[i] = 0;
    }
  }

  int flags = PT_PRESENT | PT_WRITEABLE;
  if (user) {
    flags |= PT_USER;
  }
  pgdir[PGDIR_IDX(addr)] = ((uint32_t)virt2phys(page_table)) | flags;

  return &page_table[PT_IDX(addr)];
}

void map_continous(uint32_t* pgdir, void* addr, size_t size,
                   void* phys_addr, int flags) {
  addr = (void*)ROUNDDOWN((uint32_t)addr);
  phys_addr = (void*)ROUNDDOWN((uint32_t)phys_addr);
  size = ROUNDUP_SZ(size);

  while (size > 0) {
    uint32_t* pte = alloc_page(pgdir, addr, flags | PT_USER);
    *pte = ((uint32_t)phys_addr) | PT_PRESENT;
    *pte |= flags;
    addr += PAGE_SIZE;
    phys_addr += PAGE_SIZE;
    size -= PAGE_SIZE;
  }
}

/*!
 *  @brief if addr == (void*)-1, then looks for free segment
 */
void* mmap_task(struct task* task, void* addr, void* phys_addr /*may be null*/,
                size_t length, int flags) {
  void* start = ROUNDDOWN(addr);
  void* end = ROUNDUP(addr + length);
  flags = (flags & PT_WRITEABLE) | (flags & PT_USER);

  if (addr == (void*)-1) {
    size_t seg_length =  ROUNDUP_SZ(length);
    start = find_segment(&task->proc_map_list, seg_length);
    if (start == (void*)-1) {
      return (void*)-1;
    }
    end = start + seg_length;
  }

  length = (uint32_t)(end - start);

  if (start >= end ||
      end > (void*)&KERNEL_HIGH[0]) {
    return (void*)-1;
  }

  if (phys_addr) {
    if ((uint32_t)phys_addr % PAGE_SIZE != 0) {
      return (void*)-1;
    }
  } else {
    if ((phys_addr = kalloc(length)) == NULL) {
      return (void*)-1;
    }
    phys_addr = virt2phys(phys_addr);
  }

  int err = 0;

  if ((err = insert_segment(&task->proc_map_list, start, end, flags))) {
    return (void*)-1;
  }

  map_continous(task->pgdir, start, length,
                phys_addr, flags);

  return start;
}

uint32_t* kernel_pgdir = NULL;

void init_kernel_paging() {
  kernel_pgdir = kalloc(PAGE_SIZE);
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
  map_continous(kernel_pgdir, addr, sz, addr, PT_WRITEABLE);
}

void* virt2phys_from_pgdir(uint32_t* pgdir, void* addr) {
  uint32_t pgdir_idx = PGDIR_IDX(addr);
  uint32_t pt_idx = PT_IDX(addr);

  if (pgdir[pgdir_idx] & PT_PAGE_SIZE) {
    void* page_addr = (void*)(pgdir[pgdir_idx] & PGTABLE_MASK);
    uint32_t offset = (uint32_t)addr & PGDIR_OFFSET;
    return page_addr + offset;
  }

  uint32_t* pt_addr = phys2virt((void*)(pgdir[pgdir_idx] & PGTABLE_MASK));
  return (void*)(pt_addr[pt_idx] & PGTABLE_MASK);
}

int flags_from_pgdir(uint32_t* pgdir, void* addr) {
  uint32_t pgdir_idx = PGDIR_IDX(addr);
  uint32_t pt_idx = PT_IDX(addr);


  uint32_t* pt_addr = phys2virt((void*)(pgdir[pgdir_idx] & PGTABLE_MASK));
  return (pt_addr[pt_idx] - (pt_addr[pt_idx] & PGTABLE_MASK));
}

void free_mapping(uint32_t* pgdir, struct proc_map_segment** proc_map_list) {
  struct proc_map_segment* current = *proc_map_list;
  while (current) {
    void* phys_addr = virt2phys_from_pgdir(pgdir, current->begin);
    size_t len = current->end - current->begin;
    kfree(phys2virt(phys_addr), len);
    current = current->next;
  }

  for (size_t i = 0; i < 512; ++i) {
    if (pgdir[i] & PT_PRESENT) {
      kfree(phys2virt((void*)(pgdir[i] & PGTABLE_MASK)), PAGE_SIZE);
    }
  }
}

/*!
 * @return flags | PTR_IN_USR
 */
int check_ptr_in_userspace(struct task* task, void* addr, size_t sz) {
  void* begin = addr;
  void* end = addr + sz;

  if (end < begin) {
    return 0;
  }

  if (end >= (void*)&KERNEL_HIGH[0] || begin >= (void*)&KERNEL_HIGH[0]) {
    return 0;
  }

  int found = 0;
  struct proc_map_segment* cur = task->proc_map_list;
  void* cur_seg_begin = (void*)-1;
  void* cur_seg_end = (void*)-1;
  uint32_t cur_flags = 0xffffffff;
  while (cur) {
    if (cur->begin != cur_seg_end) {
      cur_seg_begin = cur->begin;
      cur_flags = cur->flags;
    }
    cur_flags &= (uint32_t)cur->flags;
    cur_seg_end = cur->end;

    if (cur_seg_begin <= begin && end <= cur_seg_end) {
      found = 1;
      break;
    }
    cur = cur->next;
  }

  if (!found || cur == NULL) {
    return 0;
  } else {
    return cur_flags | PTR_IN_USR;
  }
}

int check_ptr_from_userspace(struct task* task, void* addr, size_t sz) {
  int flags = check_ptr_in_userspace(task, addr, sz);
  return flags ? 1 : 0;
}

int check_ptr_to_userspace(struct task* task, void* addr, size_t sz) {
  int flags = check_ptr_in_userspace(task, addr, sz);
  return (flags & PT_WRITEABLE) ? 1 : 0;
}
