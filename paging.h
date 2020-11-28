#pragma once

#include <stddef.h>
#include <stdint.h>

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

#define ALLOC_USER (1 << 0)
#define ALLOC_KERNEL (1 << 1)
#define ALLOC_WRITABLE (1 << 2)

extern char KERNEL_HIGH[];
extern char KERNEL_END[];
extern char KERNEL_START[];
extern char USERSPACE_START[];

void* virt2phys(void* addr);

void* phys2virt(void* addr);



void init_kalloc_early();
void* kalloc(size_t size);
void kfree(void* ptr, size_t size);
void init_kernel_paging();
void identity_map(void* addr, size_t sz);
