/* Lot 8 (US-076): ARMv7-A short-descriptor helpers for the ARM port.
 * Pure functions (no coprocessor access) so the host unit test can check
 * the bit layout and the user pointer rule. */
#ifndef ARM_MMU_H
#define ARM_MMU_H

#include <stdint.h>

#define ARM_SECTION_SIZE 0x00100000U
#define ARM_KERNEL_BASE 0x40000000U       /* QEMU virt RAM start, kernel section */
#define ARM_USER_BASE 0x40100000U         /* the one user section */
#define ARM_USER_SIZE ARM_SECTION_SIZE

/* Access permissions (AP[1:0], APX = 0). */
#define ARM_AP_PRIV_ONLY 1U               /* PL1 read/write, PL0 no access */
#define ARM_AP_FULL 3U                    /* PL1 and PL0 read/write */

#define ARM_MEM_NORMAL 0U                 /* write-back cacheable */
#define ARM_MEM_DEVICE 1U                 /* shareable device */

uint32_t arm_section_desc(uint32_t pa, uint32_t ap, uint32_t mem, int xn);
uint32_t arm_section_index(uint32_t va);
/* 1 when [ptr, ptr+len) lies inside the user section (no wrap). */
int arm_user_range_ok(uint32_t ptr, uint32_t len);
/* Fault status decoding of DFSR (short descriptor): 1 for a permission
 * fault on a section, 2 for a translation fault, 0 otherwise. */
int arm_dfsr_kind(uint32_t dfsr);

#endif
