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
#define GET_PFN(pte_p) (*(pte_p) >> 10)
#define SET_FLAGS(pte_p, flags) *pte_p = ((*pte_p & ~(0x3ff)) | flags)
#define GET_FLAGS(pte_p) (*(pte_p) & 0x3ff)

typedef u64 pte_t;
typedef struct free_node_t {
    union {
        u64 size;
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
void uptmap(vpn_t uptbase, PTReference_2 *ptref_base, vpn_t kernel_vpn, vpn_t user_vpn, pfn_t pfn, u64 flags);
void uptunmap(vpn_t uptbase, PTReference_2 *ptref_base, vpn_t vpn);
void ptref_free(pfn_t ptbase_pfn, vpn_t ptbase_vpn, PTReference_2 *ptref_base);
void ptref_copy(pfn_t dst_ptbase_pfn, vpn_t dst_ptbase_vpn, PTReference_2 *dst_ptref_base, pfn_t src_ptbase_pfn, vpn_t src_ptbase_vpn, PTReference_2 *src_ptref_base);

// kernel page stuff
pfn_t palloc_ptr(vpn_t vpn, u64 flags);
void pfree(pfn_t pfn, vpn_t vpn);
void ptmap(vpn_t vpn, pfn_t pfn, u64 flags);
pfn_t ptunmap(vpn_t vpn);

// managed memory allocation
void *kalloc(u64 size);
void kfree(void *ptr, u64 size);

// page mapping
void init_pagetable(void);
#endif