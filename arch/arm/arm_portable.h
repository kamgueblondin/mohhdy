/* arm_portable.h - Lot 69 (US-076): self-tests of more portable Mohhdy
 * code compiled unchanged for ARMv7-A: kernel/aes_gcm.c (the mesh and TLS
 * cipher), kernel/edid.c (screen size, US-080) and userspace/p2p_flow.c +
 * p2p_pace.c (per-peer pacer, US-068/073). Expected values were computed
 * with the same sources on i386/x86-64, so a pass shows the two
 * architectures agree bit for bit. Pure code; also run on the host by
 * tests/unit/kernel/test_arm_portable.c. */
#ifndef ARM_PORTABLE_H
#define ARM_PORTABLE_H
#include <stdint.h>

#define ARM_PORT_AES_GCM 1U
#define ARM_PORT_EDID    2U
#define ARM_PORT_FLOW    4U
#define ARM_PORT_RTX     8U  /* Lot 71: userspace/p2p_rtx.c */
#define ARM_PORT_CHARSET 16U /* Lot 71: include/os_charset.h */
#define ARM_PORT_AEAD    32U /* Lot 72: userspace/wf_aead.c */
#define ARM_PORT_PAGES   64U /* Lot 72: glyph pages, include/os_charset.h */
#define ARM_PORT_BWE     128U /* Lot 84: userspace/p2p_bwe.c late-ack sample */
#define ARM_PORT_MARKS   256U /* Lot 85: Vietnamese composed letters, os_marks.h */
#define ARM_PORT_TREND   512U /* Lot 87: userspace/p2p_trend.c overuse and cap */
#define ARM_PORT_CEA     1024U /* Lot 87: kernel/edid.c CEA-861 extension */
#define ARM_PORT_ALL     2047U

int arm_port_aes_gcm(void);   /* 1 ok */
int arm_port_edid(void);
int arm_port_flow(void);
int arm_port_rtx(void);
int arm_port_charset(void);
int arm_port_aead(void);
int arm_port_pages(void);
int arm_port_bwe(void);
int arm_port_marks(void);
int arm_port_trend(void);
int arm_port_cea(void);
/* bit mask of the checks that passed */
uint32_t arm_port_selftest(void);
#endif
