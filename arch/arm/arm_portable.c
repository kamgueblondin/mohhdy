/* arm_portable.c - Lot 69 (US-076): see arm_portable.h. */
#include "arm_portable.h"
#include "aes_gcm.h"
#include "edid.h"
#include "p2p_flow.h"
#include "p2p_rtx.h"
#include "os_charset.h"

static int same(const uint8_t* a, const uint8_t* b, int n) {
    int i;
    for (i = 0; i < n; i++) if (a[i] != b[i]) return 0;
    return 1;
}

int arm_port_aes_gcm(void) {
    /* reference from the same kernel/aes_gcm.c built for x86-64 */
    static const uint8_t want_ct[20] = { 0xc6,0xdd,0xa1,0x12,0x3b,0xee,0x46,0x25,0xe7,0x31,
                                         0x19,0x1f,0xfe,0x7e,0xea,0xf4,0xd3,0x3b,0xa0,0x7a };
    static const uint8_t want_tag[16] = { 0x19,0xe0,0x75,0x7e,0x4e,0xb3,0x51,0x10,
                                          0x27,0x26,0xb7,0xf3,0xee,0x4e,0xcf,0xe5 };
    static const uint8_t iv[4] = { 1, 2, 3, 4 };
    static const uint8_t nonce[8] = { 0, 0, 0, 0, 0, 0, 0, 7 };
    static const uint8_t aad[8] = { 'm','o','h','h','d','y','-','a' };
    static const uint8_t pt[20] = { 'a','r','m',' ','p','o','r','t','a','b','l','e',' ','g','c','m',' ','t','s','t' };
    uint8_t key[16], ct[20], tag[16], back[20];
    int i;
    for (i = 0; i < 16; i++) key[i] = (uint8_t)(i * 17 + 3);
    if (aes128_gcm_encrypt(key, iv, nonce, aad, 8, pt, 20, ct, tag) != 0) return 0;
    if (!same(ct, want_ct, 20) || !same(tag, want_tag, 16)) return 0;
    if (aes128_gcm_decrypt(key, iv, nonce, aad, 8, ct, 20, tag, back) != 0 || !same(back, pt, 20)) return 0;
    ct[3] ^= 1U;                                   /* tampered: refused */
    return aes128_gcm_decrypt(key, iv, nonce, aad, 8, ct, 20, tag, back) != 0;
}

int arm_port_edid(void) {
    /* the block QEMU's std VGA gives on i386 (RHT, 1280x800, 325x203 mm) */
    static uint8_t b[EDID_BLOCK];
    edid_info_t e;
    uint8_t* d = b + 54;
    unsigned s = 0;
    int i;
    for (i = 0; i < EDID_BLOCK; i++) b[i] = 0;
    for (i = 1; i < 7; i++) b[i] = 0xFFU;
    b[8] = 0x49U; b[9] = 0x14U;                    /* "RHT" */
    b[18] = 1; b[19] = 4; b[21] = 33; b[22] = 20;
    d[0] = 0x10; d[1] = 0x27;
    d[2] = 0x00; d[4] = 0x50;                      /* 1280 */
    d[5] = 0x20; d[7] = 0x30;                      /* 800 */
    d[12] = 0x45; d[13] = 0xCB; d[14] = 0x10;      /* 325 x 203 mm */
    for (i = 0; i < 127; i++) s += b[i];
    b[127] = (uint8_t)((256U - (s & 0xFFU)) & 0xFFU);
    if (edid_parse(b, EDID_BLOCK, &e) != 0 || !e.ok) return 0;
    if (e.mfg[0] != 'R' || e.mfg[1] != 'H' || e.mfg[2] != 'T') return 0;
    if (e.native_w != 1280 || e.native_h != 800 || e.mm_w != 325 || e.mm_h != 203) return 0;
    if (e.diag_tenths != 151) return 0;            /* 15.1 in, as on i386 */
    b[127] ^= 1U;                                  /* bad checksum: refused */
    return edid_parse(b, EDID_BLOCK, &e) != 0 || !e.ok;
}

int arm_port_flow(void) {
    static p2p_flow_t f, rx;
    uint8_t body[1 + 4 * P2P_FLOW_ACK_BATCH];
    int len;
    p2p_flow_init(&f);
    p2p_flow_init(&rx);
    if (p2p_flow_on_send(&f, 7, 1000, 0) != 0) return 0;
    if (p2p_flow_note_rx(&rx, 7) != 0) return 0;
    len = p2p_flow_take_acks(&rx, body, (int)sizeof(body));
    if (len != 5 || p2p_flow_apply_acks(&f, body, len, 20000) != 1) return 0;
    if (f.srtt_us != 20000U || f.btl_bps != 50000U || p2p_flow_rate(&f) != 62500U) return 0;
    p2p_flow_on_send(&f, 8, 1000, 100000);
    if (p2p_flow_expire(&f, 600000) != 1 || f.lost != 1U) return 0;
    return p2p_flow_rate(&f) == 62500U * 7U / 8U;
}

/* Lot 71: a lost copy leaves again under a new counter, the receiver
 * delivers the original once (same sequence as test_p2p_rtx.c). */
int arm_port_rtx(void) {
    static p2p_rtx_t s, r;
    uint8_t f[16];
    uint32_t orig;
    uint8_t inner;
    const uint8_t* body;
    int idx, len, blen;
    p2p_rtx_init(&s);
    p2p_rtx_init(&r);
    if (p2p_rtx_keep(&s, 9, 13, (const uint8_t*)"k=v", 3) != 0) return 0;
    if ((idx = p2p_rtx_lost(&s, 9)) < 0) return 0;
    if ((len = p2p_rtx_frame(&s, idx, f, (int)sizeof(f))) != 8) return 0;
    p2p_rtx_resent(&s, idx, 20);
    if (p2p_rtx_parse(f, len, &orig, &inner, &body, &blen) != 0 || orig != 9U || inner != 13 || blen != 3 || body[2] != 'v') return 0;
    if (p2p_rtx_accept(&r, 9) != 1 || p2p_rtx_accept(&r, 9) != 0) return 0;
    if (p2p_rtx_accept(&r, 300) != 1 || p2p_rtx_accept(&r, 9) != 0 || r.old_rx != 1U) return 0;
    return p2p_rtx_acked(&s, 20) == 1 && s.resent == 1U;
}

/* Lot 71: UTF-8 to the CP1252 display bytes the fonts draw */
int arm_port_charset(void) {
    char t[24] = { 'c','a','f',(char)0xC3,(char)0xA9,' ',(char)0xE2,(char)0x82,(char)0xAC,' ',(char)0xC5,(char)0x93,'u','v','r','e',0 };
    int n = os_text_to_display_inplace(t, (int)sizeof(t));
    return n == 12 && (unsigned char)t[3] == 0xE9U && (unsigned char)t[5] == 0x80U && (unsigned char)t[7] == 0x9CU && t[8] == 'u';
}

uint32_t arm_port_selftest(void) {
    uint32_t m = 0;
    if (arm_port_aes_gcm()) m |= ARM_PORT_AES_GCM;
    if (arm_port_edid()) m |= ARM_PORT_EDID;
    if (arm_port_flow()) m |= ARM_PORT_FLOW;
    if (arm_port_rtx()) m |= ARM_PORT_RTX;
    if (arm_port_charset()) m |= ARM_PORT_CHARSET;
    return m;
}
