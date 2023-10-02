#include <string.h>
#include "fdt.h"
#include "drivers/virtio.h"
#include "machine_spec.h"
/// @brief  A push-down state machine to parse flattened devicetree.
/// @param dtb dtb stored in physical memory


u64 max_pfn;
u32 clock_freq;
u64 nCPU;
u64 num_virtio_mmio;
VirtIOHeader *virtio_mmio_headers[MAX_VIRTIO_MMIO];

i64 parse_dtb(struct fdt_header *dtb) {
    // dtb discovery
    char *name_block = (char *)((u64)dtb + ntohl(dtb->off_dt_strings));
    fdt32_t *fdt_structs = (fdt32_t *)((u64)dtb + ntohl(dtb->off_dt_struct));

    // We care about only:
    // memory stop; (For physical frame allocation) /memory@*, reg[0] + reg[1]
    // nCPU (For parallelism), /cpus, num of subnodes
    // CPU frequency (for clock) (/cpus.timebase_frequency)
    // virtio.mmio devices /soc/virtio_mmio@*

    u32 indent = 0;
    i32 prop_l = 0;
    enum pda_state {
        initial,
        nodename,
        nodeprop,
        proplen,
        propoff,
        propval
    } state = initial;

    char *level_names[8];
    char *prop_name;
    for (u32 i = 0; i < ntohl(dtb->size_dt_struct) / sizeof(fdt32_t); i++) {
        u32 input = ntohl(fdt_structs[i]);

        char name[4];
        switch (state) {
            case initial:
                if (input == FDT_NOP) break;
                if (input == FDT_BEGIN_NODE) {
                    state = nodename;
                    level_names[indent] = (char *)(fdt_structs + i + 1);
                    indent++;
                } else {
                    return -1;
                }
                break;
            case nodename:
                *(u32 *)name = ntohl(input);
                if (name[0] == '\0' || name[1] == '\0' || name[2] == '\0' || name[3] == '\0') {
                    // name parse done.
                    state = nodeprop;
                }
                break;
            case nodeprop:
                if (input == FDT_NOP) break;
                if (input == FDT_PROP) {
                    state = proplen;
                } else if (input == FDT_BEGIN_NODE) {
                    state = nodename;
                    level_names[indent] = (char *)(fdt_structs + i + 1);
                    indent++;
                } else if (input == FDT_END_NODE) {
                    indent--;
                } else if (input == FDT_END) {
                    if (indent != 0)
                        return -1;
                    return 0;
                } else {
                    return -1;
                }
                break;
            case proplen:
                prop_l = input;
                state = propoff;
                break;
            case propoff:
                prop_name = name_block + input;

                // test for target properties
                // max_pfn
                if (indent == 2 &&
                    strncmp(level_names[1], "memory@", 7) == 0 &&
                    strcmp(prop_name, "reg") == 0
                    ) {
                    u64 begin = ntohll(*(fdt64_t *)(fdt_structs + i + 1));
                    u64 size = ntohll(*(fdt64_t *)(fdt_structs + i + 3));
                    max_pfn = ADDR_2_PAGE(begin + size);
                }
                // nCPU
                if (indent > 2 &&
                    strcmp(level_names[1], "cpus") == 0 &&
                    strcmp(level_names[2], "cpu-map") == 0 &&
                    strcmp(prop_name, "cpu") == 0) {
                    nCPU++;
                }
                // CPU Frequency
                if (indent == 2 &&
                    strcmp(level_names[1], "cpus") == 0 &&
                    strcmp(prop_name, "timebase-frequency") == 0) {
                    clock_freq = ntohl(*(fdt32_t *)(fdt_structs + i + 1));
                }
                // virtio-mmio devices
                if (indent == 3 &&
                    strcmp(level_names[1], "soc") == 0 &&
                    strncmp(level_names[2], "virtio_mmio@", 12) == 0 &&
                    strcmp(prop_name, "reg") == 0
                    ) {
                    u64 begin = ntohll(*(fdt64_t *)(fdt_structs + i + 1));
                    virtio_mmio_headers[num_virtio_mmio++] = (VirtIOHeader *)begin;
                }

                if (prop_l != 0)
                    state = propval;
                else {
                    state = nodeprop;
                }
                break;
            case propval:
                *(u32 *)name = ntohl(input);
                int max_l = prop_l > 4 ? 4 : prop_l;
                for (int j = 0; j < max_l; j++) {
                }
                prop_l -= 4;
                if (prop_l <= 0) {
                    state = nodeprop;
                }
                break;
        }
    }
    return 0;
}