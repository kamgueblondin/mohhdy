/* Phase 6 multi-platform groundwork (US-076..US-090). */
#include "../../framework/unity.h"
#include <string.h>
#include <stdio.h>
#include "../../../userspace/platform.h"

static char out[4096];

void test_hal_and_ports(void) {
    plat_hal_t h;
    const char* why;
    memset(&h, 0, sizeof(h));
    h.arch = "i386"; h.word_bits = 32; h.page_size = 4096; h.little_endian = 1;
    strcpy(h.cpu_vendor, "GenuineIntel"); h.cpu_family = 6; h.features_edx = 1U | (1U << 4) | (1U << 26);
    h.mem_total_kib = 1048576; h.mem_free_kib = 900000;
    plat_hal_report(&h, out, sizeof(out));
    TEST_ASSERT_NOT_NULL(strstr(out, "hal arch i386 bits 32 endian little page 4096"));
    TEST_ASSERT_NOT_NULL(strstr(out, "vendor GenuineIntel family 6"));
    TEST_ASSERT_NOT_NULL(strstr(out, "tsc yes sse2 yes rdrand no"));
    TEST_ASSERT_NOT_NULL(strstr(out, "hal ports i386=built arm=not-ported"));
    TEST_ASSERT_EQUAL(1, plat_port_supported("i386", &why));
    TEST_ASSERT_EQUAL(0, plat_port_supported("arm", &why));
    TEST_ASSERT_NOT_NULL(strstr(why, "not ported"));
    TEST_ASSERT_EQUAL(0, plat_port_supported("riscv", &why));
}

void test_layout_forms(void) {
    plat_layout_t l;
    TEST_ASSERT_EQUAL(0, plat_layout(1024, 768, &l));
    TEST_ASSERT_EQUAL(PLAT_FORM_DESKTOP, l.form); TEST_ASSERT_EQUAL(3, l.panels); TEST_ASSERT_EQUAL(0, l.compact);
    TEST_ASSERT_EQUAL(128, l.cols); TEST_ASSERT_EQUAL(48, l.rows);
    TEST_ASSERT_EQUAL(0, plat_layout(720, 400, &l)); /* VGA text 80x25 */
    TEST_ASSERT_EQUAL(PLAT_FORM_TABLET, l.form); TEST_ASSERT_EQUAL(90, l.cols); TEST_ASSERT_EQUAL(25, l.rows);
    TEST_ASSERT_EQUAL(2, l.panels);
    TEST_ASSERT_EQUAL(0, plat_layout(360, 640, &l));
    TEST_ASSERT_EQUAL(PLAT_FORM_PHONE, l.form); TEST_ASSERT_EQUAL(1, l.portrait); TEST_ASSERT_EQUAL(1, l.compact);
    TEST_ASSERT_EQUAL(1, l.panels); TEST_ASSERT_EQUAL(20, l.font_px); TEST_ASSERT_EQUAL(36, l.cols);
    TEST_ASSERT_EQUAL(0, plat_layout(768, 1024, &l));
    TEST_ASSERT_EQUAL(PLAT_FORM_TABLET, l.form); TEST_ASSERT_EQUAL(1, l.panels);
    TEST_ASSERT_EQUAL(-1, plat_layout(100, 100, &l));
    TEST_ASSERT_EQUAL(-1, plat_layout(9000, 100, &l));
    TEST_ASSERT_EQUAL_STRING("phone", plat_form_name(PLAT_FORM_PHONE));
}

void test_gestures(void) {
    plat_point_t tap[] = {{10, 10, 0, 1}, {12, 11, 80, 1}, {12, 11, 100, 0}};
    plat_point_t lp[] = {{10, 10, 0, 1}, {11, 10, 700, 1}, {11, 10, 720, 0}};
    plat_point_t sl[] = {{200, 50, 0, 1}, {120, 55, 100, 1}, {60, 52, 180, 0}};
    plat_point_t su[] = {{50, 300, 0, 1}, {52, 150, 150, 1}};
    plat_point_t dt[] = {{10, 10, 0, 1}, {10, 10, 60, 0}, {11, 10, 200, 1}, {11, 10, 260, 0}};
    plat_point_t dr[] = {{10, 10, 0, 1}, {200, 10, 2000, 1}};
    TEST_ASSERT_EQUAL(PLAT_G_TAP, plat_gesture(tap, 3));
    TEST_ASSERT_EQUAL(PLAT_G_LONG_PRESS, plat_gesture(lp, 3));
    TEST_ASSERT_EQUAL(PLAT_G_SWIPE_LEFT, plat_gesture(sl, 3));
    TEST_ASSERT_EQUAL(PLAT_G_SWIPE_UP, plat_gesture(su, 2));
    TEST_ASSERT_EQUAL(PLAT_G_DOUBLE_TAP, plat_gesture(dt, 4));
    TEST_ASSERT_EQUAL(PLAT_G_DRAG, plat_gesture(dr, 2));
    TEST_ASSERT_EQUAL(PLAT_G_NONE, plat_gesture(tap, 1));
    TEST_ASSERT_EQUAL_STRING("swipe-left", plat_gesture_name(PLAT_G_SWIPE_LEFT));
}

void test_power(void) {
    plat_power_policy_t p;
    TEST_ASSERT_EQUAL(0, plat_power_policy(PLAT_PWR_SAVER, &p));
    TEST_ASSERT_EQUAL(8, (int)p.idle_yields); TEST_ASSERT_EQUAL(1, (int)p.poll_budget);
    TEST_ASSERT_EQUAL(0, plat_power_policy(PLAT_PWR_PERFORMANCE, &p));
    TEST_ASSERT_EQUAL(0, (int)p.screen_dim_s);
    TEST_ASSERT_EQUAL(-1, plat_power_policy(9, &p));
    TEST_ASSERT_EQUAL(PLAT_PWR_BALANCED, plat_power_parse("balanced"));
    TEST_ASSERT_EQUAL(0, plat_power_parse("turbo"));
    TEST_ASSERT_EQUAL(PLAT_PWR_PERFORMANCE, plat_power_auto(90, 0));
    TEST_ASSERT_EQUAL(PLAT_PWR_SAVER, plat_power_auto(5, 0));
    TEST_ASSERT_EQUAL(PLAT_PWR_BALANCED, plat_power_auto(40, 0));
    TEST_ASSERT_EQUAL(PLAT_PWR_SAVER, plat_power_auto(40, 1));
    TEST_ASSERT_EQUAL(PLAT_PWR_BALANCED, plat_power_auto(95, 1));
}

void test_devices(void) {
    plat_devices_t r;
    memset(&r, 0, sizeof(r));
    TEST_ASSERT_EQUAL(0, plat_dev_add(&r, "cpu0", "cpu", 1, "i386"));
    TEST_ASSERT_EQUAL(0, plat_dev_add(&r, "eth0", "net", 0, "ne2000 not found"));
    TEST_ASSERT_EQUAL(2, plat_dev_report(&r, 0, out, sizeof(out)));
    TEST_ASSERT_NOT_NULL(strstr(out, "dev cpu0 class cpu present i386"));
    TEST_ASSERT_NOT_NULL(strstr(out, "dev eth0 class net absent ne2000 not found"));
    TEST_ASSERT_NOT_NULL(strstr(out, "dev ok count 2 present 1"));
    TEST_ASSERT_EQUAL(1, plat_dev_report(&r, "eth0", out, sizeof(out)));
    TEST_ASSERT_EQUAL(-1, plat_dev_report(&r, "gpu0", out, sizeof(out)));
    while (r.n < PLAT_DEVICES) plat_dev_add(&r, "x", "y", 1, "");
    TEST_ASSERT_EQUAL(-1, plat_dev_add(&r, "z", "z", 1, ""));
}

void test_elf_compat(void) {
    uint8_t b[64];
    plat_compat_t c;
    memset(b, 0, sizeof(b));
    b[0] = 0x7f; b[1] = 'E'; b[2] = 'L'; b[3] = 'F'; b[4] = 1; b[5] = 1;
    b[16] = 2; b[18] = 3; b[24] = 0x00; b[25] = 0x10; b[26] = 0x40; b[44] = 2;
    TEST_ASSERT_EQUAL(1, plat_elf_check(b, 64, &c));
    TEST_ASSERT_EQUAL(0x401000, (int)c.entry); TEST_ASSERT_EQUAL(2, (int)c.phnum);
    b[18] = 40;
    TEST_ASSERT_EQUAL(0, plat_elf_check(b, 64, &c)); TEST_ASSERT_NOT_NULL(strstr(c.reason, "ARM"));
    b[18] = 3; b[4] = 2;
    TEST_ASSERT_EQUAL(0, plat_elf_check(b, 64, &c)); TEST_ASSERT_NOT_NULL(strstr(c.reason, "ELF64"));
    b[4] = 1; b[16] = 3;
    TEST_ASSERT_EQUAL(0, plat_elf_check(b, 64, &c));
    TEST_ASSERT_EQUAL(0, plat_elf_check((const uint8_t*)"#!/bin/sh\n", 10, &c));
}

void test_notifications(void) {
    plat_notes_t n;
    uint32_t a, b;
    int i;
    char t[16];
    memset(&n, 0, sizeof(n));
    a = plat_note_push(&n, 1, "backup done");
    b = plat_note_push(&n, 3, "disk full");
    TEST_ASSERT_EQUAL(a, plat_note_push(&n, 2, "backup done")); /* coalesced */
    TEST_ASSERT_EQUAL(2, plat_note_report(&n, out, sizeof(out)));
    TEST_ASSERT_TRUE(strstr(out, "disk full") < strstr(out, "backup done")); /* priority order */
    TEST_ASSERT_NOT_NULL(strstr(out, "prio 2 x2 backup done"));
    TEST_ASSERT_EQUAL(0, plat_note_ack(&n, b));
    TEST_ASSERT_EQUAL(-1, plat_note_ack(&n, b));
    TEST_ASSERT_EQUAL(0, (int)plat_note_push(&n, 0, "bad"));
    for (i = 0; i < 10; i++) { sprintf(t, "low %d", i); plat_note_push(&n, 1, t); }
    TEST_ASSERT_GREATER_THAN(0, (int)n.dropped);
    TEST_ASSERT_NOT_NULL(strstr(out, "notify ok pending 2 dropped 0"));
}

void test_migration_roundtrip_and_tamper(void) {
    plat_mig_entry_t e[3] = {{"notes.txt", "hello\nworld", 11}, {"empty", "", 0}, {"b.pm", "print \"x\"", 9}};
    plat_mig_entry_t got[PLAT_MIG_FILES];
    static char arc[2048];
    int n = plat_mig_pack(e, 3, arc, sizeof(arc));
    TEST_ASSERT_GREATER_THAN(0, n);
    TEST_ASSERT_EQUAL(0, strncmp(arc, "MMIG1\nF notes.txt 11 ", 21));
    TEST_ASSERT_EQUAL(3, plat_mig_unpack(arc, n, got, PLAT_MIG_FILES));
    TEST_ASSERT_EQUAL_STRING("notes.txt", got[0].name);
    TEST_ASSERT_EQUAL(11, got[0].len); TEST_ASSERT_EQUAL(0, memcmp(got[0].data, "hello\nworld", 11));
    TEST_ASSERT_EQUAL(0, got[1].len);
    arc[25] ^= 1; /* corrupt a byte of the first file */
    TEST_ASSERT_LESS_THAN(0, plat_mig_unpack(arc, n, got, PLAT_MIG_FILES));
    arc[25] ^= 1;
    TEST_ASSERT_LESS_THAN(0, plat_mig_unpack(arc, n - 5, got, PLAT_MIG_FILES));
    TEST_ASSERT_EQUAL(-1, plat_mig_unpack(arc, 40, got, PLAT_MIG_FILES)); /* truncated */
    TEST_ASSERT_EQUAL(-3, plat_mig_unpack(arc, n, got, 2));
    {
        plat_mig_entry_t bad[1] = {{"../etc", "x", 1}};
        TEST_ASSERT_EQUAL(-1, plat_mig_pack(bad, 1, arc, sizeof(arc)));
    }
    TEST_ASSERT_EQUAL(-1, plat_mig_pack(e, 3, arc, 20)); /* too small */
}

void test_deploy_manifest(void) {
    plat_deploy_item_t it[4];
    const char* m = "DEPLOY1\nfile /pm/hello.pm /apps/hello.pm 1a2b3c4d\nfile /a /b 0\n";
    TEST_ASSERT_EQUAL(2, plat_deploy_parse(m, (int)strlen(m), it, 4));
    TEST_ASSERT_EQUAL_STRING("/apps/hello.pm", it[0].dst);
    TEST_ASSERT_EQUAL(0x1a2b3c4d, (int)it[0].fnv);
    TEST_ASSERT_EQUAL(-1, plat_deploy_parse("DEPLOY1\nfile /a /x/../y 0\n", 25, it, 4));
    TEST_ASSERT_EQUAL(-1, plat_deploy_parse("NOPE\n", 5, it, 4));
    TEST_ASSERT_EQUAL(-1, plat_deploy_parse(m, (int)strlen(m), it, 1));
    TEST_ASSERT_EQUAL((int)0x811c9dc5, (int)plat_fnv((const uint8_t*)"", 0));
    TEST_ASSERT_EQUAL((int)0xe40c292c, (int)plat_fnv((const uint8_t*)"a", 1));
}

int main(void) {
    unity_init();
    RUN_TEST(test_hal_and_ports);
    RUN_TEST(test_layout_forms);
    RUN_TEST(test_gestures);
    RUN_TEST(test_power);
    RUN_TEST(test_devices);
    RUN_TEST(test_elf_compat);
    RUN_TEST(test_notifications);
    RUN_TEST(test_migration_roundtrip_and_tamper);
    RUN_TEST(test_deploy_manifest);
    unity_print_results();
    unity_cleanup();
    return (unity_stats.tests_failed == 0) ? 0 : 1;
}
