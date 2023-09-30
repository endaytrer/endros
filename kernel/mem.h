#ifndef _K_MEM_H
#define _K_MEM_H
#include <type.h>

#define NULL ((void *)0)
#define PAGESIZE 4096
#define PAGE_2_ADDR(pn) ((u64)((pn) * PAGESIZE))

#define ADDR_2_PAGE(addr) (((u64)(addr)) / PAGESIZE)
#define ADDR_2_PAGEUP(addr) (ADDR_2_PAGE(addr) + (((u64)(addr) & (PAGESIZE - 1)) ? 1 : 0))

#define ADDR(pn, offset) ((u64)(((pn) * PAGESIZE) + (offset)))

#define OFFSET(addr) ((u64)(addr) & 0xfff)

#define TRAMPOLINE ((void *)0xfffffffffffff000)
#define TRAPFRAME ((void *)0xffffffffffffe000)
typedef u64 pfn_t;
typedef u64 vpn_t;
#endif