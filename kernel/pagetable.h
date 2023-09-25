#ifndef _K_PAGETABLE_H
#define _K_PAGETABLE_H
#include "mem.h"

#define PTE_DIRTY    0x80
#define PTE_ACCESSED 0x40
#define PTE_G        0x20
#define PTE_USER     0x10
#define PTE_EXECUTE  0x08
#define PTE_WRITE    0x04
#define PTE_READ     0x02
#define PTE_VALID    0x01

#define VPN(level, vpn) ((vpn >> (level * 9)) & 0x1ff)
#define PTE(pfn, flags) ((pfn << 10) | flags)
#define SET_PFN(pte_p, pfn) *pte_p = ((*pte_p & ~(0xfffffffffffffc00)) | (pfn << 10))
#define GET_PFN(pte_p) (*pte_p >> 10)
#define SET_FLAGS(pte_p, flags) *pte_p = ((*pte_p & ~(0x3ff)) | flags)
#define OFFSET(addr) (addr & 0xfff)

typedef uint64_t pte_t;
typedef struct free_node_t {
    uint64_t size;
    struct free_node_t *next;
} FreeNode;


void *kalloc(uint64_t size);
void kfree(void *ptr, uint64_t size);


void pfree(vpn_t vpn, pfn_t pfn);


void ptmap(vpn_t vpn, pfn_t pfn, uint64_t flags);
void ptunmap(vpn_t vpn);
void init_pagetable(void);
#endif