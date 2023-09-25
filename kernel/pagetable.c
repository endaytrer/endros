#include "pagetable.h"
#include "printk.h"

pfn_t pfn_start = 0;
pfn_t pt_base;
static pte_t *pgdir = (pte_t *)0x1000;

FreeNode *freelist = NULL;
void *kbrk;

pfn_t palloc() {
    return pfn_start++;
}

void *kalloc(uint64_t size) {
    uint64_t pages = ADDR_2_PAGEUP(size);

    FreeNode *p = freelist, *prev = NULL;
    while (p != NULL) {
        if (p->size > pages) {
            FreeNode *newp = (FreeNode *)((uint64_t)p + pages * PAGESIZE);
            newp->next = p->next;
            newp->size = (p->size - pages);
            prev->next = newp;
            memset(p, 0, sizeof(FreeNode));
            return p;
        } else if (p->size == pages) {
            prev->next = p->next;
            memset(p, 0, sizeof(FreeNode));
            return p;
        }
        prev = p;
        p = p->next;
    }
    void *ans = kbrk;
    for (uint64_t i = 0; i < pages; i++) {
        pfn_t pfn = palloc();
        ptmap(ADDR_2_PAGE(kbrk), pfn, PTE_VALID | PTE_READ | PTE_WRITE);
        kbrk = (void *)((uint64_t)kbrk + PAGESIZE);
    }
    return ans;
}
void kfree(void *ptr, uint64_t size) {
    uint64_t pages = ADDR_2_PAGEUP(size);
    // lazy approach, leave fragments
    FreeNode *free_head = (FreeNode *)ptr;
    free_head->size = size;
    free_head->next = freelist;
    freelist = free_head;
}

void ptmap(vpn_t vpn, pfn_t pfn, uint64_t flags) {
    // level 2
    pte_t *ptable = pgdir;
    pte_t *pte = (pte_t *)(((uint64_t) ptable) | (VPN(2, vpn) << 3));

    if (!(*pte & PTE_VALID)) {
        pfn_t temp_pfn = palloc();
        // we have to temporally set r+w flags, since we are goint to modify its children.
        *pte = PTE(temp_pfn, PTE_VALID | PTE_READ | PTE_WRITE);
    } else {
        SET_FLAGS(pte, PTE_VALID | PTE_READ | PTE_WRITE);
    }

    // level 1
    ptable = (pte_t *)(VPN(2, vpn) << 12);
    pte_t *new_pte = (pte_t *)(((uint64_t) ptable) | (VPN(1, vpn) << 3));
    if (!(*new_pte & PTE_VALID)) {
        pfn_t temp_pfn = palloc();
        *new_pte = PTE(temp_pfn, PTE_VALID | PTE_READ | PTE_WRITE);
    } else {
        SET_FLAGS(pte, PTE_VALID | PTE_READ | PTE_WRITE);
    }
    SET_FLAGS(pte, PTE_VALID);
    pte = new_pte;

    // level 0
    ptable = (pte_t *)((VPN(2, vpn) << 21) | (VPN(1, vpn) << 12));
    new_pte = (pte_t *)(((uint64_t) ptable) | (VPN(0, vpn) << 3));
    *new_pte = PTE(pfn, flags);
    SET_FLAGS(pte, PTE_VALID);
}

void ptunmap(vpn_t vpn) {
    // level 2
    pte_t *ptable = pgdir;
    pte_t *pte = (pte_t *)(((uint64_t) ptable) | (VPN(2, vpn) << 3));

    if (!(*pte & PTE_VALID)) {
        panic("Cannot unmap unmapped page\n");
    } else {
        SET_FLAGS(pte, PTE_VALID | PTE_READ | PTE_WRITE);
    }

    // level 1
    ptable = (pte_t *)(VPN(2, vpn) << 12);
    pte_t *new_pte = (pte_t *)(((uint64_t) ptable) | (VPN(1, vpn) << 3));
    if (!(*new_pte & PTE_VALID)) {
        panic("Cannot unmap unmapped page\n");
    } else {
        SET_FLAGS(pte, PTE_VALID | PTE_READ | PTE_WRITE);
    }
    SET_FLAGS(pte, PTE_VALID);
    pte = new_pte;

    // level 0
    ptable = (pte_t *)((VPN(2, vpn) << 21) | (VPN(1, vpn) << 12));
    new_pte = (pte_t *)(((uint64_t) ptable) | (VPN(0, vpn) << 3));
    SET_FLAGS(new_pte, 0);
    SET_FLAGS(pte, PTE_VALID);
    
}

void ptmap_physical(vpn_t vpn, pfn_t pfn, uint64_t flags) {
    pte_t *pte;
    pfn_t temp_pfn = pt_base;
    for (int level = 2; level > 0; level--) {
        pte = (pte_t *)((uint64_t)PAGE_2_ADDR(temp_pfn) | (VPN(level, vpn) << 3));
        if (!(*pte & PTE_VALID)) {
            temp_pfn = palloc();
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
    kbrk = ekernel;
    pfn_start = ADDR_2_PAGE(ekernel);
    // map recursive page table
    pt_base = palloc();

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