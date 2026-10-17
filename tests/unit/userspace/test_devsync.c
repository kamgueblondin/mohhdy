/* Lot 8 (US-083): multi-device sync (LWW map, version vectors, deltas). */
#include "../../framework/unity.h"
#include "../../../userspace/devsync.h"

static char buf[4096];
static ds_replica_t a, b, c;

static void reset(void) { ds_init(&a, 1); ds_init(&b, 2); ds_init(&c, 3); }
void setUp(void) {}
void tearDown(void) {}

static int sync_into(ds_replica_t* dst, const ds_replica_t* src) {
    if (ds_export(src, dst->vv, buf, (int)sizeof(buf)) <= 0) return -100;
    return ds_import(dst, buf);
}

static int str_eq(const char* x, const char* y) {
    if (!x || !y) return 0;
    while (*x && *x == *y) { x++; y++; }
    return *x == *y;
}

void test_local_writes_and_validation(void) {
    reset();
    TEST_ASSERT_EQUAL(0, ds_set(&a, "theme", "dark"));
    TEST_ASSERT_TRUE(str_eq(ds_get(&a, "theme"), "dark"));
    TEST_ASSERT_EQUAL(-1, ds_set(&a, "bad key", "x"));
    TEST_ASSERT_EQUAL(-1, ds_set(&a, "k", ""));
    TEST_ASSERT_EQUAL(-1, ds_set(&a, "averyveryverylongkey", "x"));
    TEST_ASSERT_EQUAL(-1, ds_del(&a, "absent"));
    TEST_ASSERT_EQUAL(0, ds_del(&a, "theme"));
    TEST_ASSERT_TRUE(ds_get(&a, "theme") == 0);
    TEST_ASSERT_EQUAL(0, ds_live(&a));
    TEST_ASSERT_TRUE(a.clock == 2U && a.vv[1] == 2U);
    TEST_ASSERT_EQUAL(-1, ds_init(&a, DS_DEVICES));
}

void test_three_devices_converge_in_any_order(void) {
    reset();
    ds_set(&a, "theme", "dark");
    ds_set(&b, "theme", "light");          /* concurrent: same clock, dev 2 wins */
    ds_set(&b, "lang", "fr");
    ds_set(&c, "wallpaper", "sea");
    ds_del(&c, "wallpaper");
    TEST_ASSERT_TRUE(sync_into(&a, &c) >= 1);
    TEST_ASSERT_TRUE(sync_into(&b, &a) >= 1);
    TEST_ASSERT_TRUE(sync_into(&c, &b) >= 1);
    sync_into(&a, &b);
    sync_into(&b, &c);
    TEST_ASSERT_TRUE(ds_digest(&a) == ds_digest(&b));
    TEST_ASSERT_TRUE(ds_digest(&b) == ds_digest(&c));
    TEST_ASSERT_TRUE(str_eq(ds_get(&a, "theme"), "light"));
    TEST_ASSERT_TRUE(str_eq(ds_get(&c, "lang"), "fr"));
    TEST_ASSERT_TRUE(ds_get(&b, "wallpaper") == 0);   /* tombstone travelled */
    TEST_ASSERT_TRUE(a.conflicts >= 1U);
}

void test_later_write_wins_and_merge_is_idempotent(void) {
    reset();
    uint32_t d;
    ds_set(&a, "k", "v1");
    sync_into(&b, &a);
    ds_set(&b, "k", "v2");                 /* b saw v1: clock 2 > 1 */
    sync_into(&a, &b);
    TEST_ASSERT_TRUE(str_eq(ds_get(&a, "k"), "v2"));
    d = ds_digest(&a);
    TEST_ASSERT_EQUAL(0, sync_into(&a, &b));  /* nothing new: empty delta */
    TEST_ASSERT_TRUE(ds_export(&b, 0, buf, (int)sizeof(buf)) > 0);
    TEST_ASSERT_EQUAL(0, ds_import(&a, buf));  /* full replay changes nothing */
    TEST_ASSERT_TRUE(d == ds_digest(&a));
}

void test_delta_only_carries_unseen_entries(void) {
    reset();
    int i, lines = 0;
    ds_set(&a, "x", "1");
    ds_set(&a, "y", "2");
    sync_into(&b, &a);
    ds_set(&a, "z", "3");
    TEST_ASSERT_TRUE(ds_export(&a, b.vv, buf, (int)sizeof(buf)) > 0);
    for (i = 0; buf[i]; i++) if (buf[i] == '\n' && buf[i + 1] == 'e') lines++;
    TEST_ASSERT_EQUAL(1, lines);
    TEST_ASSERT_EQUAL(1, ds_import(&b, buf));
    TEST_ASSERT_TRUE(str_eq(ds_get(&b, "z"), "3"));
}

void test_tampered_or_malformed_delta_is_refused(void) {
    reset();
    int n, i;
    ds_set(&a, "pay", "10");
    n = ds_export(&a, 0, buf, (int)sizeof(buf));
    TEST_ASSERT_TRUE(n > 0);
    for (i = 0; i < n; i++) if (buf[i] == '1' && buf[i + 1] == '0') { buf[i] = '9'; break; }
    TEST_ASSERT_EQUAL(-2, ds_import(&b, buf));
    TEST_ASSERT_TRUE(ds_get(&b, "pay") == 0);
    TEST_ASSERT_EQUAL(-1, ds_import(&b, "ds1 1 1\ne 1 1 0 k v\n"));      /* no sum */
    TEST_ASSERT_EQUAL(-1, ds_import(&b, "garbage"));
    TEST_ASSERT_EQUAL(-1, ds_export(&a, 0, buf, 16));
}

int main(void) {
    unity_init();
    RUN_TEST(test_local_writes_and_validation);
    RUN_TEST(test_three_devices_converge_in_any_order);
    RUN_TEST(test_later_write_wins_and_merge_is_idempotent);
    RUN_TEST(test_delta_only_carries_unseen_entries);
    RUN_TEST(test_tampered_or_malformed_delta_is_refused);
    unity_print_results();
    unity_cleanup();
    return unity_stats.tests_failed == 0 ? 0 : 1;
}
