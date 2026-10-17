/* Lot 7 (US-086): 8086 .COM interpreter. The three programs are the ones of
 * dosapps/ (shipped in the initrd under /dos). */
#include "../../framework/unity.h"
#include "../../../userspace/dosemu.h"
#include <string.h>

static char out[512];
static dos_result_t r;

static const unsigned char hello[] = {
    0xBA, 0x0C, 0x01, 0xB4, 0x09, 0xCD, 0x21, 0xB8, 0x00, 0x4C, 0xCD, 0x21,
    'H', 'e', 'l', 'l', 'o', ' ', 'f', 'r', 'o', 'm', ' ', 'a', ' ', 'D', 'O', 'S', ' ', '.', 'C', 'O', 'M',
    ' ', 'p', 'r', 'o', 'g', 'r', 'a', 'm', 0x0D, 0x0A, '$'};
static const unsigned char count[] = {
    0xB9, 0x0A, 0x00, 0xB2, 0x30, 0xB4, 0x02, 0xCD, 0x21, 0xFE, 0xC2, 0xE2, 0xF8, 0xB2, 0x0D, 0xCD, 0x21,
    0xB2, 0x0A, 0xCD, 0x21, 0x31, 0xC0, 0xB9, 0x64, 0x00, 0x01, 0xC8, 0xE2, 0xFC, 0xB4, 0x4C, 0xCD, 0x21};
static const unsigned char upper[] = {
    0xBE, 0x27, 0x01, 0x89, 0xF7, 0xE8, 0x0C, 0x00, 0xBA, 0x27, 0x01, 0xB4, 0x09, 0xCD, 0x21, 0xB8, 0x07,
    0x4C, 0xCD, 0x21, 0xAC, 0x3C, 0x24, 0x74, 0x0D, 0x3C, 0x61, 0x72, 0x06, 0x3C, 0x7A, 0x77, 0x02, 0x2C,
    0x20, 0xAA, 0xEB, 0xEE, 0xC3,
    'm', 'o', 'h', 'h', 'd', 'y', ' ', 'd', 'o', 's', ' ', 'o', 'k', 0x0D, 0x0A, '$'};

static void test_hello(void) {
    TEST_ASSERT_EQUAL(DOS_EXITED, dos_run_com(hello, (int)sizeof(hello), out, sizeof(out), 1000, &r));
    TEST_ASSERT_EQUAL_STRING("Hello from a DOS .COM program\n", out);
    TEST_ASSERT_EQUAL(0, r.exit_code);
    TEST_ASSERT_EQUAL(5, (int)r.steps);
    TEST_ASSERT_EQUAL(2, (int)r.ints);
}

static void test_loops_and_arithmetic(void) {
    TEST_ASSERT_EQUAL(DOS_EXITED, dos_run_com(count, (int)sizeof(count), out, sizeof(out), 10000, &r));
    TEST_ASSERT_EQUAL_STRING("0123456789\n", out);
    TEST_ASSERT_EQUAL(0xBA, r.exit_code);        /* 5050 = 13BAh */
    TEST_ASSERT_EQUAL(2 + 10 * 4 + 4 + 2 + 100 * 2 + 2, (int)r.steps);   /* 250 */
}

static void test_call_ret_and_strings(void) {
    TEST_ASSERT_EQUAL(DOS_EXITED, dos_run_com(upper, (int)sizeof(upper), out, sizeof(out), 10000, &r));
    TEST_ASSERT_EQUAL_STRING("MOHHDY DOS OK\n", out);
    TEST_ASSERT_EQUAL(7, r.exit_code);
}

static void test_more_opcodes(void) {
    /* mov ax,1234 / mov bx,10 / xor dx,dx / div bx (AX 123 DX 4) / push ax /
     * pop cx / shl cx,1 (246) / mov al,cl / int 21h AH=4C ... exit 246 */
    static const unsigned char p1[] = {
        0xB8, 0xD2, 0x04, 0xBB, 0x0A, 0x00, 0x31, 0xD2, 0xF7, 0xF3, 0x50, 0x59, 0xD1, 0xE1,
        0x88, 0xC8, 0xB4, 0x4C, 0xCD, 0x21};
    /* REP STOSB fills 5 bytes at 0x200 with 'z', then a '$', prints them */
    static const unsigned char p2[] = {
        0xFC, 0xBF, 0x00, 0x02, 0xB9, 0x05, 0x00, 0xB0, 'z', 0xF3, 0xAA, 0xC6, 0x05, '$',
        0xBA, 0x00, 0x02, 0xB4, 0x09, 0xCD, 0x21, 0xC3};
    /* signed compare: mov al,-5 / cmp al,3 / jl +2 / mov al,1 / int 20h path
     * via RET to the PSP: AL stays -5 (jl taken) and RET exits through INT 20h */
    static const unsigned char p3[] = {0xB0, 0xFB, 0x3C, 0x03, 0x7C, 0x02, 0xB0, 0x01, 0xB4, 0x0E, 0xCD, 0x10, 0xC3};
    TEST_ASSERT_EQUAL(DOS_EXITED, dos_run_com(p1, (int)sizeof(p1), out, sizeof(out), 100, &r));
    TEST_ASSERT_EQUAL(246, r.exit_code);
    TEST_ASSERT_EQUAL(DOS_EXITED, dos_run_com(p2, (int)sizeof(p2), out, sizeof(out), 100, &r));
    TEST_ASSERT_EQUAL_STRING("zzzzz", out);
    TEST_ASSERT_EQUAL(0, r.exit_code);
    TEST_ASSERT_EQUAL(DOS_EXITED, dos_run_com(p3, (int)sizeof(p3), out, sizeof(out), 100, &r));
    TEST_ASSERT_EQUAL(1, r.out_len);
    TEST_ASSERT_EQUAL((char)0xFB, out[0]);
}

static void test_errors_are_reported(void) {
    static const unsigned char spin[] = {0xEB, 0xFE};                 /* jmp $ */
    static const unsigned char p386[] = {0x90, 0x0F, 0xA2};          /* cpuid: 0F not 8086 */
    static const unsigned char div0[] = {0x31, 0xDB, 0xF7, 0xF3};    /* xor bx,bx / div bx */
    static const unsigned char file[] = {0xB4, 0x3D, 0xCD, 0x21};    /* AH=3D open file */
    static const unsigned char port[] = {0xE4, 0x60};                /* in al,60h */
    TEST_ASSERT_EQUAL(DOS_ERR_STEPS, dos_run_com(spin, 2, out, sizeof(out), 500, &r));
    TEST_ASSERT_EQUAL(500, (int)r.steps);
    TEST_ASSERT_EQUAL(DOS_ERR_OPCODE, dos_run_com(p386, 3, out, sizeof(out), 500, &r));
    TEST_ASSERT_EQUAL(0x0F, r.fault_op);
    TEST_ASSERT_EQUAL(0x101, (int)r.fault_ip);
    TEST_ASSERT_EQUAL(DOS_ERR_DIV, dos_run_com(div0, 4, out, sizeof(out), 500, &r));
    TEST_ASSERT_EQUAL(DOS_ERR_INT, dos_run_com(file, 4, out, sizeof(out), 500, &r));
    TEST_ASSERT_EQUAL(0x213D, r.fault_op);
    TEST_ASSERT_EQUAL(DOS_ERR_OPCODE, dos_run_com(port, 2, out, sizeof(out), 500, &r));
    TEST_ASSERT_EQUAL(DOS_ERR_SIZE, dos_run_com(spin, 0, out, sizeof(out), 500, &r));
    TEST_ASSERT_EQUAL_STRING("unsupported_service", dos_status_name(DOS_ERR_INT));
    /* output is truncated at cap, never overflows */
    TEST_ASSERT_EQUAL(DOS_EXITED, dos_run_com(hello, (int)sizeof(hello), out, 6, 1000, &r));
    TEST_ASSERT_EQUAL_STRING("Hello", out);
}

int main(void) {
    unity_init();
    RUN_TEST(test_hello);
    RUN_TEST(test_loops_and_arithmetic);
    RUN_TEST(test_call_ret_and_strings);
    RUN_TEST(test_more_opcodes);
    RUN_TEST(test_errors_are_reported);
    unity_print_results();
    unity_cleanup();
    return unity_stats.tests_failed == 0 ? 0 : 1;
}
