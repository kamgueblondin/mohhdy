/* arm_portable.c - Lot 69 (US-076): see arm_portable.h. */
#include "arm_portable.h"
#include "aes_gcm.h"
#include "edid.h"
#include "p2p_trend.h"
#include "p2p_flow.h"
#include "p2p_rtx.h"
#include "p2p_bwe.h"
#include "os_charset.h"
#include "wf_aead.h"
#include "wf_regdom.h"
#include "osui_frame.h"
#include "plat_form.h"
#include "p2p_shape.h"

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

/* Lot 72: the TLS AEAD ciphers of userspace/wf_aead.c (AES-256-GCM and
 * ChaCha20-Poly1305): key 00..1f, nonce a0..ab, aad "hdr", text
 * "mohhdy lot 72"; the expected bytes match OpenSSL. A flipped tag bit
 * is refused and leaves the output untouched. */
int arm_port_aead(void) {
    static const uint8_t want[2][29] = {
        { 0x8b,0x77,0x14,0x45,0x21,0xb2,0x22,0xd3,0x0d,0x11,0xa7,0xe4,0x35,
          0xbd,0x98,0x94,0x9f,0x5e,0x1f,0x76,0x61,0x4f,0xb7,0x24,0x7c,0x98,0x00,0xc0,0xed },
        { 0x61,0xc4,0x10,0x37,0x29,0x9f,0xe2,0xc1,0xcf,0x7b,0xd3,0x23,0xce,
          0x8f,0xf4,0x06,0x31,0xf6,0x94,0x65,0x89,0x9f,0xb5,0x44,0x78,0x31,0x6e,0x71,0x15 } };
    static const uint8_t pt[13] = { 'm','o','h','h','d','y',' ','l','o','t',' ','7','2' };
    static wf_aead_t c;
    uint8_t key[32], nonce[12], ct[13], tag[16], back[13];
    int i, a;
    for (i = 0; i < 32; i++) key[i] = (uint8_t)i;
    for (i = 0; i < 12; i++) nonce[i] = (uint8_t)(0xA0 + i);
    for (a = 0; a < 2; a++) {
        if (wf_aead_init(&c, a == 0 ? WF_AEAD_AES256_GCM : WF_AEAD_CHACHA20_POLY1305, key) != 0) return 0;
        if (wf_aead_seal(&c, nonce, (const uint8_t*)"hdr", 3, pt, 13, ct, tag) != 0) return 0;
        if (!same(ct, want[a], 13) || !same(tag, want[a] + 13, 16)) return 0;
        if (wf_aead_open(&c, nonce, (const uint8_t*)"hdr", 3, ct, 13, tag, back) != 0 || !same(back, pt, 13)) return 0;
        tag[15] ^= 1U;
        back[0] = 0;
        if (wf_aead_open(&c, nonce, (const uint8_t*)"hdr", 3, ct, 13, tag, back) != -1 || back[0] != 0) return 0;
        wf_aead_wipe(&c);
    }
    return 1;
}

/* Lot 72: glyph pages of include/os_charset.h: UTF-8 Greek, Cyrillic and
 * an arrow become selector + byte runs (one cell per letter), and go back
 * to the same UTF-8 */
int arm_port_pages(void) {
    static const char in[] = { (char)0xCE,(char)0xA9,' ',(char)0xD0,(char)0x96,(char)0xE2,(char)0x86,(char)0x92,'!',0 };
    static const uint8_t want[] = { 0x8F,0xD9,' ',0x90,0xB6,0x9D,0x83,'!' };
    char out[24], back[24];
    int n = os_text_to_display(in, (int)sizeof(in) - 1, out, (int)sizeof(out));
    if (n != 8 || !same((const uint8_t*)out, want, 8)) return 0;
    if (os_display_cells(out, n) != 5) return 0;
    os_display_str_to_utf8(out, n, back, (int)sizeof(back));
    return same((const uint8_t*)back, (const uint8_t*)in, (int)sizeof(in));
}

/* Lot 84 (US-068/073, US-076): a probe declared lost still yields a
 * delivery-rate sample when its ack arrives late. 256 bytes over the
 * 2000 ms between send and the late ack is 128 B/s. */
int arm_port_bwe(void) {
    p2p_bwe_t b;
    uint32_t rate;
    p2p_bwe_init(&b, 256, 20000U, 0U);
    if (p2p_bwe_on_send(&b, 1U, 256U, 0U) != 0) return 0;
    p2p_bwe_on_loss(&b, 1U, 2000U);
    if (b.inflight_bytes != 0U || b.losses != 1U) return 0;
    rate = p2p_bwe_on_ack(&b, 1U, 2000U);
    if (!b.late_hit || rate != 128U || b.samples != 1U || p2p_bwe_btlbw(&b) == 0U || b.losses != 0U) return 0;
    if (p2p_bwe_on_ack(&b, 1U, 2001U) != 0U || b.late_hit) return 0;
    return p2p_bwe_on_ack(&b, 99U, 2001U) == 0U;
}

/* Lot 85: U+1EA0 (A with dot below) is a composed letter on a glyph page,
 * and a letter with no free byte falls back to its base letter. */
int arm_port_marks(void) {
    char out[8], back[8];
    int page = 0, n;
    unsigned char b = 0;
    n = os_display_put(0x1EA0u, &page, out, (int)sizeof(out));
    if (n != 2 || page != 2 || (unsigned char)out[0] != 0x8Fu) return 0;
    if (os_display_cp(2, (unsigned char)out[1]) != 0x1EA0u) return 0;
    if (os_cp_page(0x1EA0u, &b) != 2) return 0;
    os_display_str_to_utf8(out, n, back, (int)sizeof(back));
    if ((unsigned char)back[0] != 0xE1u || (unsigned char)back[1] != 0xBAu || (unsigned char)back[2] != 0xA0u) return 0;
    if (os_mark_ascii_base(0x1EB2u) != 'A') return 0;
    if (os_cp_page(0x1EB2u, &b) != 5) return 0;
    page = 0;
    n = os_display_put(0x1EB2u, &page, out, (int)sizeof(out));
    if (n != 3 || page != 5 || (unsigned char)out[1] != 0xFFu) return 0;
    if (os_display_cells(out, n) != 1) return 0;
    if (os_cp1252_from_cp(0xFF21u) != 'A') return 0;
    if (os_cp1252_from_cp(0x0259u) != 'e') return 0;
    /* Lot 86: A with circumflex and hook (U+1EA8) is the first page-5 byte. */
    if (os_cp_page(0x1EA8u, &b) != 5 || b != 0x80u) return 0;
    page = 0;
    n = os_display_put(0x1EA8u, &page, out, (int)sizeof(out));
    if (n != 3 || (unsigned char)out[1] != 0xFFu || (unsigned char)out[2] != 0x80u) return 0;
    if (os_display_cells(out, n) != 1) return 0;
    return 1;
}

/* Lot 87: the delay-gradient detector. 100 probes on a steady path, then
 * 60 whose queue grows 1 ms each: overuse once, and the cap is 85 % of
 * the acked rate, 15515 B/s on i386 and x86-64 with the same source. */
int arm_port_trend(void) {
    p2p_trend_t t;
    uint32_t i;
    p2p_trend_init(&t, 0U);
    for (i = 0; i < 100U; i++) p2p_trend_on_packet(&t, i * 10U, i * 10U + 20U, 200U);
    if (t.state != P2P_TREND_NORMAL || t.overuses != 0U) return 0;
    for (i = 0; i < 60U; i++) p2p_trend_on_packet(&t, 1000U + i * 10U, 1000U + i * 10U + 20U + i, 200U);
    if (t.state != P2P_TREND_OVERUSE || t.overuses != 1U) return 0;
    if (p2p_trend_cap(&t) != 15515U || t.acked_bps != 18253U) return 0;
    return p2p_trend_limit(&t, 1000000U, t.last_update_ms + 10U) == 15515U;
}

/* Lot 87: a CEA-861 block with a native 1080p SVD and an HDMI vendor
 * block; the parse picks 1920x1080@60 and physical address 1.0.0.0. */
int arm_port_cea(void) {
    static uint8_t b[EDID_BLOCK];
    static const uint8_t body[] = { 0x02, 0x03, 0x00, 0xE0,
                                    0x43, 0x90, 0x04, 0x03,          /* SVD: 16 native, 4, 3 */
                                    0x65, 0x03, 0x0C, 0x00, 0x10, 0x00 }; /* HDMI 1.0.0.0 */
    edid_cea_t c;
    unsigned i, s = 0;
    int w = 0, h = 0, hz = 0;
    for (i = 0; i < EDID_BLOCK; i++) b[i] = 0;
    for (i = 0; i < sizeof(body); i++) b[i] = body[i];
    b[2] = (uint8_t)sizeof(body);
    for (i = 0; i < EDID_BLOCK - 1; i++) s += b[i];
    b[EDID_BLOCK - 1] = (uint8_t)((256u - (s & 0xFFu)) & 0xFFu);
    if (edid_cea_parse(b, EDID_BLOCK, &c) != 0 || !c.ok) return 0;
    if (c.svd_n != 3 || !c.svd_native[0] || c.vic[0] != 16 || !c.hdmi || c.phys_addr != 0x1000u) return 0;
    if (!c.underscan || !c.basic_audio || !c.ycbcr444 || c.ycbcr422) return 0;
    if (edid_cea_best(&c, &w, &h, &hz) != 0) return 0;
    if (w != 1920 || h != 1080 || hz != 60) return 0;
    {
        /* Lot 90: this TV declares underscan, so no overscan margin; the
         * same summary without the bit gets 2.5% per side */
        edid_cea_sum_t sum;
        int mx = -1, my = -1;
        edid_cea_summarize(&c, 0, 2, &sum);
        if (edid_overscan_margin(&sum, 0, 1920, 1080, &mx, &my) != 0 || mx != 0 || my != 0) return 0;
        sum.flags = (uint8_t)(sum.flags & ~EDID_SUM_UNDERSCAN);
        if (edid_overscan_margin(&sum, 0, 1920, 1080, &mx, &my) != 1 || mx != 48 || my != 27) return 0;
    }
    b[5] ^= 1u;   /* a damaged block is refused */
    return edid_cea_parse(b, EDID_BLOCK, &c) == -2;
}

static int same_str(const char* a, const char* b) {
    while (*a && *a == *b) { a++; b++; }
    return *a == *b;
}

/* Lot 88: registrable domains (autofill per site). Lower case, a country
 * second level, a hosting suffix whose subdomains are different owners,
 * and hosts that have none. */
int arm_port_regdom(void) {
    char out[WF_REGDOM_MAX];
    if (wf_regdom("Login.Example.COM", out, (int)sizeof(out)) != 0 || !same_str(out, "example.com")) return 0;
    if (wf_regdom("a.b.shop.co.uk", out, (int)sizeof(out)) != 0 || !same_str(out, "shop.co.uk")) return 0;
    if (wf_regdom("alice.github.io", out, (int)sizeof(out)) != 0 || !same_str(out, "alice.github.io")) return 0;
    if (wf_regdom("co.uk", out, (int)sizeof(out)) != -1) return 0;
    if (wf_regdom("10.0.2.2", out, (int)sizeof(out)) != -1 || !wf_regdom_is_ip("10.0.2.2")) return 0;
    if (wf_regdom("localhost", out, (int)sizeof(out)) != -1) return 0;
    if (!wf_regdom_same_site("www.example.com", "login.example.com")) return 0;
    if (wf_regdom_same_site("alice.github.io", "bob.github.io")) return 0;
    /* Lot 89: an IDN host (UTF-8 b, u-umlaut, cher) in its ASCII form,
     * through the Punycode of userspace/wf_idna.c */
    if (wf_regdom("www.b\xc3\xbc" "cher.example", out, (int)sizeof(out)) != 0 || !same_str(out, "xn--bcher-kva.example")) return 0;
    if (!wf_regdom_same_site("\xe4\xbe\x8b\xe5\xad\x90.example", "xn--fsqu00a.example")) return 0;
    /* Lot 90: PSL wildcard and exception rules */
    if (wf_regdom("a.b.ck", out, (int)sizeof(out)) != 0 || !same_str(out, "a.b.ck")) return 0;
    if (wf_regdom("x.www.ck", out, (int)sizeof(out)) != 0 || !same_str(out, "www.ck")) return 0;
    if (wf_regdom("x.city.kawasaki.jp", out, (int)sizeof(out)) != 0 || !same_str(out, "city.kawasaki.jp")) return 0;
    if (wf_regdom("ward.kawasaki.jp", out, (int)sizeof(out)) != -1 || !wf_regdom_is_suffix("ward.kawasaki.jp")) return 0;
    return !wf_regdom_same_site("example.com", "evil-example.com");
}

/* Lot 88: the desktop frame pacer. 40 idle wakes 100 ms apart at the
 * deep-dim 5 Hz budget paint 20, a key paints at once and wakes, and the
 * period holds across the 2^32 wrap of the millisecond clock. */
int arm_port_frame(void) {
    osui_frame_t f;
    uint32_t now = 0xFFFFF800U;   /* 2048 ms before the wrap */
    int i, painted = 0;
    osui_frame_init(&f, now);
    for (i = 0; i < 40; i++) {
        now += 100U;
        if (osui_frame_due(&f, now, 0, 5U, 2)) {
            painted++;
            osui_frame_cost(&f, now, now + 40U, 1);   /* chunk 9: a 40 ms paint */
        }
    }
    if (painted != 20 || f.idle_skipped != 20U || osui_frame_idle_fps10(&f, now) != 50U) return 0;
    /* 20 paints of 40 ms in 3.9 s at the deep dim (the first interval
     * counts at level 0, the level before the first call): 205 permille,
     * and 800 ms not spent on the 20 skipped wakes */
    if (f.paint_ms[2] != 800U || osui_frame_busy_permille(&f, 2) != 205U || osui_frame_saved_ms(&f) != 800U) return 0;
    if (osui_frame_due(&f, now + 1U, 1, 5U, 0) != 1 || f.wakes != 1U || f.idle_drawn != 0U) return 0;
    return osui_frame_idle_ms(&f, now + 1U) == 0U && now < 0x1000U;
}

/* Lot 88 (US-080): the safe TV scale. A 1080p TV wants 2x; on 1024x768 it
 * steps down to 1.5x, on 1280x800 it keeps 2x, a pin or a monitor gets
 * nothing, and with no framebuffer yet the desktop's 1024x768 is used. */
int arm_port_tvq(void) {
    static os_display_info_t d;
    const char* why = 0;
    uint8_t* p = (uint8_t*)&d;
    unsigned i;
    for (i = 0; i < sizeof(d); i++) p[i] = 0;
    d.cea_status = 0;
    d.cea_tv = 1;
    d.cea_best_from = 1;
    d.cea_best_w = 1920; d.cea_best_h = 1080;
    d.fb_w = 1024; d.fb_h = 768;
    if (plat_form_tv_safe_q(&d, &why) != 6 || !same_str(why, "stepped down for the framebuffer")) return 0;
    d.fb_w = 1280; d.fb_h = 800;
    if (plat_form_tv_safe_q(&d, &why) != 8 || !same_str(why, "tv 1080p")) return 0;
    d.fb_w = 0; d.fb_h = 0;
    if (plat_form_tv_safe_q(&d, &why) != 6) return 0;
    d.ui_hint_q = 4;
    if (plat_form_tv_safe_q(&d, &why) != 0 || !same_str(why, "pinned")) return 0;
    d.ui_hint_q = 0;
    d.cea_tv = 0;
    return plat_form_tv_safe_q(&d, &why) == 0 && same_str(why, "not a tv");
}

/* Lot 88 (US-068/US-073): the fair shaping queue. Peer 5 queues x in
 * slot 0, peer 3 queues 1 and 2 in slots 1 and 2; the first pass refuses
 * peer 3 (its pacer is closed), so x leaves; 3 then takes the freed slot
 * 0, and the second pass still sends 1 2 3 in that order (the Lot 6
 * queue sent slot 0 first). A full queue held by peer 3 makes
 * room for peer 5 by pushing out peer 3's newest entry. */
static char g_shape_log[24];
static int g_shape_open3;

static int shape_cb(void* ctx, uint32_t peer, int inner, const uint8_t* body, int len) {
    int n = 0;
    (void)ctx; (void)inner; (void)len;
    if (peer == 3U && !g_shape_open3) return P2P_SHAPE_WAIT;
    while (n < 22 && g_shape_log[n]) n++;
    if (n < 22) { g_shape_log[n] = (char)body[0]; g_shape_log[n + 1] = 0; }
    return P2P_SHAPE_SENT;
}

int arm_port_shape(void) {
    static p2p_shape_t q;
    uint8_t b[4] = { 0, 0, 0, 0 };
    int i, d = 0;
    p2p_shape_init(&q);
    g_shape_log[0] = 0;
    g_shape_open3 = 0;
    b[0] = 'x'; (void)p2p_shape_push(&q, 5U, 0, 1, b, 4, 0);
    b[0] = '1'; (void)p2p_shape_push(&q, 3U, 0, 1, b, 4, 0);
    b[0] = '2'; (void)p2p_shape_push(&q, 3U, 0, 1, b, 4, 0);
    if (p2p_shape_drain(&q, shape_cb, 0, 10, &d) != 1 || !same_str(g_shape_log, "x")) return 0;
    b[0] = '3'; (void)p2p_shape_push(&q, 3U, 0, 1, b, 4, 20);
    if (!q.e[0].used || q.e[0].body[0] != '3') return 0;   /* the freed front slot */
    g_shape_open3 = 1;
    if (p2p_shape_drain(&q, shape_cb, 0, 50, &d) != 3 || !same_str(g_shape_log, "x123")) return 0;
    if (q.max_wait_ms != 50U || q.skips != 1U) return 0;
    for (i = 0; i < P2P_SHAPE_Q; i++) { b[0] = (uint8_t)('a' + i); (void)p2p_shape_push(&q, 3U, 0, 1, b, 4, 60); }
    if (p2p_shape_push(&q, 3U, 0, 1, b, 4, 60) != P2P_SHAPE_REFUSED) return 0;
    if (p2p_shape_push(&q, 5U, 1, 2, b, 4, 60) != P2P_SHAPE_QUEUED_EVICT) return 0;
    if (!(p2p_shape_depth(&q, 3U, -1) == P2P_SHAPE_Q - 1 && p2p_shape_depth(&q, 5U, 1) == 1 && d == 0)) return 0;
    /* Lot 90: weights and the age limit (every entry was pushed at 60 ms) */
    if (p2p_shape_set_weight(&q, 0, 17) != -1 || p2p_shape_set_weight(&q, 0, 2) != 0 || p2p_shape_weight(&q, 0) != 2) return 0;
    if (p2p_shape_set_max_age(&q, 30) != 0 || p2p_shape_expire(&q, 90) != 0) return 0;
    return p2p_shape_expire(&q, 91) == P2P_SHAPE_Q && p2p_shape_depth(&q, 0, -1) == 0;
}

uint32_t arm_port_selftest(void) {
    uint32_t m = 0;
    if (arm_port_aes_gcm()) m |= ARM_PORT_AES_GCM;
    if (arm_port_edid()) m |= ARM_PORT_EDID;
    if (arm_port_flow()) m |= ARM_PORT_FLOW;
    if (arm_port_rtx()) m |= ARM_PORT_RTX;
    if (arm_port_charset()) m |= ARM_PORT_CHARSET;
    if (arm_port_aead()) m |= ARM_PORT_AEAD;
    if (arm_port_pages()) m |= ARM_PORT_PAGES;
    if (arm_port_bwe()) m |= ARM_PORT_BWE;
    if (arm_port_marks()) m |= ARM_PORT_MARKS;
    if (arm_port_trend()) m |= ARM_PORT_TREND;
    if (arm_port_cea()) m |= ARM_PORT_CEA;
    if (arm_port_regdom()) m |= ARM_PORT_REGDOM;
    if (arm_port_frame()) m |= ARM_PORT_FRAME;
    if (arm_port_tvq()) m |= ARM_PORT_TVQ;
    if (arm_port_shape()) m |= ARM_PORT_SHAPE;
    return m;
}
