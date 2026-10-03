/* Kernel page-table sharing for task page directories (pure logic, unit
 * tested in tests/unit/kernel/test_vmm_share.c).
 *
 * A task page directory copies the kernel directory's page-directory
 * entries when it is created (task.c task_static_vmm_acquire). A kernel
 * mapping added later in a NEW page table of the kernel directory (e.g. the
 * VBE linear framebuffer mapped by gfx_fb when the desktop starts) is then
 * missing from every directory created before it. Code that touches that
 * mapping with such a directory loaded (IRQ0 redrawing the cursor while a
 * boot worker runs) page-faults in Ring 0. vmm_share_kernel_tables() copies
 * the missing kernel tables into one directory. */
#include "vmm.h"

static int vmm_share_is_private(const vmm_directory_t* dir, uint32_t table_index) {
    return (int)((dir->private_table_mask[table_index / 32U] >> (table_index % 32U)) & 1U);
}

uint32_t vmm_share_kernel_tables(vmm_directory_t* dst, const vmm_directory_t* src,
                                 uint32_t first, uint32_t count) {
    uint32_t i, shared = 0U;
    if (!dst || !src || dst == src || !dst->tables || !src->tables ||
        !dst->physical_dir || !src->physical_dir || first >= ENTRIES_PER_TABLE)
        return 0U;
    if (count > ENTRIES_PER_TABLE - first) count = ENTRIES_PER_TABLE - first;
    for (i = first; i < first + count; i++) {
        /* Only fill holes: a private (user) table or a table the directory
         * already has is never replaced. */
        if (!src->tables[i] || dst->tables[i] || vmm_share_is_private(dst, i)) continue;
        dst->tables[i] = src->tables[i];
        dst->physical_dir->tablesPhysical[i] = src->physical_dir->tablesPhysical[i];
        shared++;
    }
    return shared;
}
