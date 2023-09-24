#include "pagetable.h"


pfn_t pfn_start = 0;
pfn_t pt_base;

FreeNode *freelist = NULL;


pfn_t palloc_physical() {
    // In virtual space. If pfree is never called, it is also viable in physical space.
    // After calling palloc(), one MUST map the physical page,
    return pfn_start++;
}
pfn_t palloc() {
    if (freelist) {
        pfn_t pfn = freelist->pfn;
        ptunmap(ADDR_2_PAGE(freelist));
        freelist = freelist->next;
        return pfn;
    }
    return pfn_start++;
}

void pfree(vpn_t vpn, pfn_t pfn) {
    // In virtual space. The mapping is left mapped, in order to be remapped.
    FreeNode *free_head = (FreeNode *)PAGE_2_ADDR(vpn);

    free_head->pfn = pfn;
    free_head->next = freelist;
    freelist = free_head;
}

void ptmap(vpn_t vpn, pfn_t pfn, uint64_t flags) {

}

void ptunmap(vpn_t vpn) {

}
void ptmap_physical(vpn_t vpn, pfn_t pfn, uint64_t flags) {
    pte_t *pte;
    pfn_t temp_pfn = pt_base;
    for (int level = 2; level > 0; level--) {
        pte = (pte_t *)((uint64_t)PAGE_2_ADDR(temp_pfn) | (VPN(level, vpn) << 3));
        if (!(*pte & PTE_VALID)) {
            temp_pfn = palloc_physical();
            *pte = PTE(temp_pfn, PTE_VALID);
        } else {
            temp_pfn = GET_PFN(pte);
        }
    }
    pte = (pte_t *)((uint64_t)PAGE_2_ADDR(temp_pfn) | (VPN(0, vpn) << 3));
    *pte = PTE(pfn, flags);
}


void init_pagetable(void) {

    extern void stext();
    extern void etext();
    extern void srodata();
    extern void erodata();
    extern void sdata();
    extern void ebss();

    extern void ekernel();

    pfn_start = ADDR_2_PAGE(ekernel);
    // map recursive page table
    pt_base = palloc_physical();

    // making the 0th entry pointing to itself (recursive mapping), and 1st entry pointing to it self with rw permission
    // in this way, first page = 0x000 000 000 xxx, does not have r/w permission
    // page directory = 0x000 000 001 000, with r/w permission, be specially careful about this.
    *(pte_t *)((uint64_t)PAGE_2_ADDR(pt_base) | 0x000) = PTE(pt_base, PTE_VALID);
    *(pte_t *)((uint64_t)PAGE_2_ADDR(pt_base) | 0x008) = PTE(pt_base, PTE_VALID | PTE_READ | PTE_WRITE);
    
    // map kernel code identically with r-x
    for (pfn_t pfn = ADDR_2_PAGE(stext); pfn < ADDR_2_PAGEUP(etext); ++pfn) {
        ptmap_physical(pfn, pfn, PTE_VALID | PTE_READ | PTE_EXECUTE);
    }
    // map kernel rodata identically with r--
    for (pfn_t pfn = ADDR_2_PAGE(srodata); pfn < ADDR_2_PAGEUP(erodata); ++pfn) {
        ptmap_physical(pfn, pfn, PTE_VALID | PTE_READ);
    }
    // map kernel data and bss identically with rw-
    for (pfn_t pfn = ADDR_2_PAGE(sdata); pfn < ADDR_2_PAGEUP(ebss); ++pfn) {
        ptmap_physical(pfn, pfn, PTE_VALID | PTE_READ | PTE_WRITE);
    }
    // activate paging
    uint64_t token = ((uint64_t)1 << 63) | pt_base;
    asm volatile(
        "csrw satp, %0\n\t"
        "sfence.vma"
        :: "r" (token)
    );
}