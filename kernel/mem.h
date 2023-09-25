#ifndef _K_MEM_H
#define _K_MEM_H
#include <stdint.h>

#define NULL ((void *)0)
#define PAGESIZE 4096
#define PAGE_2_ADDR(pn) ((void *)((pn) * PAGESIZE))

#define ADDR_2_PAGE(addr) (((uint64_t)(addr)) / PAGESIZE)
#define ADDR_2_PAGEUP(addr) (ADDR_2_PAGE(addr) + (((uint64_t)(addr) & (PAGESIZE - 1)) ? 1 : 0))

typedef uint64_t pfn_t;
typedef uint64_t vpn_t;
#endif