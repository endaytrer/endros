#include <string.h>
#include "drivers/virtio.h"
#include "printk.h"
#include "pagetable.h"
#include "machine_spec.h"

extern void ekernel();
pfn_t pfn_start;
pfn_t pt_base;
static pte_t *kpgdir = (pte_t *)0x1000;

FreeNode *vfreelist = NULL;
FreeNode *pfreelist = NULL;

// managed memory space.

/*
Kernel Memory Layout:
    0x0000 0000 0000 0000 - 0x0000 0000 7fff ffff     Page table recursive mapping
    0x0000 0000 8000 0000 - 0x0000 0000 801f ffff     (Reserved)
    0x0000 0000 8020 0000 - 0x0000 0000 ffff ffff     kernel sections
    0x0000 0001 0000 0000 - 0x0000 003f ffff ffff     unmanaged mapping space, with manual physical frame allocation, for managing user ptbases
    0xffff ffe0 0000 0000 - 0xffff ffff ffff efff     managed mapping space, cannot free physical frames once allocated
    0xffff ffff ffff f000 - 0xffff ffff ffff ffff     trampoline
*/

void *kbrk = (void *)0xffffffe000000000;

void *ptbrk = (void *)0x100000000;
void *pttop = (void *)0x4000000000;


// unmanaged memory space is from 0x0 to 0x100000000, using palloc

pfn_t palloc() {
    // Allocate one physical page to void. Use this for pagetables.
    if (pfreelist) {
        pfn_t pfn = pfreelist->pfn;
        FreeNode *next = pfreelist->next;
        ptunmap(ADDR_2_PAGE(pfreelist));
        pfreelist = next;
        return pfn;
    }
    if (pfn_start >= max_pfn) {
        panic("[kernel] cannot allocate more pages\n");
    }
    char buf[64];
    printk(itoa(pfn_start, buf, 16));
    printk("\n");
    return pfn_start++;
}

pfn_t palloc_ptr(vpn_t vpn, u64 flags) {
    // Allocate one physical page to virtual page vpn. One have to manage the space once allocated.
    if (pfreelist) {
        pfn_t pfn = pfreelist->pfn;
        FreeNode *next = pfreelist->next;
        ptunmap(ADDR_2_PAGE(pfreelist));
        pfreelist = next;
        ptmap(vpn, pfn, flags);
        memset((void *)PAGE_2_ADDR(vpn), 0, PAGESIZE);
        return pfn;
    }
    if (pfn_start >= max_pfn) {
        panic("[kernel] cannot allocate more pages\n");
    }
    pfn_t pfn = pfn_start++;
    ptmap(vpn, pfn, flags);
    memset((void *)PAGE_2_ADDR(vpn), 0, PAGESIZE);
    return pfn;
}

void pfree(pfn_t pfn, vpn_t vpn) {
    FreeNode *temp = pfreelist;
    pfreelist = (FreeNode *)PAGE_2_ADDR(vpn);
    pfreelist->pfn = pfn;
    pfreelist->next = temp;
}

FreeNode *ptfreelist = NULL;

pfn_t dmalloc(vpn_t *out_vpn, u64 pages, bool zeroing) {
    // dmalloc/dmafree and uptalloc/uptfree also shares ptfreelist freelist:
    // They are all sizeable, physically allocatable and contiguous.
    FreeNode *p = ptfreelist, *prev = NULL;
    while (p != NULL) {
        if (p->size > pages) {
            FreeNode *newp = (FreeNode *)((u64)p + pages * PAGESIZE);
            newp->next = p->next;
            newp->size = (p->size - pages);
            newp->pfn = (p->pfn + pages);
            if (prev)
                prev->next = newp;
            else
                ptfreelist = newp;
            pfn_t pfn = p->pfn;
            if (zeroing)
                memset(p, 0, pages * PAGESIZE);
            *out_vpn = ADDR_2_PAGE(p);
            return pfn;

        } else if (p->size == pages) {
            if (prev)
                prev->next = p->next;
            else
                ptfreelist = p->next;
            pfn_t pfn = p->pfn;
            if (zeroing)
                memset(p, 0, pages * PAGESIZE);
            *out_vpn = ADDR_2_PAGE(p);
            return pfn;
        }
        prev = p;
        p = p->next;
    }
    // if there is only one page to allocate, reuse freed pages by palloc_ptr();

    void *ans = ptbrk;
    vpn_t vpn = ADDR_2_PAGE(ptbrk);
    *out_vpn = vpn;
    ptbrk = (void *)((u64)ptbrk + pages * PAGESIZE);
    if (pages == 1) 
        return palloc_ptr(vpn, PTE_VALID | PTE_READ | PTE_WRITE);
    
    if (pfn_start + pages > max_pfn)
        panic("[kernel] cannot allocate more pages\n");
    
    pfn_t pfn = pfn_start;
    pfn_start += pages;
    // the pages need to be claimed before using ptmap.
    for (u64 i = 0; i < pages; i++) {
        ptmap(vpn + i, pfn + i, PTE_VALID | PTE_READ | PTE_WRITE);
    }
    if (zeroing)
        memset(ans, 0, pages * PAGESIZE);
    return pfn;
}

void dmafree(pfn_t pfn, vpn_t vpn, u64 pages) {
    // lazy approach, leave fragments
    FreeNode *free_head = (FreeNode *)PAGE_2_ADDR(vpn);
    free_head->size = pages;
    free_head->pfn = pfn;
    free_head->next = ptfreelist;
    ptfreelist = free_head;
}


void *kalloc(u64 size) {
    // Although not recommended, the caller may use more than size.
    // so when freeing, always zero (pages * PAGESIZE) bytes.
    if (size == 0) return NULL;
    u64 pages = ADDR_2_PAGEUP(size);

    FreeNode *p = vfreelist, *prev = NULL;
    while (p != NULL) {
        if (p->size > pages) {
            FreeNode *newp = (FreeNode *)((u64)p + pages * PAGESIZE);
            newp->next = p->next;
            newp->size = (p->size - pages);
            if (prev)
                prev->next = newp;
            else
                vfreelist = newp;
            memset(p, 0, pages * PAGESIZE);
            return p;
        } else if (p->size == pages) {
            if (prev)
                prev->next = p->next;
            else
                vfreelist = p->next;
            memset(p, 0, pages * PAGESIZE);
            return p;
        }
        prev = p;
        p = p->next;
    }
    void *ans = kbrk;
    for (u64 i = 0; i < pages; i++) {
        palloc_ptr(ADDR_2_PAGE(kbrk), PTE_VALID | PTE_READ | PTE_WRITE);
        // discarding pfn returned, thus unable to reallocate.
        kbrk = (void *)((u64)kbrk + PAGESIZE);
    }
    memset(ans, 0, pages * PAGESIZE);
    return ans;
}

void kfree(void *ptr, u64 size) {
    if (size == 0) return;
    u64 pages = ADDR_2_PAGEUP(size);
    // lazy approach, leave fragments
    FreeNode *free_head = (FreeNode *)ptr;
    free_head->size = pages;
    free_head->next = vfreelist;
    vfreelist = free_head;
}
vpn_t walkupt(const PTReference_2 *ptref_base, vpn_t user_vpn) {
    /* walk in user page table
        return the page referenced by kernel vpn
     */
    const PTReference_2 *ptref2 = ptref_base + VPN(2, user_vpn);

    if (!ptref2->ptable) {
        // lookup failed
        return 0;
    }

    const PTReference_1 *ptref1 = ptref2->pt_ref + VPN(1, user_vpn);

    if (!ptref1->ptable) {
        // lookup failed
        return 0;
    }

    const vpn_t *ptref0 = ptref1->pt_ref + VPN(0, user_vpn);
    return *ptref0;

}
void uptmap(vpn_t uptbase, PTReference_2 *ptref_base, vpn_t kernel_vpn, vpn_t user_vpn, pfn_t pfn, u64 flags) {
    /* map in user page table
       if kernel_vpn is 0, the page cannot be referenced by kernel
     */

    pte_t *pte = (pte_t *)ADDR(uptbase, VPN(2, user_vpn) << 3);
    PTReference_2 *ptref2 = ptref_base + VPN(2, user_vpn);

    if (!ptref2->ptable) {
        vpn_t temp_vpn;
        // it need to be zeroed because may cause pagetable errors
        pfn_t temp_pfn = dmalloc(&temp_vpn, 1, true);
        PTReference_1 *next_ptref = kalloc(2 * PAGESIZE);

        *pte = PTE(temp_pfn, PTE_VALID);
        ptref2->ptable = (pte_t *)PAGE_2_ADDR(temp_vpn);
        ptref2->pt_ref = next_ptref;
    }

    pte = (pte_t *)((u64)ptref2->ptable | (VPN(1, user_vpn) << 3));
    PTReference_1 *ptref1 = ptref2->pt_ref + VPN(1, user_vpn);

    if (!ptref1->ptable) {
        vpn_t temp_vpn;
        pfn_t temp_pfn = dmalloc(&temp_vpn, 1, true);
        vpn_t *next_ptref = kalloc(PAGESIZE);

        *pte = PTE(temp_pfn, PTE_VALID);
        ptref1->ptable = (pte_t *)PAGE_2_ADDR(temp_vpn);
        ptref1->pt_ref = next_ptref;
    }

    pte = (pte_t *)((u64)ptref1->ptable | (VPN(0, user_vpn) << 3));
    vpn_t *ptref0 = ptref1->pt_ref + VPN(0, user_vpn);
    *pte = PTE(pfn, flags);
    *ptref0 = kernel_vpn;
}

void uptunmap(vpn_t uptbase, PTReference_2 *ptref_base, vpn_t user_vpn) {
    // unmap in user page table, auto freeing the page;
    // uiptbase is a page table, but using virtual addresses, to track lower level page tables

    pte_t *pte = (pte_t *)ADDR(uptbase, VPN(2, user_vpn) << 3);
    PTReference_2 *ptref2 = ptref_base + VPN(2, user_vpn);

    if (!ptref2->ptable) {
        panic("Cannot unmap unmapped pages\n");
    }

    pte = (pte_t *)((u64)ptref2->ptable | (VPN(1, user_vpn) << 3));
    PTReference_1 *ptref1 = ptref2->pt_ref + VPN(1, user_vpn);

    if (!ptref1->ptable) {
        panic("Cannot unmap unmapped pages\n");
    }

    pte = (pte_t *)((u64)ptref1->ptable | (VPN(0, user_vpn) << 3));
    vpn_t *ptref0 = ptref1->pt_ref + VPN(0, user_vpn);
    pfn_t pfn = GET_PFN(pte);
    if (*ptref0) {
        dmafree(pfn, *ptref0, 1);
    }
    *pte = 0;

    // cannot modify ptref0, since it needs to be managed by allocator.
}
void ptref_free(pfn_t ptbase_pfn, vpn_t ptbase_vpn, PTReference_2 *ptref_base) {
    for (u64 i = 0; i < PAGESIZE / sizeof(pte_t); i++) {
        pte_t *pte2 = (pte_t *)ADDR(ptbase_vpn, i << 3);
        PTReference_2 *ptref2 = ptref_base + i;
        if (!ptref2->ptable) continue;

        for (u64 j = 0; j < PAGESIZE / sizeof(pte_t); j++) {
            pte_t *pte1 = (pte_t *)((u64)ptref2->ptable | (j << 3));
            PTReference_1 *ptref1 = ptref2->pt_ref + j;
            if (!ptref1->ptable) continue;
            
            for (u64 k = 0; k < PAGESIZE / sizeof(pte_t); k++) {
                pte_t *pte0 = (pte_t *)((u64)ptref1->ptable | (k << 3));
                vpn_t *ptref0 = ptref1->pt_ref + k;
                if (!*ptref0) continue;
                // freeing allocated page
                dmafree(GET_PFN(pte0), *ptref0, 1);
            }
            // freeing page table level 0, and vpn table
            dmafree(GET_PFN(pte1), ADDR_2_PAGE(ptref1->ptable), 1);
            kfree(ptref1->pt_ref, PAGESIZE);
        }
        // freeing page table level 1, and ptref_1 table
        dmafree(GET_PFN(pte2), ADDR_2_PAGE(ptref2->ptable), 1);
        kfree(ptref2->pt_ref, PAGESIZE * 2);
    }
    // freeing ptref and ptbase
    dmafree(ptbase_pfn, ptbase_vpn, 1);
    kfree(ptref_base, PAGESIZE * 2);
}

void ptref_copy(pfn_t dst_ptbase_pfn, vpn_t dst_ptbase_vpn, PTReference_2 *dst_ptref_base, pfn_t src_ptbase_pfn, vpn_t src_ptbase_vpn, PTReference_2 *src_ptref_base) {

    // copy pagetable. copy-on-write NOT IMPLEMENTED
    for (u64 i = 0; i < PAGESIZE / sizeof(pte_t); i++) {
        pte_t *dst_pte2 = (pte_t *)ADDR(dst_ptbase_vpn, i << 3);
        PTReference_2 *src_ptref2 = src_ptref_base + i;
        PTReference_2 *dst_ptref2 = dst_ptref_base + i;

        if (!src_ptref2->ptable) continue;

        // dst pagetable is empty, so always create pagetable in this step;
        vpn_t kernel_vpn_2;
        pfn_t pfn_2 = dmalloc(&kernel_vpn_2, 1, true);
        dst_ptref2->pt_ref = kalloc(PAGESIZE * 2);
        dst_ptref2->ptable = (pte_t *)PAGE_2_ADDR(kernel_vpn_2);
        *dst_pte2 = PTE(pfn_2, PTE_VALID);

        for (u64 j = 0; j < PAGESIZE / sizeof(pte_t); j++) {
            pte_t *dst_pte1 = (pte_t *)((u64)dst_ptref2->ptable | (j << 3));
            PTReference_1 *src_ptref1 = src_ptref2->pt_ref + j;
            PTReference_1 *dst_ptref1 = dst_ptref2->pt_ref + j;
            if (!src_ptref1->ptable) continue;


            // dst pagetable is empty, so always create pagetable in this step;
            vpn_t kernel_vpn_1;
            pfn_t pfn_1 = dmalloc(&kernel_vpn_1, 1, true);
            dst_ptref1->pt_ref = kalloc(PAGESIZE);
            dst_ptref1->ptable = (pte_t *)PAGE_2_ADDR(kernel_vpn_1);
            *dst_pte1 = PTE(pfn_1, PTE_VALID);
            
            for (u64 k = 0; k < PAGESIZE / sizeof(pte_t); k++) {
                pte_t *src_pte0 = (pte_t *)((u64)src_ptref1->ptable | (k << 3));
                pte_t *dst_pte0 = (pte_t *)((u64)dst_ptref1->ptable | (k << 3));
                vpn_t *src_ptref0 = src_ptref1->pt_ref + k;
                vpn_t *dst_ptref0 = dst_ptref1->pt_ref + k;

                if (!*src_ptref0) continue;

                // creating page and corresponding level-0 page entry;
                vpn_t kernel_vpn;
                // dont need to zero because it is then mem-copied.
                pfn_t pfn = dmalloc(&kernel_vpn, 1, false);
                *dst_pte0 = PTE(pfn, GET_FLAGS(src_pte0));
                *dst_ptref0 = kernel_vpn;

                // copying from src space to dst space;
                memcpy((void *)PAGE_2_ADDR(*dst_ptref0), (void *)PAGE_2_ADDR(*src_ptref0), PAGESIZE);
            }
        }
    }
}

void ptmap(vpn_t vpn, pfn_t pfn, u64 flags) {
    // level 2
    pte_t *ptable = kpgdir;
    pte_t *pte = (pte_t *)(((u64) ptable) | (VPN(2, vpn) << 3));

    if (!(*pte & PTE_VALID)) {
        pfn_t temp_pfn = palloc();
        // we have to temporally set r+w flags, since we are goint to modify its children.
        *pte = PTE(temp_pfn, PTE_VALID | PTE_READ | PTE_WRITE);

        // after palloc, we have to clean allocated page.
        // accessing the page is 000 000 VPN(2) offset
        memset((void *)PAGE_2_ADDR(VPN(2, vpn)), 0, PAGESIZE);
        // accessing
    } else {
        SET_FLAGS(pte, PTE_VALID | PTE_READ | PTE_WRITE);
    }

    // level 1
    ptable = (pte_t *)PAGE_2_ADDR(VPN(2, vpn));
    pte_t *new_pte = (pte_t *)(((u64) ptable) | (VPN(1, vpn) << 3));
    if (!(*new_pte & PTE_VALID)) {
        pfn_t temp_pfn = palloc();
        *new_pte = PTE(temp_pfn, PTE_VALID | PTE_READ | PTE_WRITE);

        // after palloc, we have to clean allocated page.
        // accessing the page is 000 VPN(2) VPN(1) offset
        SET_FLAGS(pte, PTE_VALID);
        memset((void *)PAGE_2_ADDR((VPN(2, vpn) << 9) | VPN(1, vpn)), 0, PAGESIZE);
    } else {
        SET_FLAGS(new_pte, PTE_VALID | PTE_READ | PTE_WRITE);
        SET_FLAGS(pte, PTE_VALID);
    }
    pte = new_pte;

    // level 0
    ptable = (pte_t *)PAGE_2_ADDR((VPN(2, vpn) << 9) | VPN(1, vpn));
    new_pte = (pte_t *)(((u64) ptable) | (VPN(0, vpn) << 3));
    *new_pte = PTE(pfn, flags);
    SET_FLAGS(pte, PTE_VALID);
}

pfn_t ptunmap(vpn_t vpn) {
    // level 2
    pte_t *ptable = kpgdir;
    pte_t *pte = (pte_t *)(((u64) ptable) | (VPN(2, vpn) << 3));

    if (!(*pte & PTE_VALID)) {
        panic("Cannot unmap unmapped page\n");
    } else {
        SET_FLAGS(pte, PTE_VALID | PTE_READ | PTE_WRITE);
    }

    // level 1
    ptable = (pte_t *)(VPN(2, vpn) << 12);
    pte_t *new_pte = (pte_t *)(((u64) ptable) | (VPN(1, vpn) << 3));
    if (!(*new_pte & PTE_VALID)) {
        panic("Cannot unmap unmapped page\n");
    } else {
        SET_FLAGS(new_pte, PTE_VALID | PTE_READ | PTE_WRITE);
    }
    SET_FLAGS(pte, PTE_VALID);
    pte = new_pte;

    // level 0
    ptable = (pte_t *)((VPN(2, vpn) << 21) | (VPN(1, vpn) << 12));
    new_pte = (pte_t *)(((u64) ptable) | (VPN(0, vpn) << 3));
    pfn_t pfn = GET_PFN(new_pte);
    SET_FLAGS(new_pte, 0);
    SET_FLAGS(pte, PTE_VALID);
    return pfn;
}

void ptmap_physical(vpn_t vpn, pfn_t pfn, u64 flags) {
    pte_t *pte;
    pfn_t temp_pfn = pt_base;
    for (int level = 2; level > 0; level--) {
        pte = (pte_t *)ADDR(temp_pfn, VPN(level, vpn) << 3);
        if (!(*pte & PTE_VALID)) {
            temp_pfn = palloc();
            memset((void *)PAGE_2_ADDR(temp_pfn), 0, PAGESIZE);
            *pte = PTE(temp_pfn, PTE_VALID);
        } else {
            temp_pfn = GET_PFN(pte);
        }
    }
    pte = (pte_t *)ADDR(temp_pfn, VPN(0, vpn) << 3);
    *pte = PTE(pfn, flags);
}


void init_pagetable(void) {

    extern void stext();
    extern void strampoline();

    extern void etext();
    extern void srodata();
    extern void erodata();
    extern void sdata();
    extern void ebss();

    // map recursive page table
    pfn_start = ADDR_2_PAGE(ekernel);
    pt_base = palloc();
    memset((void *)PAGE_2_ADDR(pt_base), 0, PAGESIZE);

    // making the 0th entry pointing to itself (recursive mapping), and 1st entry pointing to it self with rw permission
    // in this way, first page = 0x000 000 000 xxx, does not have r/w permission
    // page directory = 0x000 000 001 000, with r/w permission, be specially careful about this.
    *(pte_t *)ADDR(pt_base, 0x000) = PTE(pt_base, PTE_VALID);
    *(pte_t *)ADDR(pt_base, 0x008) = PTE(pt_base, PTE_VALID | PTE_READ | PTE_WRITE);
    
    // map kernel code identically with r-x
    char buf[64];
    printk("text size=");
    printk(itoa(ADDR_2_PAGEUP(etext) - ADDR_2_PAGE(stext), buf, 10));
    printk("pages\n");
    for (pfn_t pfn = ADDR_2_PAGE(stext); pfn < ADDR_2_PAGEUP(etext); ++pfn) {
        ptmap_physical(pfn, pfn, PTE_VALID | PTE_READ | PTE_EXECUTE);
    }
    // map kernel rodata identically with r--

    printk("\nrodata size=");
    printk(itoa(ADDR_2_PAGEUP(erodata) - ADDR_2_PAGE(srodata), buf, 10));
    printk("pages\n");
    for (pfn_t pfn = ADDR_2_PAGE(srodata); pfn < ADDR_2_PAGEUP(erodata); ++pfn) {
        ptmap_physical(pfn, pfn, PTE_VALID | PTE_READ);
    }
    // map kernel data and bss identically with rw-

    printk("\ndata / bss size=");
    printk(itoa(ADDR_2_PAGEUP(ebss) - ADDR_2_PAGE(sdata), buf, 10));
    printk("pages\n");
    for (pfn_t pfn = ADDR_2_PAGE(sdata); pfn < ADDR_2_PAGEUP(ebss); ++pfn) {
        ptmap_physical(pfn, pfn, PTE_VALID | PTE_READ | PTE_WRITE);
    }
    // map trampoline with r-x

    printk("\ntrapoline:\n");
    ptmap_physical(ADDR_2_PAGE(TRAMPOLINE), ADDR_2_PAGE(strampoline), PTE_VALID | PTE_READ | PTE_EXECUTE);

    // identical map virtio devices
    for (int i = 0; i < num_virtio_mmio; i++) {
        ptmap_physical(ADDR_2_PAGE(virtio_mmio_headers[i]), ADDR_2_PAGE(virtio_mmio_headers[i]), PTE_VALID | PTE_READ | PTE_WRITE);
    }

    printk("[kernel] activating paging...\n");
    // activate paging
    register u64 token = ((u64)1 << 63) | pt_base;

    asm volatile(
        "csrw satp, %0\n"
        "sfence.vma"
        :: "r" (token)
    );
    printk("[kernel] activation success\n");
}