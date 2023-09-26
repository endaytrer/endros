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

#define TRAMPOLINE ((void *)0xfffffffffffff000)
#define TRAPFRAME ((void *)0xffffffffffffe000)
#define VPN(level, vpn) ((vpn >> (level * 9)) & 0x1ff)
#define PTE(pfn, flags) ((pfn << 10) | flags)
#define SET_PFN(pte_p, pfn) *pte_p = ((*pte_p & ~(0xfffffffffffffc00)) | (pfn << 10))
#define GET_PFN(pte_p) (*(pte_p) >> 10)
#define SET_FLAGS(pte_p, flags) *pte_p = ((*pte_p & ~(0x3ff)) | flags)
#define OFFSET(addr) ((uint64_t)(addr) & 0xfff)

typedef uint64_t pte_t;
typedef struct free_node_t {
    union {
        uint64_t size;
        pfn_t pfn;
    } type;
    struct free_node_t *next;
} FreeNode;



typedef struct {
    pte_t *ptable;
    vpn_t *pt_ref;
} PTReference_1;

typedef struct {
    pte_t *ptable;
    PTReference_1 *pt_ref;
} PTReference_2;

// user page stuff
vpn_t walkupt(PTReference_2 *ptref_base, vpn_t user_vpn);
pfn_t uptalloc(vpn_t *out_vpn);
void uptfree(pfn_t pfn, vpn_t vpn);
void uptmap(vpn_t uptbase, PTReference_2 *ptref_base, vpn_t kernel_vpn, vpn_t user_vpn, pfn_t pfn, uint64_t flags);
void uptunmap(vpn_t uptbase, PTReference_2 *ptref_base, vpn_t vpn);
void ptref_free(pfn_t ptbase_pfn, vpn_t ptbase_vpn, PTReference_2 *ptref_base);

// kernel page stuff
pfn_t palloc_ptr(vpn_t vpn, uint64_t flags);
void pfree(pfn_t pfn, vpn_t vpn);
void ptmap(vpn_t vpn, pfn_t pfn, uint64_t flags);
void ptunmap(vpn_t vpn);

// managed memory allocation
void *kalloc(uint64_t size);
void kfree(void *ptr, uint64_t size);

// page mapping
void init_pagetable(void);
#endif