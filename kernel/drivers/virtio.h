#ifndef _K_VIRTIO_H
#define _K_VIRTIO_H

#include <type.h>
#include "../mem.h"
#define VIRTIO0 ((VirtIOHeader *)0x10001000)

#define VIRTIO_VERSION_LEGACY 0x1
#define VIRTIO_VERSION_MODERN 0x2

#define VIRTIO_STATUS_ACKNOWLEDGE 0x1
#define VIRTIO_STATUS_DRIVER 0x2
#define VIRTIO_STATUS_FAILED 0x80
#define VIRTIO_STATUS_FEATURES_OK 0x8
#define VIRTIO_STATUS_DRIVER_OK 0x4
#define VIRTIO_STATUS_DEVICE_NEEDS_RESET 0x40

/**
 * MMIO VirtIO Header
*/
typedef volatile struct {
    /// READONLY Magic value = 0x74726976
    const u32 magic;

    /// READONLY Device version number
    ///
    /// Legacy device returns value 0x1.
    const u32 version;
    
    /// READONLY Virtio Subsystem Device ID
    const u32 device_id;

    /// READONLY Virtio Subsystem Vendor ID
    const u32 vendor_id;

    /// READONLY Flags representing features the device supports
    const u32 device_features;

    /// WRITEONLY Device (host) features word selection
    u32 device_features_sel;
    
    const u32 __reserved_1[2];

    /// WRITEONLY Flags representing device features understood and activated by the driver
    u32 driver_features;

    /// WRITEONLY Activated (guest) features word selection
    u32 driver_features_sel;

    /// WRITEONLY Guest page size
    ///
    /// The driver writes the guest page size in bytes to the register during
    /// initialization, before any queues are used. This value should be a
    /// power of 2 and is used by the device to calculate the Guest address
    /// of the first queue page (see QueuePFN).
    u32 legacy_guest_page_size;

    const u32 __reserved_2;

    /// WRITEONLY Virtual queue index
    ///
    /// Writing to this register selects the virtual queue that the following
    /// operations on the QueueNumMax, QueueNum, QueueAlign and QueuePFN
    /// registers apply to. The index number of the first queue is zero (0x0).
    u32 queue_sel;

    /// READONLY Maximum virtual queue size
    ///
    /// Reading from the register returns the maximum size of the queue the
    /// device is ready to process or zero (0x0) if the queue is not available.
    /// This applies to the queue selected by writing to QueueSel and is
    /// allowed only when QueuePFN is set to zero (0x0), so when the queue is
    /// not actively used.
    const u32 queue_num_max;

    /// WRITEONLY Virtual queue size
    ///
    /// Queue size is the number of elements in the queue. Writing to this
    /// register notifies the device what size of the queue the driver will use.
    /// This applies to the queue selected by writing to QueueSel.
    u32 queue_num;

    /// WRITEONLY Used Ring alignment in the virtual queue
    ///
    /// Writing to this register notifies the device about alignment boundary
    /// of the Used Ring in bytes. This value should be a power of 2 and
    /// applies to the queue selected by writing to QueueSel.
    u32 legacy_queue_align;

    /// READWRITE Guest physical page number of the virtual queue
    ///
    /// Writing to this register notifies the device about location of the
    /// virtual queue in the Guest’s physical address space. This value is
    /// the index number of a page starting with the queue Descriptor Table.
    /// Value zero (0x0) means physical address zero (0x00000000) and is illegal.
    /// When the driver stops using the queue it writes zero (0x0) to this
    /// register. Reading from this register returns the currently used page
    /// number of the queue, therefore a value other than zero (0x0) means that
    /// the queue is in use. Both read and write accesses apply to the queue
    /// selected by writing to QueueSel.
    u32 legacy_queue_pfn;

    /// READWRITE new interface only
    u32 queue_ready;

    /// Reserved
    const u32 __reserved_3[2];

    /// WRITEONLY Queue notifier
    u32 queue_notify;

    /// Reserved
    const u32 __reserved_4[3];

    /// READONLY Interrupt status
    const u32 interrupt_status;

    /// WRITEONLY Interrupt acknowledge
    u32 interrupt_ack;

    /// Reserved
    const u32 __reserved_5[2];

    /// READWRITE Device status
    ///
    /// Reading from this register returns the current device status flags.
    /// Writing non-zero values to this register sets the status flags,
    /// indicating the OS/driver progress. Writing zero (0x0) to this register
    /// triggers a device reset. The device sets QueuePFN to zero (0x0) for
    /// all queues in the device. Also see 3.1 Device Initialization.
    u32 status;

    /// Reserved
    const u32 __reserved_6[3];

    /// WRITEONLY
    u32 queue_desc_low;
    /// WRITEONLY
    u32 queue_desc_high;

    /// Reserved
    const u32 __reserved_7[2];

    /// WRITEONLY
    u32 queue_driver_low;
    /// WRITEONLY
    u32 queue_driver_high;

    /// Reserved
    const u32 __reserved_8[2];

    // WRITEONLY
    u32 queue_device_low;
    // WRITEONLY
    u32 queue_device_high;

    /// Reserved
   const u32 reserved_9[21];

    /// READONLY
    const u32 config_generation;
} VirtIOHeader;


#define VIRTIO_NUM_DESC 8
#define VRING_DESC_F_NEXT  1 // chained with another descriptor
#define VRING_DESC_F_WRITE 2 // device writes (vs read)

typedef volatile struct {
    u64 addr;
    u32 len;
    u16 flags;
    u16 next;
} VirtQueueDesc;

typedef volatile struct {
    u16 flags;
    u16 idx;
    u16 ring[VIRTIO_NUM_DESC];
    u16 used_event;
} VirtQueueAvail;

typedef volatile struct {
    u16 flags;
    u16 idx;
    struct used_elm {
        u32 id;
        u32 len;
    } ring[VIRTIO_NUM_DESC];
    u16 avail_event;
} VirtQueueUsed;

/**
 * VirtIO Queue
*/

struct dma_t {
    pfn_t pfn;
    vpn_t vpn;
    u64 pages;
};
typedef struct {
    u16 queue_idx;
    u16 num_used;
    u16 free_head;
    VirtQueueDesc *desc;
    VirtQueueAvail *avail;
    VirtQueueUsed *used;

    VirtQueueDesc desc_shadow[VIRTIO_NUM_DESC];
    u16 avail_idx;
    u16 last_used_idx;
    bool event_idx;
    bool indirect;
    VirtQueueDesc *indirect_lists[VIRTIO_NUM_DESC];
    union {
        struct {
            struct dma_t dma;
            u64 avail_offset;
            u64 used_offset;
        } legacy;
        struct {
            struct dma_t driver_to_device_dma;
            struct dma_t device_to_driver_dma;
            u64 avail_offset;
        } modern;
    } layout;
} VirtIOQueue;

// utils for queue

bool queue_used(VirtIOHeader *hdr, u16 idx);
void init_queue(VirtIOQueue *queue, VirtIOHeader *hdr, u16 idx, bool indirect, bool event_idx);

/**
 * headers init
 * 
 * returning negotiated features of VirtIO Header;
*/
u32 header_init(VirtIOHeader *hdr, u32 supported_features);
/**
 * finish init
*/
void finish_init(VirtIOHeader *hdr);
#endif