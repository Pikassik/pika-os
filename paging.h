#pragma once

#include "sched.h"
#include "errno.h"

#include <stddef.h>
#include <stdint.h>


#define PGDIR_OFFSET   0x003fffff
#define PGTABLE_OFFSET 0x00000fff
#define PGDIR_MASK     0xffc00000
#define PGTABLE_MASK   0xfffff000
#define BIG_PAGE_SIZE (1 << 22)
#define PAGE_SIZE 4096

#define PT_PRESENT      (1 << 0)
#define PT_WRITEABLE    (1 << 1)
#define PT_USER         (1 << 2)
#define PT_PAGE_SIZE    (1 << 7)

#define PGDIR_IDX(addr) ((((uint32_t)addr) >> 22) & 1023)
#define PT_IDX(addr) ((((uint32_t)addr) >> 12) & 1023)

#define ROUNDUP(addr) ((void*)(((uint32_t)(addr) + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1)))
#define ROUNDDOWN(addr) ((void*)((uint32_t)(addr) & ~(PAGE_SIZE - 1)))
#define ROUNDUPBIG(addr) ((void*)(((uint32_t)(addr) + BIG_PAGE_SIZE - 1) & ~(BIG_PAGE_SIZE - 1)))
#define ROUNDDOWNBIG(addr) ((void*)((uint32_t)(addr) & ~(BIG_PAGE_SIZE - 1)))

#define ROUNDUP_SZ(sz) (uint32_t)ROUNDUP((void*)sz)
#define ROUNDDOWN_SZ(sz) (uint32_t)ROUNDDOWN((void*)sz)
#define ROUNDUPBIG_SZ(sz) (uint32_t)ROUNDUPBIG((void*)sz)
#define ROUNDDOWNBIG_SZ(sz) (uint32_t)ROUNDDOWNBIG((void*)sz)

#define ALLOC_USER (1 << 0)
#define ALLOC_KERNEL (1 << 1)
#define ALLOC_WRITABLE (1 << 2)

extern char KERNEL_HIGH[];
extern char KERNEL_END[];
extern char KERNEL_START[];
extern char USERSPACE_START[];
extern char USERSPACE_END[];

void* virt2phys(void* addr);

void* phys2virt(void* addr);



void init_kalloc_early();
void* kalloc(size_t size);
void kfree(void* ptr, size_t size);
void init_kernel_paging();
void map_continous(uint32_t* pgdir, void* addr, size_t size,
                   void* phys_addr, int flags);

void* mmap_task(struct task* task, void* addr, void* phys_addr /*may be null*/,
                size_t length, int flags);

void identity_map(void* addr, size_t sz);
void* virt2phys_from_pgdir(uint32_t* pgdir, void* addr);
int flags_from_pgdir(uint32_t* pgdir, void* addr);
void free_mapping(uint32_t* pgdir, struct proc_map_segment** proc_map_list);

#define PTR_IN_USR 0x40000000
int check_ptr_in_userspace(struct task* task, void* addr, size_t sz);
int check_ptr_from_userspace(struct task* task, void* addr, size_t sz);
int check_ptr_to_userspace(struct task* task, void* addr, size_t sz);
