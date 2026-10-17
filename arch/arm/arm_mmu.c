#include "arm_mmu.h"

uint32_t arm_section_desc(uint32_t pa, uint32_t ap, uint32_t mem, int xn) {
    uint32_t d = (pa & 0xFFF00000U) | 0x2U;           /* section */
    d |= (ap & 3U) << 10;                             /* AP[1:0] */
    if (mem == ARM_MEM_DEVICE) d |= 1U << 2;          /* B: shareable device */
    else d |= (1U << 3) | (1U << 2) | (1U << 12);     /* TEX=001 C B: WB-WA */
    if (xn) d |= 1U << 4;
    return d;                                         /* domain 0 */
}

uint32_t arm_section_index(uint32_t va) { return va >> 20; }

int arm_user_range_ok(uint32_t ptr, uint32_t len) {
    if (ptr < ARM_USER_BASE) return 0;
    if (len > ARM_USER_SIZE) return 0;
    if (ptr - ARM_USER_BASE > ARM_USER_SIZE - len) return 0;
    return 1;
}

int arm_dfsr_kind(uint32_t dfsr) {
    uint32_t fs = (dfsr & 0xFU) | ((dfsr >> 6) & 0x10U);
    if (fs == 0x0DU) return 1;                        /* permission, section */
    if (fs == 0x05U) return 2;                        /* translation, section */
    return 0;
}
