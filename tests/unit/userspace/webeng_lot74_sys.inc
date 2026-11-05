/* Lot 74 generated systematic proofs (pattern, nesting, tables, iframes) */

static void test_l74_pat_0(void) {
    l74_open(L74_BAD "<form><input id=a pattern='[A-Z]+' value=1><input id=go type=submit></form>");
    TEST_ASSERT_EQUAL(1, l74_bad("a"));
    (void)l74_click("a");
    while (l74_val("a")[0]) l74_key(WE_KEY_BACKSPACE);
    l74_type("ABC");
    TEST_ASSERT_EQUAL(0, l74_bad("a"));
    l74_done();
}

static void test_l74_pat_1(void) {
    l74_open(L74_BAD "<form><input id=a pattern='\\d+' value=x><input id=go type=submit></form>");
    TEST_ASSERT_EQUAL(1, l74_bad("a"));
    (void)l74_click("a");
    while (l74_val("a")[0]) l74_key(WE_KEY_BACKSPACE);
    l74_type("42");
    TEST_ASSERT_EQUAL(0, l74_bad("a"));
    l74_done();
}

static void test_l74_pat_2(void) {
    l74_open(L74_BAD "<form><input id=a pattern='a.*z' value=ab><input id=go type=submit></form>");
    TEST_ASSERT_EQUAL(1, l74_bad("a"));
    (void)l74_click("a");
    while (l74_val("a")[0]) l74_key(WE_KEY_BACKSPACE);
    l74_type("abcz");
    TEST_ASSERT_EQUAL(0, l74_bad("a"));
    l74_done();
}

static void test_l74_pat_3(void) {
    l74_open(L74_BAD "<form><input id=a pattern='[abc][abc]' value=x><input id=go type=submit></form>");
    TEST_ASSERT_EQUAL(1, l74_bad("a"));
    (void)l74_click("a");
    while (l74_val("a")[0]) l74_key(WE_KEY_BACKSPACE);
    l74_type("ab");
    TEST_ASSERT_EQUAL(0, l74_bad("a"));
    l74_done();
}

static void test_l74_pat_4(void) {
    l74_open(L74_BAD "<form><input id=a pattern='x?' value=yy><input id=go type=submit></form>");
    TEST_ASSERT_EQUAL(1, l74_bad("a"));
    (void)l74_click("a");
    while (l74_val("a")[0]) l74_key(WE_KEY_BACKSPACE);
    l74_type("x");
    TEST_ASSERT_EQUAL(0, l74_bad("a"));
    l74_done();
}

static void test_l74_pat_5(void) {
    l74_open(L74_BAD "<form><input id=a pattern='a{2,5}' value=a><input id=go type=submit></form>");
    TEST_ASSERT_EQUAL(1, l74_bad("a"));
    (void)l74_click("a");
    while (l74_val("a")[0]) l74_key(WE_KEY_BACKSPACE);
    l74_type("aaa");
    TEST_ASSERT_EQUAL(0, l74_bad("a"));
    l74_done();
}

static void test_l74_pat_6(void) {
    l74_open(L74_BAD "<form><input id=a pattern='\\w{3}' value=ab><input id=go type=submit></form>");
    TEST_ASSERT_EQUAL(1, l74_bad("a"));
    (void)l74_click("a");
    while (l74_val("a")[0]) l74_key(WE_KEY_BACKSPACE);
    l74_type("abc");
    TEST_ASSERT_EQUAL(0, l74_bad("a"));
    l74_done();
}

static void test_l74_pat_7(void) {
    l74_open(L74_BAD "<form><input id=a pattern='[^0-9]+' value=1><input id=go type=submit></form>");
    TEST_ASSERT_EQUAL(1, l74_bad("a"));
    (void)l74_click("a");
    while (l74_val("a")[0]) l74_key(WE_KEY_BACKSPACE);
    l74_type("ab");
    TEST_ASSERT_EQUAL(0, l74_bad("a"));
    l74_done();
}

static void test_l74_pat_8(void) {
    l74_open(L74_BAD "<form><input id=a pattern='foo' value=f><input id=go type=submit></form>");
    TEST_ASSERT_EQUAL(1, l74_bad("a"));
    (void)l74_click("a");
    while (l74_val("a")[0]) l74_key(WE_KEY_BACKSPACE);
    l74_type("foo");
    TEST_ASSERT_EQUAL(0, l74_bad("a"));
    l74_done();
}

static void test_l74_pat_9(void) {
    l74_open(L74_BAD "<form><input id=a pattern='abc' value=ab><input id=go type=submit></form>");
    TEST_ASSERT_EQUAL(1, l74_bad("a"));
    (void)l74_click("a");
    while (l74_val("a")[0]) l74_key(WE_KEY_BACKSPACE);
    l74_type("abc");
    TEST_ASSERT_EQUAL(0, l74_bad("a"));
    l74_done();
}

static void test_l74_pat_10(void) {
    l74_open(L74_BAD "<form><input id=a pattern='[A-Z]+' value=1><input id=go type=submit></form>");
    TEST_ASSERT_EQUAL(1, l74_bad("a"));
    (void)l74_click("a");
    while (l74_val("a")[0]) l74_key(WE_KEY_BACKSPACE);
    l74_type("ABC");
    TEST_ASSERT_EQUAL(0, l74_bad("a"));
    l74_done();
}

static void test_l74_pat_11(void) {
    l74_open(L74_BAD "<form><input id=a pattern='\\d+' value=x><input id=go type=submit></form>");
    TEST_ASSERT_EQUAL(1, l74_bad("a"));
    (void)l74_click("a");
    while (l74_val("a")[0]) l74_key(WE_KEY_BACKSPACE);
    l74_type("42");
    TEST_ASSERT_EQUAL(0, l74_bad("a"));
    l74_done();
}

static void test_l74_pat_12(void) {
    l74_open(L74_BAD "<form><input id=a pattern='a.*z' value=ab><input id=go type=submit></form>");
    TEST_ASSERT_EQUAL(1, l74_bad("a"));
    (void)l74_click("a");
    while (l74_val("a")[0]) l74_key(WE_KEY_BACKSPACE);
    l74_type("abcz");
    TEST_ASSERT_EQUAL(0, l74_bad("a"));
    l74_done();
}

static void test_l74_pat_13(void) {
    l74_open(L74_BAD "<form><input id=a pattern='[abc][abc]' value=x><input id=go type=submit></form>");
    TEST_ASSERT_EQUAL(1, l74_bad("a"));
    (void)l74_click("a");
    while (l74_val("a")[0]) l74_key(WE_KEY_BACKSPACE);
    l74_type("ab");
    TEST_ASSERT_EQUAL(0, l74_bad("a"));
    l74_done();
}

static void test_l74_pat_14(void) {
    l74_open(L74_BAD "<form><input id=a pattern='x?' value=yy><input id=go type=submit></form>");
    TEST_ASSERT_EQUAL(1, l74_bad("a"));
    (void)l74_click("a");
    while (l74_val("a")[0]) l74_key(WE_KEY_BACKSPACE);
    l74_type("x");
    TEST_ASSERT_EQUAL(0, l74_bad("a"));
    l74_done();
}

static void test_l74_pat_15(void) {
    l74_open(L74_BAD "<form><input id=a pattern='a{2,5}' value=a><input id=go type=submit></form>");
    TEST_ASSERT_EQUAL(1, l74_bad("a"));
    (void)l74_click("a");
    while (l74_val("a")[0]) l74_key(WE_KEY_BACKSPACE);
    l74_type("aaa");
    TEST_ASSERT_EQUAL(0, l74_bad("a"));
    l74_done();
}

static void test_l74_pat_16(void) {
    l74_open(L74_BAD "<form><input id=a pattern='\\w{3}' value=ab><input id=go type=submit></form>");
    TEST_ASSERT_EQUAL(1, l74_bad("a"));
    (void)l74_click("a");
    while (l74_val("a")[0]) l74_key(WE_KEY_BACKSPACE);
    l74_type("abc");
    TEST_ASSERT_EQUAL(0, l74_bad("a"));
    l74_done();
}

static void test_l74_pat_17(void) {
    l74_open(L74_BAD "<form><input id=a pattern='[^0-9]+' value=1><input id=go type=submit></form>");
    TEST_ASSERT_EQUAL(1, l74_bad("a"));
    (void)l74_click("a");
    while (l74_val("a")[0]) l74_key(WE_KEY_BACKSPACE);
    l74_type("ab");
    TEST_ASSERT_EQUAL(0, l74_bad("a"));
    l74_done();
}

static void test_l74_pat_18(void) {
    l74_open(L74_BAD "<form><input id=a pattern='foo' value=f><input id=go type=submit></form>");
    TEST_ASSERT_EQUAL(1, l74_bad("a"));
    (void)l74_click("a");
    while (l74_val("a")[0]) l74_key(WE_KEY_BACKSPACE);
    l74_type("foo");
    TEST_ASSERT_EQUAL(0, l74_bad("a"));
    l74_done();
}

static void test_l74_pat_19(void) {
    l74_open(L74_BAD "<form><input id=a pattern='abc' value=ab><input id=go type=submit></form>");
    TEST_ASSERT_EQUAL(1, l74_bad("a"));
    (void)l74_click("a");
    while (l74_val("a")[0]) l74_key(WE_KEY_BACKSPACE);
    l74_type("abc");
    TEST_ASSERT_EQUAL(0, l74_bad("a"));
    l74_done();
}

static void test_l74_pat_20(void) {
    l74_open(L74_BAD "<form><input id=a pattern='[A-Z]+' value=1><input id=go type=submit></form>");
    TEST_ASSERT_EQUAL(1, l74_bad("a"));
    (void)l74_click("a");
    while (l74_val("a")[0]) l74_key(WE_KEY_BACKSPACE);
    l74_type("ABC");
    TEST_ASSERT_EQUAL(0, l74_bad("a"));
    l74_done();
}

static void test_l74_pat_21(void) {
    l74_open(L74_BAD "<form><input id=a pattern='\\d+' value=x><input id=go type=submit></form>");
    TEST_ASSERT_EQUAL(1, l74_bad("a"));
    (void)l74_click("a");
    while (l74_val("a")[0]) l74_key(WE_KEY_BACKSPACE);
    l74_type("42");
    TEST_ASSERT_EQUAL(0, l74_bad("a"));
    l74_done();
}

static void test_l74_pat_22(void) {
    l74_open(L74_BAD "<form><input id=a pattern='a.*z' value=ab><input id=go type=submit></form>");
    TEST_ASSERT_EQUAL(1, l74_bad("a"));
    (void)l74_click("a");
    while (l74_val("a")[0]) l74_key(WE_KEY_BACKSPACE);
    l74_type("abcz");
    TEST_ASSERT_EQUAL(0, l74_bad("a"));
    l74_done();
}

static void test_l74_pat_23(void) {
    l74_open(L74_BAD "<form><input id=a pattern='[abc][abc]' value=x><input id=go type=submit></form>");
    TEST_ASSERT_EQUAL(1, l74_bad("a"));
    (void)l74_click("a");
    while (l74_val("a")[0]) l74_key(WE_KEY_BACKSPACE);
    l74_type("ab");
    TEST_ASSERT_EQUAL(0, l74_bad("a"));
    l74_done();
}

static void test_l74_pat_24(void) {
    l74_open(L74_BAD "<form><input id=a pattern='x?' value=yy><input id=go type=submit></form>");
    TEST_ASSERT_EQUAL(1, l74_bad("a"));
    (void)l74_click("a");
    while (l74_val("a")[0]) l74_key(WE_KEY_BACKSPACE);
    l74_type("x");
    TEST_ASSERT_EQUAL(0, l74_bad("a"));
    l74_done();
}

static void test_l74_pat_25(void) {
    l74_open(L74_BAD "<form><input id=a pattern='a{2,5}' value=a><input id=go type=submit></form>");
    TEST_ASSERT_EQUAL(1, l74_bad("a"));
    (void)l74_click("a");
    while (l74_val("a")[0]) l74_key(WE_KEY_BACKSPACE);
    l74_type("aaa");
    TEST_ASSERT_EQUAL(0, l74_bad("a"));
    l74_done();
}

static void test_l74_pat_26(void) {
    l74_open(L74_BAD "<form><input id=a pattern='\\w{3}' value=ab><input id=go type=submit></form>");
    TEST_ASSERT_EQUAL(1, l74_bad("a"));
    (void)l74_click("a");
    while (l74_val("a")[0]) l74_key(WE_KEY_BACKSPACE);
    l74_type("abc");
    TEST_ASSERT_EQUAL(0, l74_bad("a"));
    l74_done();
}

static void test_l74_pat_27(void) {
    l74_open(L74_BAD "<form><input id=a pattern='[^0-9]+' value=1><input id=go type=submit></form>");
    TEST_ASSERT_EQUAL(1, l74_bad("a"));
    (void)l74_click("a");
    while (l74_val("a")[0]) l74_key(WE_KEY_BACKSPACE);
    l74_type("ab");
    TEST_ASSERT_EQUAL(0, l74_bad("a"));
    l74_done();
}

static void test_l74_pat_28(void) {
    l74_open(L74_BAD "<form><input id=a pattern='foo' value=f><input id=go type=submit></form>");
    TEST_ASSERT_EQUAL(1, l74_bad("a"));
    (void)l74_click("a");
    while (l74_val("a")[0]) l74_key(WE_KEY_BACKSPACE);
    l74_type("foo");
    TEST_ASSERT_EQUAL(0, l74_bad("a"));
    l74_done();
}

static void test_l74_pat_29(void) {
    l74_open(L74_BAD "<form><input id=a pattern='abc' value=ab><input id=go type=submit></form>");
    TEST_ASSERT_EQUAL(1, l74_bad("a"));
    (void)l74_click("a");
    while (l74_val("a")[0]) l74_key(WE_KEY_BACKSPACE);
    l74_type("abc");
    TEST_ASSERT_EQUAL(0, l74_bad("a"));
    l74_done();
}

static void test_l74_pat_30(void) {
    l74_open(L74_BAD "<form><input id=a pattern='[A-Z]+' value=1><input id=go type=submit></form>");
    TEST_ASSERT_EQUAL(1, l74_bad("a"));
    (void)l74_click("a");
    while (l74_val("a")[0]) l74_key(WE_KEY_BACKSPACE);
    l74_type("ABC");
    TEST_ASSERT_EQUAL(0, l74_bad("a"));
    l74_done();
}

static void test_l74_pat_31(void) {
    l74_open(L74_BAD "<form><input id=a pattern='\\d+' value=x><input id=go type=submit></form>");
    TEST_ASSERT_EQUAL(1, l74_bad("a"));
    (void)l74_click("a");
    while (l74_val("a")[0]) l74_key(WE_KEY_BACKSPACE);
    l74_type("42");
    TEST_ASSERT_EQUAL(0, l74_bad("a"));
    l74_done();
}

static void test_l74_pat_32(void) {
    l74_open(L74_BAD "<form><input id=a pattern='a.*z' value=ab><input id=go type=submit></form>");
    TEST_ASSERT_EQUAL(1, l74_bad("a"));
    (void)l74_click("a");
    while (l74_val("a")[0]) l74_key(WE_KEY_BACKSPACE);
    l74_type("abcz");
    TEST_ASSERT_EQUAL(0, l74_bad("a"));
    l74_done();
}

static void test_l74_pat_33(void) {
    l74_open(L74_BAD "<form><input id=a pattern='[abc][abc]' value=x><input id=go type=submit></form>");
    TEST_ASSERT_EQUAL(1, l74_bad("a"));
    (void)l74_click("a");
    while (l74_val("a")[0]) l74_key(WE_KEY_BACKSPACE);
    l74_type("ab");
    TEST_ASSERT_EQUAL(0, l74_bad("a"));
    l74_done();
}

static void test_l74_pat_34(void) {
    l74_open(L74_BAD "<form><input id=a pattern='x?' value=yy><input id=go type=submit></form>");
    TEST_ASSERT_EQUAL(1, l74_bad("a"));
    (void)l74_click("a");
    while (l74_val("a")[0]) l74_key(WE_KEY_BACKSPACE);
    l74_type("x");
    TEST_ASSERT_EQUAL(0, l74_bad("a"));
    l74_done();
}

static void test_l74_pat_35(void) {
    l74_open(L74_BAD "<form><input id=a pattern='a{2,5}' value=a><input id=go type=submit></form>");
    TEST_ASSERT_EQUAL(1, l74_bad("a"));
    (void)l74_click("a");
    while (l74_val("a")[0]) l74_key(WE_KEY_BACKSPACE);
    l74_type("aaa");
    TEST_ASSERT_EQUAL(0, l74_bad("a"));
    l74_done();
}

static void test_l74_pat_36(void) {
    l74_open(L74_BAD "<form><input id=a pattern='\\w{3}' value=ab><input id=go type=submit></form>");
    TEST_ASSERT_EQUAL(1, l74_bad("a"));
    (void)l74_click("a");
    while (l74_val("a")[0]) l74_key(WE_KEY_BACKSPACE);
    l74_type("abc");
    TEST_ASSERT_EQUAL(0, l74_bad("a"));
    l74_done();
}

static void test_l74_pat_37(void) {
    l74_open(L74_BAD "<form><input id=a pattern='[^0-9]+' value=1><input id=go type=submit></form>");
    TEST_ASSERT_EQUAL(1, l74_bad("a"));
    (void)l74_click("a");
    while (l74_val("a")[0]) l74_key(WE_KEY_BACKSPACE);
    l74_type("ab");
    TEST_ASSERT_EQUAL(0, l74_bad("a"));
    l74_done();
}

static void test_l74_pat_38(void) {
    l74_open(L74_BAD "<form><input id=a pattern='foo' value=f><input id=go type=submit></form>");
    TEST_ASSERT_EQUAL(1, l74_bad("a"));
    (void)l74_click("a");
    while (l74_val("a")[0]) l74_key(WE_KEY_BACKSPACE);
    l74_type("foo");
    TEST_ASSERT_EQUAL(0, l74_bad("a"));
    l74_done();
}

static void test_l74_pat_39(void) {
    l74_open(L74_BAD "<form><input id=a pattern='abc' value=ab><input id=go type=submit></form>");
    TEST_ASSERT_EQUAL(1, l74_bad("a"));
    (void)l74_click("a");
    while (l74_val("a")[0]) l74_key(WE_KEY_BACKSPACE);
    l74_type("abc");
    TEST_ASSERT_EQUAL(0, l74_bad("a"));
    l74_done();
}

static void test_l74_pat_40(void) {
    l74_open(L74_BAD "<form><input id=a pattern='[A-Z]+' value=1><input id=go type=submit></form>");
    TEST_ASSERT_EQUAL(1, l74_bad("a"));
    (void)l74_click("a");
    while (l74_val("a")[0]) l74_key(WE_KEY_BACKSPACE);
    l74_type("ABC");
    TEST_ASSERT_EQUAL(0, l74_bad("a"));
    l74_done();
}

static void test_l74_pat_41(void) {
    l74_open(L74_BAD "<form><input id=a pattern='\\d+' value=x><input id=go type=submit></form>");
    TEST_ASSERT_EQUAL(1, l74_bad("a"));
    (void)l74_click("a");
    while (l74_val("a")[0]) l74_key(WE_KEY_BACKSPACE);
    l74_type("42");
    TEST_ASSERT_EQUAL(0, l74_bad("a"));
    l74_done();
}

static void test_l74_pat_42(void) {
    l74_open(L74_BAD "<form><input id=a pattern='a.*z' value=ab><input id=go type=submit></form>");
    TEST_ASSERT_EQUAL(1, l74_bad("a"));
    (void)l74_click("a");
    while (l74_val("a")[0]) l74_key(WE_KEY_BACKSPACE);
    l74_type("abcz");
    TEST_ASSERT_EQUAL(0, l74_bad("a"));
    l74_done();
}

static void test_l74_pat_43(void) {
    l74_open(L74_BAD "<form><input id=a pattern='[abc][abc]' value=x><input id=go type=submit></form>");
    TEST_ASSERT_EQUAL(1, l74_bad("a"));
    (void)l74_click("a");
    while (l74_val("a")[0]) l74_key(WE_KEY_BACKSPACE);
    l74_type("ab");
    TEST_ASSERT_EQUAL(0, l74_bad("a"));
    l74_done();
}

static void test_l74_pat_44(void) {
    l74_open(L74_BAD "<form><input id=a pattern='x?' value=yy><input id=go type=submit></form>");
    TEST_ASSERT_EQUAL(1, l74_bad("a"));
    (void)l74_click("a");
    while (l74_val("a")[0]) l74_key(WE_KEY_BACKSPACE);
    l74_type("x");
    TEST_ASSERT_EQUAL(0, l74_bad("a"));
    l74_done();
}

static void test_l74_pat_45(void) {
    l74_open(L74_BAD "<form><input id=a pattern='a{2,5}' value=a><input id=go type=submit></form>");
    TEST_ASSERT_EQUAL(1, l74_bad("a"));
    (void)l74_click("a");
    while (l74_val("a")[0]) l74_key(WE_KEY_BACKSPACE);
    l74_type("aaa");
    TEST_ASSERT_EQUAL(0, l74_bad("a"));
    l74_done();
}

static void test_l74_pat_46(void) {
    l74_open(L74_BAD "<form><input id=a pattern='\\w{3}' value=ab><input id=go type=submit></form>");
    TEST_ASSERT_EQUAL(1, l74_bad("a"));
    (void)l74_click("a");
    while (l74_val("a")[0]) l74_key(WE_KEY_BACKSPACE);
    l74_type("abc");
    TEST_ASSERT_EQUAL(0, l74_bad("a"));
    l74_done();
}

static void test_l74_pat_47(void) {
    l74_open(L74_BAD "<form><input id=a pattern='[^0-9]+' value=1><input id=go type=submit></form>");
    TEST_ASSERT_EQUAL(1, l74_bad("a"));
    (void)l74_click("a");
    while (l74_val("a")[0]) l74_key(WE_KEY_BACKSPACE);
    l74_type("ab");
    TEST_ASSERT_EQUAL(0, l74_bad("a"));
    l74_done();
}

static void test_l74_pat_48(void) {
    l74_open(L74_BAD "<form><input id=a pattern='foo' value=f><input id=go type=submit></form>");
    TEST_ASSERT_EQUAL(1, l74_bad("a"));
    (void)l74_click("a");
    while (l74_val("a")[0]) l74_key(WE_KEY_BACKSPACE);
    l74_type("foo");
    TEST_ASSERT_EQUAL(0, l74_bad("a"));
    l74_done();
}

static void test_l74_pat_49(void) {
    l74_open(L74_BAD "<form><input id=a pattern='abc' value=ab><input id=go type=submit></form>");
    TEST_ASSERT_EQUAL(1, l74_bad("a"));
    (void)l74_click("a");
    while (l74_val("a")[0]) l74_key(WE_KEY_BACKSPACE);
    l74_type("abc");
    TEST_ASSERT_EQUAL(0, l74_bad("a"));
    l74_done();
}

static void test_l74_pat_50(void) {
    l74_open(L74_BAD "<form><input id=a pattern='[A-Z]+' value=1><input id=go type=submit></form>");
    TEST_ASSERT_EQUAL(1, l74_bad("a"));
    (void)l74_click("a");
    while (l74_val("a")[0]) l74_key(WE_KEY_BACKSPACE);
    l74_type("ABC");
    TEST_ASSERT_EQUAL(0, l74_bad("a"));
    l74_done();
}

static void test_l74_pat_51(void) {
    l74_open(L74_BAD "<form><input id=a pattern='\\d+' value=x><input id=go type=submit></form>");
    TEST_ASSERT_EQUAL(1, l74_bad("a"));
    (void)l74_click("a");
    while (l74_val("a")[0]) l74_key(WE_KEY_BACKSPACE);
    l74_type("42");
    TEST_ASSERT_EQUAL(0, l74_bad("a"));
    l74_done();
}

static void test_l74_pat_52(void) {
    l74_open(L74_BAD "<form><input id=a pattern='a.*z' value=ab><input id=go type=submit></form>");
    TEST_ASSERT_EQUAL(1, l74_bad("a"));
    (void)l74_click("a");
    while (l74_val("a")[0]) l74_key(WE_KEY_BACKSPACE);
    l74_type("abcz");
    TEST_ASSERT_EQUAL(0, l74_bad("a"));
    l74_done();
}

static void test_l74_pat_53(void) {
    l74_open(L74_BAD "<form><input id=a pattern='[abc][abc]' value=x><input id=go type=submit></form>");
    TEST_ASSERT_EQUAL(1, l74_bad("a"));
    (void)l74_click("a");
    while (l74_val("a")[0]) l74_key(WE_KEY_BACKSPACE);
    l74_type("ab");
    TEST_ASSERT_EQUAL(0, l74_bad("a"));
    l74_done();
}

static void test_l74_pat_54(void) {
    l74_open(L74_BAD "<form><input id=a pattern='x?' value=yy><input id=go type=submit></form>");
    TEST_ASSERT_EQUAL(1, l74_bad("a"));
    (void)l74_click("a");
    while (l74_val("a")[0]) l74_key(WE_KEY_BACKSPACE);
    l74_type("x");
    TEST_ASSERT_EQUAL(0, l74_bad("a"));
    l74_done();
}

static void test_l74_pat_55(void) {
    l74_open(L74_BAD "<form><input id=a pattern='a{2,5}' value=a><input id=go type=submit></form>");
    TEST_ASSERT_EQUAL(1, l74_bad("a"));
    (void)l74_click("a");
    while (l74_val("a")[0]) l74_key(WE_KEY_BACKSPACE);
    l74_type("aaa");
    TEST_ASSERT_EQUAL(0, l74_bad("a"));
    l74_done();
}

static void test_l74_pat_56(void) {
    l74_open(L74_BAD "<form><input id=a pattern='\\w{3}' value=ab><input id=go type=submit></form>");
    TEST_ASSERT_EQUAL(1, l74_bad("a"));
    (void)l74_click("a");
    while (l74_val("a")[0]) l74_key(WE_KEY_BACKSPACE);
    l74_type("abc");
    TEST_ASSERT_EQUAL(0, l74_bad("a"));
    l74_done();
}

static void test_l74_pat_57(void) {
    l74_open(L74_BAD "<form><input id=a pattern='[^0-9]+' value=1><input id=go type=submit></form>");
    TEST_ASSERT_EQUAL(1, l74_bad("a"));
    (void)l74_click("a");
    while (l74_val("a")[0]) l74_key(WE_KEY_BACKSPACE);
    l74_type("ab");
    TEST_ASSERT_EQUAL(0, l74_bad("a"));
    l74_done();
}

static void test_l74_pat_58(void) {
    l74_open(L74_BAD "<form><input id=a pattern='foo' value=f><input id=go type=submit></form>");
    TEST_ASSERT_EQUAL(1, l74_bad("a"));
    (void)l74_click("a");
    while (l74_val("a")[0]) l74_key(WE_KEY_BACKSPACE);
    l74_type("foo");
    TEST_ASSERT_EQUAL(0, l74_bad("a"));
    l74_done();
}

static void test_l74_pat_59(void) {
    l74_open(L74_BAD "<form><input id=a pattern='abc' value=ab><input id=go type=submit></form>");
    TEST_ASSERT_EQUAL(1, l74_bad("a"));
    (void)l74_click("a");
    while (l74_val("a")[0]) l74_key(WE_KEY_BACKSPACE);
    l74_type("abc");
    TEST_ASSERT_EQUAL(0, l74_bad("a"));
    l74_done();
}

static void test_l74_pat_60(void) {
    l74_open(L74_BAD "<form><input id=a pattern='[A-Z]+' value=1><input id=go type=submit></form>");
    TEST_ASSERT_EQUAL(1, l74_bad("a"));
    (void)l74_click("a");
    while (l74_val("a")[0]) l74_key(WE_KEY_BACKSPACE);
    l74_type("ABC");
    TEST_ASSERT_EQUAL(0, l74_bad("a"));
    l74_done();
}

static void test_l74_pat_61(void) {
    l74_open(L74_BAD "<form><input id=a pattern='\\d+' value=x><input id=go type=submit></form>");
    TEST_ASSERT_EQUAL(1, l74_bad("a"));
    (void)l74_click("a");
    while (l74_val("a")[0]) l74_key(WE_KEY_BACKSPACE);
    l74_type("42");
    TEST_ASSERT_EQUAL(0, l74_bad("a"));
    l74_done();
}

static void test_l74_pat_62(void) {
    l74_open(L74_BAD "<form><input id=a pattern='a.*z' value=ab><input id=go type=submit></form>");
    TEST_ASSERT_EQUAL(1, l74_bad("a"));
    (void)l74_click("a");
    while (l74_val("a")[0]) l74_key(WE_KEY_BACKSPACE);
    l74_type("abcz");
    TEST_ASSERT_EQUAL(0, l74_bad("a"));
    l74_done();
}

static void test_l74_pat_63(void) {
    l74_open(L74_BAD "<form><input id=a pattern='[abc][abc]' value=x><input id=go type=submit></form>");
    TEST_ASSERT_EQUAL(1, l74_bad("a"));
    (void)l74_click("a");
    while (l74_val("a")[0]) l74_key(WE_KEY_BACKSPACE);
    l74_type("ab");
    TEST_ASSERT_EQUAL(0, l74_bad("a"));
    l74_done();
}

static void test_l74_pat_64(void) {
    l74_open(L74_BAD "<form><input id=a pattern='x?' value=yy><input id=go type=submit></form>");
    TEST_ASSERT_EQUAL(1, l74_bad("a"));
    (void)l74_click("a");
    while (l74_val("a")[0]) l74_key(WE_KEY_BACKSPACE);
    l74_type("x");
    TEST_ASSERT_EQUAL(0, l74_bad("a"));
    l74_done();
}

static void test_l74_pat_65(void) {
    l74_open(L74_BAD "<form><input id=a pattern='a{2,5}' value=a><input id=go type=submit></form>");
    TEST_ASSERT_EQUAL(1, l74_bad("a"));
    (void)l74_click("a");
    while (l74_val("a")[0]) l74_key(WE_KEY_BACKSPACE);
    l74_type("aaa");
    TEST_ASSERT_EQUAL(0, l74_bad("a"));
    l74_done();
}

static void test_l74_pat_66(void) {
    l74_open(L74_BAD "<form><input id=a pattern='\\w{3}' value=ab><input id=go type=submit></form>");
    TEST_ASSERT_EQUAL(1, l74_bad("a"));
    (void)l74_click("a");
    while (l74_val("a")[0]) l74_key(WE_KEY_BACKSPACE);
    l74_type("abc");
    TEST_ASSERT_EQUAL(0, l74_bad("a"));
    l74_done();
}

static void test_l74_pat_67(void) {
    l74_open(L74_BAD "<form><input id=a pattern='[^0-9]+' value=1><input id=go type=submit></form>");
    TEST_ASSERT_EQUAL(1, l74_bad("a"));
    (void)l74_click("a");
    while (l74_val("a")[0]) l74_key(WE_KEY_BACKSPACE);
    l74_type("ab");
    TEST_ASSERT_EQUAL(0, l74_bad("a"));
    l74_done();
}

static void test_l74_pat_68(void) {
    l74_open(L74_BAD "<form><input id=a pattern='foo' value=f><input id=go type=submit></form>");
    TEST_ASSERT_EQUAL(1, l74_bad("a"));
    (void)l74_click("a");
    while (l74_val("a")[0]) l74_key(WE_KEY_BACKSPACE);
    l74_type("foo");
    TEST_ASSERT_EQUAL(0, l74_bad("a"));
    l74_done();
}

static void test_l74_pat_69(void) {
    l74_open(L74_BAD "<form><input id=a pattern='abc' value=ab><input id=go type=submit></form>");
    TEST_ASSERT_EQUAL(1, l74_bad("a"));
    (void)l74_click("a");
    while (l74_val("a")[0]) l74_key(WE_KEY_BACKSPACE);
    l74_type("abc");
    TEST_ASSERT_EQUAL(0, l74_bad("a"));
    l74_done();
}

static void test_l74_pat_70(void) {
    l74_open(L74_BAD "<form><input id=a pattern='[A-Z]+' value=1><input id=go type=submit></form>");
    TEST_ASSERT_EQUAL(1, l74_bad("a"));
    (void)l74_click("a");
    while (l74_val("a")[0]) l74_key(WE_KEY_BACKSPACE);
    l74_type("ABC");
    TEST_ASSERT_EQUAL(0, l74_bad("a"));
    l74_done();
}

static void test_l74_pat_71(void) {
    l74_open(L74_BAD "<form><input id=a pattern='\\d+' value=x><input id=go type=submit></form>");
    TEST_ASSERT_EQUAL(1, l74_bad("a"));
    (void)l74_click("a");
    while (l74_val("a")[0]) l74_key(WE_KEY_BACKSPACE);
    l74_type("42");
    TEST_ASSERT_EQUAL(0, l74_bad("a"));
    l74_done();
}

static void test_l74_pat_72(void) {
    l74_open(L74_BAD "<form><input id=a pattern='a.*z' value=ab><input id=go type=submit></form>");
    TEST_ASSERT_EQUAL(1, l74_bad("a"));
    (void)l74_click("a");
    while (l74_val("a")[0]) l74_key(WE_KEY_BACKSPACE);
    l74_type("abcz");
    TEST_ASSERT_EQUAL(0, l74_bad("a"));
    l74_done();
}

static void test_l74_pat_73(void) {
    l74_open(L74_BAD "<form><input id=a pattern='[abc][abc]' value=x><input id=go type=submit></form>");
    TEST_ASSERT_EQUAL(1, l74_bad("a"));
    (void)l74_click("a");
    while (l74_val("a")[0]) l74_key(WE_KEY_BACKSPACE);
    l74_type("ab");
    TEST_ASSERT_EQUAL(0, l74_bad("a"));
    l74_done();
}

static void test_l74_pat_74(void) {
    l74_open(L74_BAD "<form><input id=a pattern='x?' value=yy><input id=go type=submit></form>");
    TEST_ASSERT_EQUAL(1, l74_bad("a"));
    (void)l74_click("a");
    while (l74_val("a")[0]) l74_key(WE_KEY_BACKSPACE);
    l74_type("x");
    TEST_ASSERT_EQUAL(0, l74_bad("a"));
    l74_done();
}

static void test_l74_pat_75(void) {
    l74_open(L74_BAD "<form><input id=a pattern='a{2,5}' value=a><input id=go type=submit></form>");
    TEST_ASSERT_EQUAL(1, l74_bad("a"));
    (void)l74_click("a");
    while (l74_val("a")[0]) l74_key(WE_KEY_BACKSPACE);
    l74_type("aaa");
    TEST_ASSERT_EQUAL(0, l74_bad("a"));
    l74_done();
}

static void test_l74_pat_76(void) {
    l74_open(L74_BAD "<form><input id=a pattern='\\w{3}' value=ab><input id=go type=submit></form>");
    TEST_ASSERT_EQUAL(1, l74_bad("a"));
    (void)l74_click("a");
    while (l74_val("a")[0]) l74_key(WE_KEY_BACKSPACE);
    l74_type("abc");
    TEST_ASSERT_EQUAL(0, l74_bad("a"));
    l74_done();
}

static void test_l74_pat_77(void) {
    l74_open(L74_BAD "<form><input id=a pattern='[^0-9]+' value=1><input id=go type=submit></form>");
    TEST_ASSERT_EQUAL(1, l74_bad("a"));
    (void)l74_click("a");
    while (l74_val("a")[0]) l74_key(WE_KEY_BACKSPACE);
    l74_type("ab");
    TEST_ASSERT_EQUAL(0, l74_bad("a"));
    l74_done();
}

static void test_l74_pat_78(void) {
    l74_open(L74_BAD "<form><input id=a pattern='foo' value=f><input id=go type=submit></form>");
    TEST_ASSERT_EQUAL(1, l74_bad("a"));
    (void)l74_click("a");
    while (l74_val("a")[0]) l74_key(WE_KEY_BACKSPACE);
    l74_type("foo");
    TEST_ASSERT_EQUAL(0, l74_bad("a"));
    l74_done();
}

static void test_l74_pat_79(void) {
    l74_open(L74_BAD "<form><input id=a pattern='abc' value=ab><input id=go type=submit></form>");
    TEST_ASSERT_EQUAL(1, l74_bad("a"));
    (void)l74_click("a");
    while (l74_val("a")[0]) l74_key(WE_KEY_BACKSPACE);
    l74_type("abc");
    TEST_ASSERT_EQUAL(0, l74_bad("a"));
    l74_done();
}

static void test_l74_nestg_0(void) {
    lot52_render("<style>.c0{color:#000000; & span{color:#000001} &.on{color:#000001}}</style>"
                 L64_BODY "<div class='c0' id=a>a<span id=b>b</span></div>"
                 "<div class='c0 on' id=c>c</div></body>", 400);
    TEST_ASSERT_EQUAL(1, l73c("b"));
    TEST_ASSERT_EQUAL(1, l73c("c"));
}

static void test_l74_nestg_1(void) {
    lot52_render("<style>.c1{color:#000000; & span{color:#000002} &.on{color:#000002}}</style>"
                 L64_BODY "<div class='c1' id=a>a<span id=b>b</span></div>"
                 "<div class='c1 on' id=c>c</div></body>", 400);
    TEST_ASSERT_EQUAL(2, l73c("b"));
    TEST_ASSERT_EQUAL(2, l73c("c"));
}

static void test_l74_nestg_2(void) {
    lot52_render("<style>.c2{color:#000000; & span{color:#000003} &.on{color:#000003}}</style>"
                 L64_BODY "<div class='c2' id=a>a<span id=b>b</span></div>"
                 "<div class='c2 on' id=c>c</div></body>", 400);
    TEST_ASSERT_EQUAL(3, l73c("b"));
    TEST_ASSERT_EQUAL(3, l73c("c"));
}

static void test_l74_nestg_3(void) {
    lot52_render("<style>.c3{color:#000000; & span{color:#000004} &.on{color:#000004}}</style>"
                 L64_BODY "<div class='c3' id=a>a<span id=b>b</span></div>"
                 "<div class='c3 on' id=c>c</div></body>", 400);
    TEST_ASSERT_EQUAL(4, l73c("b"));
    TEST_ASSERT_EQUAL(4, l73c("c"));
}

static void test_l74_nestg_4(void) {
    lot52_render("<style>.c4{color:#000000; & span{color:#000005} &.on{color:#000005}}</style>"
                 L64_BODY "<div class='c4' id=a>a<span id=b>b</span></div>"
                 "<div class='c4 on' id=c>c</div></body>", 400);
    TEST_ASSERT_EQUAL(5, l73c("b"));
    TEST_ASSERT_EQUAL(5, l73c("c"));
}

static void test_l74_nestg_5(void) {
    lot52_render("<style>.c5{color:#000000; & span{color:#000006} &.on{color:#000006}}</style>"
                 L64_BODY "<div class='c5' id=a>a<span id=b>b</span></div>"
                 "<div class='c5 on' id=c>c</div></body>", 400);
    TEST_ASSERT_EQUAL(6, l73c("b"));
    TEST_ASSERT_EQUAL(6, l73c("c"));
}

static void test_l74_nestg_6(void) {
    lot52_render("<style>.c6{color:#000000; & span{color:#000007} &.on{color:#000007}}</style>"
                 L64_BODY "<div class='c6' id=a>a<span id=b>b</span></div>"
                 "<div class='c6 on' id=c>c</div></body>", 400);
    TEST_ASSERT_EQUAL(7, l73c("b"));
    TEST_ASSERT_EQUAL(7, l73c("c"));
}

static void test_l74_nestg_7(void) {
    lot52_render("<style>.c7{color:#000000; & span{color:#000008} &.on{color:#000008}}</style>"
                 L64_BODY "<div class='c7' id=a>a<span id=b>b</span></div>"
                 "<div class='c7 on' id=c>c</div></body>", 400);
    TEST_ASSERT_EQUAL(8, l73c("b"));
    TEST_ASSERT_EQUAL(8, l73c("c"));
}

static void test_l74_nestg_8(void) {
    lot52_render("<style>.c8{color:#000000; & span{color:#000009} &.on{color:#000009}}</style>"
                 L64_BODY "<div class='c8' id=a>a<span id=b>b</span></div>"
                 "<div class='c8 on' id=c>c</div></body>", 400);
    TEST_ASSERT_EQUAL(9, l73c("b"));
    TEST_ASSERT_EQUAL(9, l73c("c"));
}

static void test_l74_nestg_9(void) {
    lot52_render("<style>.c9{color:#000000; & span{color:#000001} &.on{color:#000001}}</style>"
                 L64_BODY "<div class='c9' id=a>a<span id=b>b</span></div>"
                 "<div class='c9 on' id=c>c</div></body>", 400);
    TEST_ASSERT_EQUAL(1, l73c("b"));
    TEST_ASSERT_EQUAL(1, l73c("c"));
}

static void test_l74_nestg_10(void) {
    lot52_render("<style>.c10{color:#000000; & span{color:#000002} &.on{color:#000002}}</style>"
                 L64_BODY "<div class='c10' id=a>a<span id=b>b</span></div>"
                 "<div class='c10 on' id=c>c</div></body>", 400);
    TEST_ASSERT_EQUAL(2, l73c("b"));
    TEST_ASSERT_EQUAL(2, l73c("c"));
}

static void test_l74_nestg_11(void) {
    lot52_render("<style>.c11{color:#000000; & span{color:#000003} &.on{color:#000003}}</style>"
                 L64_BODY "<div class='c11' id=a>a<span id=b>b</span></div>"
                 "<div class='c11 on' id=c>c</div></body>", 400);
    TEST_ASSERT_EQUAL(3, l73c("b"));
    TEST_ASSERT_EQUAL(3, l73c("c"));
}

static void test_l74_nestg_12(void) {
    lot52_render("<style>.c12{color:#000000; & span{color:#000004} &.on{color:#000004}}</style>"
                 L64_BODY "<div class='c12' id=a>a<span id=b>b</span></div>"
                 "<div class='c12 on' id=c>c</div></body>", 400);
    TEST_ASSERT_EQUAL(4, l73c("b"));
    TEST_ASSERT_EQUAL(4, l73c("c"));
}

static void test_l74_nestg_13(void) {
    lot52_render("<style>.c13{color:#000000; & span{color:#000005} &.on{color:#000005}}</style>"
                 L64_BODY "<div class='c13' id=a>a<span id=b>b</span></div>"
                 "<div class='c13 on' id=c>c</div></body>", 400);
    TEST_ASSERT_EQUAL(5, l73c("b"));
    TEST_ASSERT_EQUAL(5, l73c("c"));
}

static void test_l74_nestg_14(void) {
    lot52_render("<style>.c14{color:#000000; & span{color:#000006} &.on{color:#000006}}</style>"
                 L64_BODY "<div class='c14' id=a>a<span id=b>b</span></div>"
                 "<div class='c14 on' id=c>c</div></body>", 400);
    TEST_ASSERT_EQUAL(6, l73c("b"));
    TEST_ASSERT_EQUAL(6, l73c("c"));
}

static void test_l74_nestg_15(void) {
    lot52_render("<style>.c15{color:#000000; & span{color:#000007} &.on{color:#000007}}</style>"
                 L64_BODY "<div class='c15' id=a>a<span id=b>b</span></div>"
                 "<div class='c15 on' id=c>c</div></body>", 400);
    TEST_ASSERT_EQUAL(7, l73c("b"));
    TEST_ASSERT_EQUAL(7, l73c("c"));
}

static void test_l74_nestg_16(void) {
    lot52_render("<style>.c16{color:#000000; & span{color:#000008} &.on{color:#000008}}</style>"
                 L64_BODY "<div class='c16' id=a>a<span id=b>b</span></div>"
                 "<div class='c16 on' id=c>c</div></body>", 400);
    TEST_ASSERT_EQUAL(8, l73c("b"));
    TEST_ASSERT_EQUAL(8, l73c("c"));
}

static void test_l74_nestg_17(void) {
    lot52_render("<style>.c17{color:#000000; & span{color:#000009} &.on{color:#000009}}</style>"
                 L64_BODY "<div class='c17' id=a>a<span id=b>b</span></div>"
                 "<div class='c17 on' id=c>c</div></body>", 400);
    TEST_ASSERT_EQUAL(9, l73c("b"));
    TEST_ASSERT_EQUAL(9, l73c("c"));
}

static void test_l74_nestg_18(void) {
    lot52_render("<style>.c18{color:#000000; & span{color:#000001} &.on{color:#000001}}</style>"
                 L64_BODY "<div class='c18' id=a>a<span id=b>b</span></div>"
                 "<div class='c18 on' id=c>c</div></body>", 400);
    TEST_ASSERT_EQUAL(1, l73c("b"));
    TEST_ASSERT_EQUAL(1, l73c("c"));
}

static void test_l74_nestg_19(void) {
    lot52_render("<style>.c19{color:#000000; & span{color:#000002} &.on{color:#000002}}</style>"
                 L64_BODY "<div class='c19' id=a>a<span id=b>b</span></div>"
                 "<div class='c19 on' id=c>c</div></body>", 400);
    TEST_ASSERT_EQUAL(2, l73c("b"));
    TEST_ASSERT_EQUAL(2, l73c("c"));
}

static void test_l74_nestg_20(void) {
    lot52_render("<style>.c20{color:#000000; & span{color:#000003} &.on{color:#000003}}</style>"
                 L64_BODY "<div class='c20' id=a>a<span id=b>b</span></div>"
                 "<div class='c20 on' id=c>c</div></body>", 400);
    TEST_ASSERT_EQUAL(3, l73c("b"));
    TEST_ASSERT_EQUAL(3, l73c("c"));
}

static void test_l74_nestg_21(void) {
    lot52_render("<style>.c21{color:#000000; & span{color:#000004} &.on{color:#000004}}</style>"
                 L64_BODY "<div class='c21' id=a>a<span id=b>b</span></div>"
                 "<div class='c21 on' id=c>c</div></body>", 400);
    TEST_ASSERT_EQUAL(4, l73c("b"));
    TEST_ASSERT_EQUAL(4, l73c("c"));
}

static void test_l74_nestg_22(void) {
    lot52_render("<style>.c22{color:#000000; & span{color:#000005} &.on{color:#000005}}</style>"
                 L64_BODY "<div class='c22' id=a>a<span id=b>b</span></div>"
                 "<div class='c22 on' id=c>c</div></body>", 400);
    TEST_ASSERT_EQUAL(5, l73c("b"));
    TEST_ASSERT_EQUAL(5, l73c("c"));
}

static void test_l74_nestg_23(void) {
    lot52_render("<style>.c23{color:#000000; & span{color:#000006} &.on{color:#000006}}</style>"
                 L64_BODY "<div class='c23' id=a>a<span id=b>b</span></div>"
                 "<div class='c23 on' id=c>c</div></body>", 400);
    TEST_ASSERT_EQUAL(6, l73c("b"));
    TEST_ASSERT_EQUAL(6, l73c("c"));
}

static void test_l74_nestg_24(void) {
    lot52_render("<style>.c24{color:#000000; & span{color:#000007} &.on{color:#000007}}</style>"
                 L64_BODY "<div class='c24' id=a>a<span id=b>b</span></div>"
                 "<div class='c24 on' id=c>c</div></body>", 400);
    TEST_ASSERT_EQUAL(7, l73c("b"));
    TEST_ASSERT_EQUAL(7, l73c("c"));
}

static void test_l74_nestg_25(void) {
    lot52_render("<style>.c25{color:#000000; & span{color:#000008} &.on{color:#000008}}</style>"
                 L64_BODY "<div class='c25' id=a>a<span id=b>b</span></div>"
                 "<div class='c25 on' id=c>c</div></body>", 400);
    TEST_ASSERT_EQUAL(8, l73c("b"));
    TEST_ASSERT_EQUAL(8, l73c("c"));
}

static void test_l74_nestg_26(void) {
    lot52_render("<style>.c26{color:#000000; & span{color:#000009} &.on{color:#000009}}</style>"
                 L64_BODY "<div class='c26' id=a>a<span id=b>b</span></div>"
                 "<div class='c26 on' id=c>c</div></body>", 400);
    TEST_ASSERT_EQUAL(9, l73c("b"));
    TEST_ASSERT_EQUAL(9, l73c("c"));
}

static void test_l74_nestg_27(void) {
    lot52_render("<style>.c27{color:#000000; & span{color:#000001} &.on{color:#000001}}</style>"
                 L64_BODY "<div class='c27' id=a>a<span id=b>b</span></div>"
                 "<div class='c27 on' id=c>c</div></body>", 400);
    TEST_ASSERT_EQUAL(1, l73c("b"));
    TEST_ASSERT_EQUAL(1, l73c("c"));
}

static void test_l74_nestg_28(void) {
    lot52_render("<style>.c28{color:#000000; & span{color:#000002} &.on{color:#000002}}</style>"
                 L64_BODY "<div class='c28' id=a>a<span id=b>b</span></div>"
                 "<div class='c28 on' id=c>c</div></body>", 400);
    TEST_ASSERT_EQUAL(2, l73c("b"));
    TEST_ASSERT_EQUAL(2, l73c("c"));
}

static void test_l74_nestg_29(void) {
    lot52_render("<style>.c29{color:#000000; & span{color:#000003} &.on{color:#000003}}</style>"
                 L64_BODY "<div class='c29' id=a>a<span id=b>b</span></div>"
                 "<div class='c29 on' id=c>c</div></body>", 400);
    TEST_ASSERT_EQUAL(3, l73c("b"));
    TEST_ASSERT_EQUAL(3, l73c("c"));
}

static void test_l74_nestg_30(void) {
    lot52_render("<style>.c30{color:#000000; & span{color:#000004} &.on{color:#000004}}</style>"
                 L64_BODY "<div class='c30' id=a>a<span id=b>b</span></div>"
                 "<div class='c30 on' id=c>c</div></body>", 400);
    TEST_ASSERT_EQUAL(4, l73c("b"));
    TEST_ASSERT_EQUAL(4, l73c("c"));
}

static void test_l74_nestg_31(void) {
    lot52_render("<style>.c31{color:#000000; & span{color:#000005} &.on{color:#000005}}</style>"
                 L64_BODY "<div class='c31' id=a>a<span id=b>b</span></div>"
                 "<div class='c31 on' id=c>c</div></body>", 400);
    TEST_ASSERT_EQUAL(5, l73c("b"));
    TEST_ASSERT_EQUAL(5, l73c("c"));
}

static void test_l74_nestg_32(void) {
    lot52_render("<style>.c32{color:#000000; & span{color:#000006} &.on{color:#000006}}</style>"
                 L64_BODY "<div class='c32' id=a>a<span id=b>b</span></div>"
                 "<div class='c32 on' id=c>c</div></body>", 400);
    TEST_ASSERT_EQUAL(6, l73c("b"));
    TEST_ASSERT_EQUAL(6, l73c("c"));
}

static void test_l74_nestg_33(void) {
    lot52_render("<style>.c33{color:#000000; & span{color:#000007} &.on{color:#000007}}</style>"
                 L64_BODY "<div class='c33' id=a>a<span id=b>b</span></div>"
                 "<div class='c33 on' id=c>c</div></body>", 400);
    TEST_ASSERT_EQUAL(7, l73c("b"));
    TEST_ASSERT_EQUAL(7, l73c("c"));
}

static void test_l74_nestg_34(void) {
    lot52_render("<style>.c34{color:#000000; & span{color:#000008} &.on{color:#000008}}</style>"
                 L64_BODY "<div class='c34' id=a>a<span id=b>b</span></div>"
                 "<div class='c34 on' id=c>c</div></body>", 400);
    TEST_ASSERT_EQUAL(8, l73c("b"));
    TEST_ASSERT_EQUAL(8, l73c("c"));
}

static void test_l74_nestg_35(void) {
    lot52_render("<style>.c35{color:#000000; & span{color:#000009} &.on{color:#000009}}</style>"
                 L64_BODY "<div class='c35' id=a>a<span id=b>b</span></div>"
                 "<div class='c35 on' id=c>c</div></body>", 400);
    TEST_ASSERT_EQUAL(9, l73c("b"));
    TEST_ASSERT_EQUAL(9, l73c("c"));
}

static void test_l74_nestg_36(void) {
    lot52_render("<style>.c36{color:#000000; & span{color:#000001} &.on{color:#000001}}</style>"
                 L64_BODY "<div class='c36' id=a>a<span id=b>b</span></div>"
                 "<div class='c36 on' id=c>c</div></body>", 400);
    TEST_ASSERT_EQUAL(1, l73c("b"));
    TEST_ASSERT_EQUAL(1, l73c("c"));
}

static void test_l74_nestg_37(void) {
    lot52_render("<style>.c37{color:#000000; & span{color:#000002} &.on{color:#000002}}</style>"
                 L64_BODY "<div class='c37' id=a>a<span id=b>b</span></div>"
                 "<div class='c37 on' id=c>c</div></body>", 400);
    TEST_ASSERT_EQUAL(2, l73c("b"));
    TEST_ASSERT_EQUAL(2, l73c("c"));
}

static void test_l74_nestg_38(void) {
    lot52_render("<style>.c38{color:#000000; & span{color:#000003} &.on{color:#000003}}</style>"
                 L64_BODY "<div class='c38' id=a>a<span id=b>b</span></div>"
                 "<div class='c38 on' id=c>c</div></body>", 400);
    TEST_ASSERT_EQUAL(3, l73c("b"));
    TEST_ASSERT_EQUAL(3, l73c("c"));
}

static void test_l74_nestg_39(void) {
    lot52_render("<style>.c39{color:#000000; & span{color:#000004} &.on{color:#000004}}</style>"
                 L64_BODY "<div class='c39' id=a>a<span id=b>b</span></div>"
                 "<div class='c39 on' id=c>c</div></body>", 400);
    TEST_ASSERT_EQUAL(4, l73c("b"));
    TEST_ASSERT_EQUAL(4, l73c("c"));
}

static void test_l74_nestg_40(void) {
    lot52_render("<style>.c40{color:#000000; & span{color:#000005} &.on{color:#000005}}</style>"
                 L64_BODY "<div class='c40' id=a>a<span id=b>b</span></div>"
                 "<div class='c40 on' id=c>c</div></body>", 400);
    TEST_ASSERT_EQUAL(5, l73c("b"));
    TEST_ASSERT_EQUAL(5, l73c("c"));
}

static void test_l74_nestg_41(void) {
    lot52_render("<style>.c41{color:#000000; & span{color:#000006} &.on{color:#000006}}</style>"
                 L64_BODY "<div class='c41' id=a>a<span id=b>b</span></div>"
                 "<div class='c41 on' id=c>c</div></body>", 400);
    TEST_ASSERT_EQUAL(6, l73c("b"));
    TEST_ASSERT_EQUAL(6, l73c("c"));
}

static void test_l74_nestg_42(void) {
    lot52_render("<style>.c42{color:#000000; & span{color:#000007} &.on{color:#000007}}</style>"
                 L64_BODY "<div class='c42' id=a>a<span id=b>b</span></div>"
                 "<div class='c42 on' id=c>c</div></body>", 400);
    TEST_ASSERT_EQUAL(7, l73c("b"));
    TEST_ASSERT_EQUAL(7, l73c("c"));
}

static void test_l74_nestg_43(void) {
    lot52_render("<style>.c43{color:#000000; & span{color:#000008} &.on{color:#000008}}</style>"
                 L64_BODY "<div class='c43' id=a>a<span id=b>b</span></div>"
                 "<div class='c43 on' id=c>c</div></body>", 400);
    TEST_ASSERT_EQUAL(8, l73c("b"));
    TEST_ASSERT_EQUAL(8, l73c("c"));
}

static void test_l74_nestg_44(void) {
    lot52_render("<style>.c44{color:#000000; & span{color:#000009} &.on{color:#000009}}</style>"
                 L64_BODY "<div class='c44' id=a>a<span id=b>b</span></div>"
                 "<div class='c44 on' id=c>c</div></body>", 400);
    TEST_ASSERT_EQUAL(9, l73c("b"));
    TEST_ASSERT_EQUAL(9, l73c("c"));
}

static void test_l74_nestg_45(void) {
    lot52_render("<style>.c45{color:#000000; & span{color:#000001} &.on{color:#000001}}</style>"
                 L64_BODY "<div class='c45' id=a>a<span id=b>b</span></div>"
                 "<div class='c45 on' id=c>c</div></body>", 400);
    TEST_ASSERT_EQUAL(1, l73c("b"));
    TEST_ASSERT_EQUAL(1, l73c("c"));
}

static void test_l74_nestg_46(void) {
    lot52_render("<style>.c46{color:#000000; & span{color:#000002} &.on{color:#000002}}</style>"
                 L64_BODY "<div class='c46' id=a>a<span id=b>b</span></div>"
                 "<div class='c46 on' id=c>c</div></body>", 400);
    TEST_ASSERT_EQUAL(2, l73c("b"));
    TEST_ASSERT_EQUAL(2, l73c("c"));
}

static void test_l74_nestg_47(void) {
    lot52_render("<style>.c47{color:#000000; & span{color:#000003} &.on{color:#000003}}</style>"
                 L64_BODY "<div class='c47' id=a>a<span id=b>b</span></div>"
                 "<div class='c47 on' id=c>c</div></body>", 400);
    TEST_ASSERT_EQUAL(3, l73c("b"));
    TEST_ASSERT_EQUAL(3, l73c("c"));
}

static void test_l74_nestg_48(void) {
    lot52_render("<style>.c48{color:#000000; & span{color:#000004} &.on{color:#000004}}</style>"
                 L64_BODY "<div class='c48' id=a>a<span id=b>b</span></div>"
                 "<div class='c48 on' id=c>c</div></body>", 400);
    TEST_ASSERT_EQUAL(4, l73c("b"));
    TEST_ASSERT_EQUAL(4, l73c("c"));
}

static void test_l74_nestg_49(void) {
    lot52_render("<style>.c49{color:#000000; & span{color:#000005} &.on{color:#000005}}</style>"
                 L64_BODY "<div class='c49' id=a>a<span id=b>b</span></div>"
                 "<div class='c49 on' id=c>c</div></body>", 400);
    TEST_ASSERT_EQUAL(5, l73c("b"));
    TEST_ASSERT_EQUAL(5, l73c("c"));
}

static void test_l74_nestg_50(void) {
    lot52_render("<style>.c50{color:#000000; & span{color:#000006} &.on{color:#000006}}</style>"
                 L64_BODY "<div class='c50' id=a>a<span id=b>b</span></div>"
                 "<div class='c50 on' id=c>c</div></body>", 400);
    TEST_ASSERT_EQUAL(6, l73c("b"));
    TEST_ASSERT_EQUAL(6, l73c("c"));
}

static void test_l74_nestg_51(void) {
    lot52_render("<style>.c51{color:#000000; & span{color:#000007} &.on{color:#000007}}</style>"
                 L64_BODY "<div class='c51' id=a>a<span id=b>b</span></div>"
                 "<div class='c51 on' id=c>c</div></body>", 400);
    TEST_ASSERT_EQUAL(7, l73c("b"));
    TEST_ASSERT_EQUAL(7, l73c("c"));
}

static void test_l74_nestg_52(void) {
    lot52_render("<style>.c52{color:#000000; & span{color:#000008} &.on{color:#000008}}</style>"
                 L64_BODY "<div class='c52' id=a>a<span id=b>b</span></div>"
                 "<div class='c52 on' id=c>c</div></body>", 400);
    TEST_ASSERT_EQUAL(8, l73c("b"));
    TEST_ASSERT_EQUAL(8, l73c("c"));
}

static void test_l74_nestg_53(void) {
    lot52_render("<style>.c53{color:#000000; & span{color:#000009} &.on{color:#000009}}</style>"
                 L64_BODY "<div class='c53' id=a>a<span id=b>b</span></div>"
                 "<div class='c53 on' id=c>c</div></body>", 400);
    TEST_ASSERT_EQUAL(9, l73c("b"));
    TEST_ASSERT_EQUAL(9, l73c("c"));
}

static void test_l74_nestg_54(void) {
    lot52_render("<style>.c54{color:#000000; & span{color:#000001} &.on{color:#000001}}</style>"
                 L64_BODY "<div class='c54' id=a>a<span id=b>b</span></div>"
                 "<div class='c54 on' id=c>c</div></body>", 400);
    TEST_ASSERT_EQUAL(1, l73c("b"));
    TEST_ASSERT_EQUAL(1, l73c("c"));
}

static void test_l74_nestg_55(void) {
    lot52_render("<style>.c55{color:#000000; & span{color:#000002} &.on{color:#000002}}</style>"
                 L64_BODY "<div class='c55' id=a>a<span id=b>b</span></div>"
                 "<div class='c55 on' id=c>c</div></body>", 400);
    TEST_ASSERT_EQUAL(2, l73c("b"));
    TEST_ASSERT_EQUAL(2, l73c("c"));
}

static void test_l74_nestg_56(void) {
    lot52_render("<style>.c56{color:#000000; & span{color:#000003} &.on{color:#000003}}</style>"
                 L64_BODY "<div class='c56' id=a>a<span id=b>b</span></div>"
                 "<div class='c56 on' id=c>c</div></body>", 400);
    TEST_ASSERT_EQUAL(3, l73c("b"));
    TEST_ASSERT_EQUAL(3, l73c("c"));
}

static void test_l74_nestg_57(void) {
    lot52_render("<style>.c57{color:#000000; & span{color:#000004} &.on{color:#000004}}</style>"
                 L64_BODY "<div class='c57' id=a>a<span id=b>b</span></div>"
                 "<div class='c57 on' id=c>c</div></body>", 400);
    TEST_ASSERT_EQUAL(4, l73c("b"));
    TEST_ASSERT_EQUAL(4, l73c("c"));
}

static void test_l74_nestg_58(void) {
    lot52_render("<style>.c58{color:#000000; & span{color:#000005} &.on{color:#000005}}</style>"
                 L64_BODY "<div class='c58' id=a>a<span id=b>b</span></div>"
                 "<div class='c58 on' id=c>c</div></body>", 400);
    TEST_ASSERT_EQUAL(5, l73c("b"));
    TEST_ASSERT_EQUAL(5, l73c("c"));
}

static void test_l74_nestg_59(void) {
    lot52_render("<style>.c59{color:#000000; & span{color:#000006} &.on{color:#000006}}</style>"
                 L64_BODY "<div class='c59' id=a>a<span id=b>b</span></div>"
                 "<div class='c59 on' id=c>c</div></body>", 400);
    TEST_ASSERT_EQUAL(6, l73c("b"));
    TEST_ASSERT_EQUAL(6, l73c("c"));
}

static void test_l74_tf_0(void) {
    lot52_render("<style>table{table-layout:fixed;width:200px}td{border:1px solid #000}</style>" L64_BODY
                 "<table id=t><tr><td id=a style='width:40px'>a</td><td id=b>b</td></tr>"
                 "<tr><td id=c>wwwwwwwwwwww</td><td id=d>d</td></tr></table></body>", 400);
    TEST_ASSERT_EQUAL(g_doc.nodes[lot52_node("a") - g_doc.nodes].w,
                      g_doc.nodes[lot52_node("c") - g_doc.nodes].w);
}

static void test_l74_tf_1(void) {
    lot52_render("<style>table{table-layout:fixed;width:200px}td{border:1px solid #000}</style>" L64_BODY
                 "<table id=t><tr><td id=a style='width:50px'>a</td><td id=b>b</td></tr>"
                 "<tr><td id=c>wwwwwwwwwwww</td><td id=d>d</td></tr></table></body>", 400);
    TEST_ASSERT_EQUAL(g_doc.nodes[lot52_node("a") - g_doc.nodes].w,
                      g_doc.nodes[lot52_node("c") - g_doc.nodes].w);
}

static void test_l74_tf_2(void) {
    lot52_render("<style>table{table-layout:fixed;width:200px}td{border:1px solid #000}</style>" L64_BODY
                 "<table id=t><tr><td id=a style='width:60px'>a</td><td id=b>b</td></tr>"
                 "<tr><td id=c>wwwwwwwwwwww</td><td id=d>d</td></tr></table></body>", 400);
    TEST_ASSERT_EQUAL(g_doc.nodes[lot52_node("a") - g_doc.nodes].w,
                      g_doc.nodes[lot52_node("c") - g_doc.nodes].w);
}

static void test_l74_tf_3(void) {
    lot52_render("<style>table{table-layout:fixed;width:200px}td{border:1px solid #000}</style>" L64_BODY
                 "<table id=t><tr><td id=a style='width:70px'>a</td><td id=b>b</td></tr>"
                 "<tr><td id=c>wwwwwwwwwwww</td><td id=d>d</td></tr></table></body>", 400);
    TEST_ASSERT_EQUAL(g_doc.nodes[lot52_node("a") - g_doc.nodes].w,
                      g_doc.nodes[lot52_node("c") - g_doc.nodes].w);
}

static void test_l74_tf_4(void) {
    lot52_render("<style>table{table-layout:fixed;width:200px}td{border:1px solid #000}</style>" L64_BODY
                 "<table id=t><tr><td id=a style='width:80px'>a</td><td id=b>b</td></tr>"
                 "<tr><td id=c>wwwwwwwwwwww</td><td id=d>d</td></tr></table></body>", 400);
    TEST_ASSERT_EQUAL(g_doc.nodes[lot52_node("a") - g_doc.nodes].w,
                      g_doc.nodes[lot52_node("c") - g_doc.nodes].w);
}

static void test_l74_tf_5(void) {
    lot52_render("<style>table{table-layout:fixed;width:200px}td{border:1px solid #000}</style>" L64_BODY
                 "<table id=t><tr><td id=a style='width:40px'>a</td><td id=b>b</td></tr>"
                 "<tr><td id=c>wwwwwwwwwwww</td><td id=d>d</td></tr></table></body>", 400);
    TEST_ASSERT_EQUAL(g_doc.nodes[lot52_node("a") - g_doc.nodes].w,
                      g_doc.nodes[lot52_node("c") - g_doc.nodes].w);
}

static void test_l74_tf_6(void) {
    lot52_render("<style>table{table-layout:fixed;width:200px}td{border:1px solid #000}</style>" L64_BODY
                 "<table id=t><tr><td id=a style='width:50px'>a</td><td id=b>b</td></tr>"
                 "<tr><td id=c>wwwwwwwwwwww</td><td id=d>d</td></tr></table></body>", 400);
    TEST_ASSERT_EQUAL(g_doc.nodes[lot52_node("a") - g_doc.nodes].w,
                      g_doc.nodes[lot52_node("c") - g_doc.nodes].w);
}

static void test_l74_tf_7(void) {
    lot52_render("<style>table{table-layout:fixed;width:200px}td{border:1px solid #000}</style>" L64_BODY
                 "<table id=t><tr><td id=a style='width:60px'>a</td><td id=b>b</td></tr>"
                 "<tr><td id=c>wwwwwwwwwwww</td><td id=d>d</td></tr></table></body>", 400);
    TEST_ASSERT_EQUAL(g_doc.nodes[lot52_node("a") - g_doc.nodes].w,
                      g_doc.nodes[lot52_node("c") - g_doc.nodes].w);
}

static void test_l74_tf_8(void) {
    lot52_render("<style>table{table-layout:fixed;width:200px}td{border:1px solid #000}</style>" L64_BODY
                 "<table id=t><tr><td id=a style='width:70px'>a</td><td id=b>b</td></tr>"
                 "<tr><td id=c>wwwwwwwwwwww</td><td id=d>d</td></tr></table></body>", 400);
    TEST_ASSERT_EQUAL(g_doc.nodes[lot52_node("a") - g_doc.nodes].w,
                      g_doc.nodes[lot52_node("c") - g_doc.nodes].w);
}

static void test_l74_tf_9(void) {
    lot52_render("<style>table{table-layout:fixed;width:200px}td{border:1px solid #000}</style>" L64_BODY
                 "<table id=t><tr><td id=a style='width:80px'>a</td><td id=b>b</td></tr>"
                 "<tr><td id=c>wwwwwwwwwwww</td><td id=d>d</td></tr></table></body>", 400);
    TEST_ASSERT_EQUAL(g_doc.nodes[lot52_node("a") - g_doc.nodes].w,
                      g_doc.nodes[lot52_node("c") - g_doc.nodes].w);
}

static void test_l74_tf_10(void) {
    lot52_render("<style>table{table-layout:fixed;width:200px}td{border:1px solid #000}</style>" L64_BODY
                 "<table id=t><tr><td id=a style='width:40px'>a</td><td id=b>b</td></tr>"
                 "<tr><td id=c>wwwwwwwwwwww</td><td id=d>d</td></tr></table></body>", 400);
    TEST_ASSERT_EQUAL(g_doc.nodes[lot52_node("a") - g_doc.nodes].w,
                      g_doc.nodes[lot52_node("c") - g_doc.nodes].w);
}

static void test_l74_tf_11(void) {
    lot52_render("<style>table{table-layout:fixed;width:200px}td{border:1px solid #000}</style>" L64_BODY
                 "<table id=t><tr><td id=a style='width:50px'>a</td><td id=b>b</td></tr>"
                 "<tr><td id=c>wwwwwwwwwwww</td><td id=d>d</td></tr></table></body>", 400);
    TEST_ASSERT_EQUAL(g_doc.nodes[lot52_node("a") - g_doc.nodes].w,
                      g_doc.nodes[lot52_node("c") - g_doc.nodes].w);
}

static void test_l74_tf_12(void) {
    lot52_render("<style>table{table-layout:fixed;width:200px}td{border:1px solid #000}</style>" L64_BODY
                 "<table id=t><tr><td id=a style='width:60px'>a</td><td id=b>b</td></tr>"
                 "<tr><td id=c>wwwwwwwwwwww</td><td id=d>d</td></tr></table></body>", 400);
    TEST_ASSERT_EQUAL(g_doc.nodes[lot52_node("a") - g_doc.nodes].w,
                      g_doc.nodes[lot52_node("c") - g_doc.nodes].w);
}

static void test_l74_tf_13(void) {
    lot52_render("<style>table{table-layout:fixed;width:200px}td{border:1px solid #000}</style>" L64_BODY
                 "<table id=t><tr><td id=a style='width:70px'>a</td><td id=b>b</td></tr>"
                 "<tr><td id=c>wwwwwwwwwwww</td><td id=d>d</td></tr></table></body>", 400);
    TEST_ASSERT_EQUAL(g_doc.nodes[lot52_node("a") - g_doc.nodes].w,
                      g_doc.nodes[lot52_node("c") - g_doc.nodes].w);
}

static void test_l74_tf_14(void) {
    lot52_render("<style>table{table-layout:fixed;width:200px}td{border:1px solid #000}</style>" L64_BODY
                 "<table id=t><tr><td id=a style='width:80px'>a</td><td id=b>b</td></tr>"
                 "<tr><td id=c>wwwwwwwwwwww</td><td id=d>d</td></tr></table></body>", 400);
    TEST_ASSERT_EQUAL(g_doc.nodes[lot52_node("a") - g_doc.nodes].w,
                      g_doc.nodes[lot52_node("c") - g_doc.nodes].w);
}

static void test_l74_tf_15(void) {
    lot52_render("<style>table{table-layout:fixed;width:200px}td{border:1px solid #000}</style>" L64_BODY
                 "<table id=t><tr><td id=a style='width:40px'>a</td><td id=b>b</td></tr>"
                 "<tr><td id=c>wwwwwwwwwwww</td><td id=d>d</td></tr></table></body>", 400);
    TEST_ASSERT_EQUAL(g_doc.nodes[lot52_node("a") - g_doc.nodes].w,
                      g_doc.nodes[lot52_node("c") - g_doc.nodes].w);
}

static void test_l74_tf_16(void) {
    lot52_render("<style>table{table-layout:fixed;width:200px}td{border:1px solid #000}</style>" L64_BODY
                 "<table id=t><tr><td id=a style='width:50px'>a</td><td id=b>b</td></tr>"
                 "<tr><td id=c>wwwwwwwwwwww</td><td id=d>d</td></tr></table></body>", 400);
    TEST_ASSERT_EQUAL(g_doc.nodes[lot52_node("a") - g_doc.nodes].w,
                      g_doc.nodes[lot52_node("c") - g_doc.nodes].w);
}

static void test_l74_tf_17(void) {
    lot52_render("<style>table{table-layout:fixed;width:200px}td{border:1px solid #000}</style>" L64_BODY
                 "<table id=t><tr><td id=a style='width:60px'>a</td><td id=b>b</td></tr>"
                 "<tr><td id=c>wwwwwwwwwwww</td><td id=d>d</td></tr></table></body>", 400);
    TEST_ASSERT_EQUAL(g_doc.nodes[lot52_node("a") - g_doc.nodes].w,
                      g_doc.nodes[lot52_node("c") - g_doc.nodes].w);
}

static void test_l74_tf_18(void) {
    lot52_render("<style>table{table-layout:fixed;width:200px}td{border:1px solid #000}</style>" L64_BODY
                 "<table id=t><tr><td id=a style='width:70px'>a</td><td id=b>b</td></tr>"
                 "<tr><td id=c>wwwwwwwwwwww</td><td id=d>d</td></tr></table></body>", 400);
    TEST_ASSERT_EQUAL(g_doc.nodes[lot52_node("a") - g_doc.nodes].w,
                      g_doc.nodes[lot52_node("c") - g_doc.nodes].w);
}

static void test_l74_tf_19(void) {
    lot52_render("<style>table{table-layout:fixed;width:200px}td{border:1px solid #000}</style>" L64_BODY
                 "<table id=t><tr><td id=a style='width:80px'>a</td><td id=b>b</td></tr>"
                 "<tr><td id=c>wwwwwwwwwwww</td><td id=d>d</td></tr></table></body>", 400);
    TEST_ASSERT_EQUAL(g_doc.nodes[lot52_node("a") - g_doc.nodes].w,
                      g_doc.nodes[lot52_node("c") - g_doc.nodes].w);
}

static void test_l74_tf_20(void) {
    lot52_render("<style>table{table-layout:fixed;width:200px}td{border:1px solid #000}</style>" L64_BODY
                 "<table id=t><tr><td id=a style='width:40px'>a</td><td id=b>b</td></tr>"
                 "<tr><td id=c>wwwwwwwwwwww</td><td id=d>d</td></tr></table></body>", 400);
    TEST_ASSERT_EQUAL(g_doc.nodes[lot52_node("a") - g_doc.nodes].w,
                      g_doc.nodes[lot52_node("c") - g_doc.nodes].w);
}

static void test_l74_tf_21(void) {
    lot52_render("<style>table{table-layout:fixed;width:200px}td{border:1px solid #000}</style>" L64_BODY
                 "<table id=t><tr><td id=a style='width:50px'>a</td><td id=b>b</td></tr>"
                 "<tr><td id=c>wwwwwwwwwwww</td><td id=d>d</td></tr></table></body>", 400);
    TEST_ASSERT_EQUAL(g_doc.nodes[lot52_node("a") - g_doc.nodes].w,
                      g_doc.nodes[lot52_node("c") - g_doc.nodes].w);
}

static void test_l74_tf_22(void) {
    lot52_render("<style>table{table-layout:fixed;width:200px}td{border:1px solid #000}</style>" L64_BODY
                 "<table id=t><tr><td id=a style='width:60px'>a</td><td id=b>b</td></tr>"
                 "<tr><td id=c>wwwwwwwwwwww</td><td id=d>d</td></tr></table></body>", 400);
    TEST_ASSERT_EQUAL(g_doc.nodes[lot52_node("a") - g_doc.nodes].w,
                      g_doc.nodes[lot52_node("c") - g_doc.nodes].w);
}

static void test_l74_tf_23(void) {
    lot52_render("<style>table{table-layout:fixed;width:200px}td{border:1px solid #000}</style>" L64_BODY
                 "<table id=t><tr><td id=a style='width:70px'>a</td><td id=b>b</td></tr>"
                 "<tr><td id=c>wwwwwwwwwwww</td><td id=d>d</td></tr></table></body>", 400);
    TEST_ASSERT_EQUAL(g_doc.nodes[lot52_node("a") - g_doc.nodes].w,
                      g_doc.nodes[lot52_node("c") - g_doc.nodes].w);
}

static void test_l74_tf_24(void) {
    lot52_render("<style>table{table-layout:fixed;width:200px}td{border:1px solid #000}</style>" L64_BODY
                 "<table id=t><tr><td id=a style='width:80px'>a</td><td id=b>b</td></tr>"
                 "<tr><td id=c>wwwwwwwwwwww</td><td id=d>d</td></tr></table></body>", 400);
    TEST_ASSERT_EQUAL(g_doc.nodes[lot52_node("a") - g_doc.nodes].w,
                      g_doc.nodes[lot52_node("c") - g_doc.nodes].w);
}

static void test_l74_tf_25(void) {
    lot52_render("<style>table{table-layout:fixed;width:200px}td{border:1px solid #000}</style>" L64_BODY
                 "<table id=t><tr><td id=a style='width:40px'>a</td><td id=b>b</td></tr>"
                 "<tr><td id=c>wwwwwwwwwwww</td><td id=d>d</td></tr></table></body>", 400);
    TEST_ASSERT_EQUAL(g_doc.nodes[lot52_node("a") - g_doc.nodes].w,
                      g_doc.nodes[lot52_node("c") - g_doc.nodes].w);
}

static void test_l74_tf_26(void) {
    lot52_render("<style>table{table-layout:fixed;width:200px}td{border:1px solid #000}</style>" L64_BODY
                 "<table id=t><tr><td id=a style='width:50px'>a</td><td id=b>b</td></tr>"
                 "<tr><td id=c>wwwwwwwwwwww</td><td id=d>d</td></tr></table></body>", 400);
    TEST_ASSERT_EQUAL(g_doc.nodes[lot52_node("a") - g_doc.nodes].w,
                      g_doc.nodes[lot52_node("c") - g_doc.nodes].w);
}

static void test_l74_tf_27(void) {
    lot52_render("<style>table{table-layout:fixed;width:200px}td{border:1px solid #000}</style>" L64_BODY
                 "<table id=t><tr><td id=a style='width:60px'>a</td><td id=b>b</td></tr>"
                 "<tr><td id=c>wwwwwwwwwwww</td><td id=d>d</td></tr></table></body>", 400);
    TEST_ASSERT_EQUAL(g_doc.nodes[lot52_node("a") - g_doc.nodes].w,
                      g_doc.nodes[lot52_node("c") - g_doc.nodes].w);
}

static void test_l74_tf_28(void) {
    lot52_render("<style>table{table-layout:fixed;width:200px}td{border:1px solid #000}</style>" L64_BODY
                 "<table id=t><tr><td id=a style='width:70px'>a</td><td id=b>b</td></tr>"
                 "<tr><td id=c>wwwwwwwwwwww</td><td id=d>d</td></tr></table></body>", 400);
    TEST_ASSERT_EQUAL(g_doc.nodes[lot52_node("a") - g_doc.nodes].w,
                      g_doc.nodes[lot52_node("c") - g_doc.nodes].w);
}

static void test_l74_tf_29(void) {
    lot52_render("<style>table{table-layout:fixed;width:200px}td{border:1px solid #000}</style>" L64_BODY
                 "<table id=t><tr><td id=a style='width:80px'>a</td><td id=b>b</td></tr>"
                 "<tr><td id=c>wwwwwwwwwwww</td><td id=d>d</td></tr></table></body>", 400);
    TEST_ASSERT_EQUAL(g_doc.nodes[lot52_node("a") - g_doc.nodes].w,
                      g_doc.nodes[lot52_node("c") - g_doc.nodes].w);
}

static void test_l74_tf_30(void) {
    lot52_render("<style>table{table-layout:fixed;width:200px}td{border:1px solid #000}</style>" L64_BODY
                 "<table id=t><tr><td id=a style='width:40px'>a</td><td id=b>b</td></tr>"
                 "<tr><td id=c>wwwwwwwwwwww</td><td id=d>d</td></tr></table></body>", 400);
    TEST_ASSERT_EQUAL(g_doc.nodes[lot52_node("a") - g_doc.nodes].w,
                      g_doc.nodes[lot52_node("c") - g_doc.nodes].w);
}

static void test_l74_tf_31(void) {
    lot52_render("<style>table{table-layout:fixed;width:200px}td{border:1px solid #000}</style>" L64_BODY
                 "<table id=t><tr><td id=a style='width:50px'>a</td><td id=b>b</td></tr>"
                 "<tr><td id=c>wwwwwwwwwwww</td><td id=d>d</td></tr></table></body>", 400);
    TEST_ASSERT_EQUAL(g_doc.nodes[lot52_node("a") - g_doc.nodes].w,
                      g_doc.nodes[lot52_node("c") - g_doc.nodes].w);
}

static void test_l74_tf_32(void) {
    lot52_render("<style>table{table-layout:fixed;width:200px}td{border:1px solid #000}</style>" L64_BODY
                 "<table id=t><tr><td id=a style='width:60px'>a</td><td id=b>b</td></tr>"
                 "<tr><td id=c>wwwwwwwwwwww</td><td id=d>d</td></tr></table></body>", 400);
    TEST_ASSERT_EQUAL(g_doc.nodes[lot52_node("a") - g_doc.nodes].w,
                      g_doc.nodes[lot52_node("c") - g_doc.nodes].w);
}

static void test_l74_tf_33(void) {
    lot52_render("<style>table{table-layout:fixed;width:200px}td{border:1px solid #000}</style>" L64_BODY
                 "<table id=t><tr><td id=a style='width:70px'>a</td><td id=b>b</td></tr>"
                 "<tr><td id=c>wwwwwwwwwwww</td><td id=d>d</td></tr></table></body>", 400);
    TEST_ASSERT_EQUAL(g_doc.nodes[lot52_node("a") - g_doc.nodes].w,
                      g_doc.nodes[lot52_node("c") - g_doc.nodes].w);
}

static void test_l74_tf_34(void) {
    lot52_render("<style>table{table-layout:fixed;width:200px}td{border:1px solid #000}</style>" L64_BODY
                 "<table id=t><tr><td id=a style='width:80px'>a</td><td id=b>b</td></tr>"
                 "<tr><td id=c>wwwwwwwwwwww</td><td id=d>d</td></tr></table></body>", 400);
    TEST_ASSERT_EQUAL(g_doc.nodes[lot52_node("a") - g_doc.nodes].w,
                      g_doc.nodes[lot52_node("c") - g_doc.nodes].w);
}

static void test_l74_tf_35(void) {
    lot52_render("<style>table{table-layout:fixed;width:200px}td{border:1px solid #000}</style>" L64_BODY
                 "<table id=t><tr><td id=a style='width:40px'>a</td><td id=b>b</td></tr>"
                 "<tr><td id=c>wwwwwwwwwwww</td><td id=d>d</td></tr></table></body>", 400);
    TEST_ASSERT_EQUAL(g_doc.nodes[lot52_node("a") - g_doc.nodes].w,
                      g_doc.nodes[lot52_node("c") - g_doc.nodes].w);
}

static void test_l74_tf_36(void) {
    lot52_render("<style>table{table-layout:fixed;width:200px}td{border:1px solid #000}</style>" L64_BODY
                 "<table id=t><tr><td id=a style='width:50px'>a</td><td id=b>b</td></tr>"
                 "<tr><td id=c>wwwwwwwwwwww</td><td id=d>d</td></tr></table></body>", 400);
    TEST_ASSERT_EQUAL(g_doc.nodes[lot52_node("a") - g_doc.nodes].w,
                      g_doc.nodes[lot52_node("c") - g_doc.nodes].w);
}

static void test_l74_tf_37(void) {
    lot52_render("<style>table{table-layout:fixed;width:200px}td{border:1px solid #000}</style>" L64_BODY
                 "<table id=t><tr><td id=a style='width:60px'>a</td><td id=b>b</td></tr>"
                 "<tr><td id=c>wwwwwwwwwwww</td><td id=d>d</td></tr></table></body>", 400);
    TEST_ASSERT_EQUAL(g_doc.nodes[lot52_node("a") - g_doc.nodes].w,
                      g_doc.nodes[lot52_node("c") - g_doc.nodes].w);
}

static void test_l74_tf_38(void) {
    lot52_render("<style>table{table-layout:fixed;width:200px}td{border:1px solid #000}</style>" L64_BODY
                 "<table id=t><tr><td id=a style='width:70px'>a</td><td id=b>b</td></tr>"
                 "<tr><td id=c>wwwwwwwwwwww</td><td id=d>d</td></tr></table></body>", 400);
    TEST_ASSERT_EQUAL(g_doc.nodes[lot52_node("a") - g_doc.nodes].w,
                      g_doc.nodes[lot52_node("c") - g_doc.nodes].w);
}

static void test_l74_tf_39(void) {
    lot52_render("<style>table{table-layout:fixed;width:200px}td{border:1px solid #000}</style>" L64_BODY
                 "<table id=t><tr><td id=a style='width:80px'>a</td><td id=b>b</td></tr>"
                 "<tr><td id=c>wwwwwwwwwwww</td><td id=d>d</td></tr></table></body>", 400);
    TEST_ASSERT_EQUAL(g_doc.nodes[lot52_node("a") - g_doc.nodes].w,
                      g_doc.nodes[lot52_node("c") - g_doc.nodes].w);
}

static void test_l74_ifr_0(void) {
    lot52_render("<body><iframe id=f srcdoc='<p id=p style=color:#000001>p0</p>'></iframe></body>", 400);
    TEST_ASSERT_EQUAL(1, l73c("p"));
}

static void test_l74_ifr_1(void) {
    lot52_render("<body><iframe id=f srcdoc='<p id=p style=color:#000002>p1</p>'></iframe></body>", 400);
    TEST_ASSERT_EQUAL(2, l73c("p"));
}

static void test_l74_ifr_2(void) {
    lot52_render("<body><iframe id=f srcdoc='<p id=p style=color:#000003>p2</p>'></iframe></body>", 400);
    TEST_ASSERT_EQUAL(3, l73c("p"));
}

static void test_l74_ifr_3(void) {
    lot52_render("<body><iframe id=f srcdoc='<p id=p style=color:#000004>p3</p>'></iframe></body>", 400);
    TEST_ASSERT_EQUAL(4, l73c("p"));
}

static void test_l74_ifr_4(void) {
    lot52_render("<body><iframe id=f srcdoc='<p id=p style=color:#000005>p4</p>'></iframe></body>", 400);
    TEST_ASSERT_EQUAL(5, l73c("p"));
}

static void test_l74_ifr_5(void) {
    lot52_render("<body><iframe id=f srcdoc='<p id=p style=color:#000006>p5</p>'></iframe></body>", 400);
    TEST_ASSERT_EQUAL(6, l73c("p"));
}

static void test_l74_ifr_6(void) {
    lot52_render("<body><iframe id=f srcdoc='<p id=p style=color:#000007>p6</p>'></iframe></body>", 400);
    TEST_ASSERT_EQUAL(7, l73c("p"));
}

static void test_l74_ifr_7(void) {
    lot52_render("<body><iframe id=f srcdoc='<p id=p style=color:#000008>p7</p>'></iframe></body>", 400);
    TEST_ASSERT_EQUAL(8, l73c("p"));
}

static void test_l74_ifr_8(void) {
    lot52_render("<body><iframe id=f srcdoc='<p id=p style=color:#000009>p8</p>'></iframe></body>", 400);
    TEST_ASSERT_EQUAL(9, l73c("p"));
}

static void test_l74_ifr_9(void) {
    lot52_render("<body><iframe id=f srcdoc='<p id=p style=color:#000001>p9</p>'></iframe></body>", 400);
    TEST_ASSERT_EQUAL(1, l73c("p"));
}

static void test_l74_ifr_10(void) {
    lot52_render("<body><iframe id=f srcdoc='<p id=p style=color:#000002>p10</p>'></iframe></body>", 400);
    TEST_ASSERT_EQUAL(2, l73c("p"));
}

static void test_l74_ifr_11(void) {
    lot52_render("<body><iframe id=f srcdoc='<p id=p style=color:#000003>p11</p>'></iframe></body>", 400);
    TEST_ASSERT_EQUAL(3, l73c("p"));
}

static void test_l74_ifr_12(void) {
    lot52_render("<body><iframe id=f srcdoc='<p id=p style=color:#000004>p12</p>'></iframe></body>", 400);
    TEST_ASSERT_EQUAL(4, l73c("p"));
}

static void test_l74_ifr_13(void) {
    lot52_render("<body><iframe id=f srcdoc='<p id=p style=color:#000005>p13</p>'></iframe></body>", 400);
    TEST_ASSERT_EQUAL(5, l73c("p"));
}

static void test_l74_ifr_14(void) {
    lot52_render("<body><iframe id=f srcdoc='<p id=p style=color:#000006>p14</p>'></iframe></body>", 400);
    TEST_ASSERT_EQUAL(6, l73c("p"));
}

static void test_l74_ifr_15(void) {
    lot52_render("<body><iframe id=f srcdoc='<p id=p style=color:#000007>p15</p>'></iframe></body>", 400);
    TEST_ASSERT_EQUAL(7, l73c("p"));
}

static void test_l74_ifr_16(void) {
    lot52_render("<body><iframe id=f srcdoc='<p id=p style=color:#000008>p16</p>'></iframe></body>", 400);
    TEST_ASSERT_EQUAL(8, l73c("p"));
}

static void test_l74_ifr_17(void) {
    lot52_render("<body><iframe id=f srcdoc='<p id=p style=color:#000009>p17</p>'></iframe></body>", 400);
    TEST_ASSERT_EQUAL(9, l73c("p"));
}

static void test_l74_ifr_18(void) {
    lot52_render("<body><iframe id=f srcdoc='<p id=p style=color:#000001>p18</p>'></iframe></body>", 400);
    TEST_ASSERT_EQUAL(1, l73c("p"));
}

static void test_l74_ifr_19(void) {
    lot52_render("<body><iframe id=f srcdoc='<p id=p style=color:#000002>p19</p>'></iframe></body>", 400);
    TEST_ASSERT_EQUAL(2, l73c("p"));
}

static void test_l74_ifr_20(void) {
    lot52_render("<body><iframe id=f srcdoc='<p id=p style=color:#000003>p20</p>'></iframe></body>", 400);
    TEST_ASSERT_EQUAL(3, l73c("p"));
}

static void test_l74_ifr_21(void) {
    lot52_render("<body><iframe id=f srcdoc='<p id=p style=color:#000004>p21</p>'></iframe></body>", 400);
    TEST_ASSERT_EQUAL(4, l73c("p"));
}

static void test_l74_ifr_22(void) {
    lot52_render("<body><iframe id=f srcdoc='<p id=p style=color:#000005>p22</p>'></iframe></body>", 400);
    TEST_ASSERT_EQUAL(5, l73c("p"));
}

static void test_l74_ifr_23(void) {
    lot52_render("<body><iframe id=f srcdoc='<p id=p style=color:#000006>p23</p>'></iframe></body>", 400);
    TEST_ASSERT_EQUAL(6, l73c("p"));
}

static void test_l74_ifr_24(void) {
    lot52_render("<body><iframe id=f srcdoc='<p id=p style=color:#000007>p24</p>'></iframe></body>", 400);
    TEST_ASSERT_EQUAL(7, l73c("p"));
}

static void test_l74_ifr_25(void) {
    lot52_render("<body><iframe id=f srcdoc='<p id=p style=color:#000008>p25</p>'></iframe></body>", 400);
    TEST_ASSERT_EQUAL(8, l73c("p"));
}

static void test_l74_ifr_26(void) {
    lot52_render("<body><iframe id=f srcdoc='<p id=p style=color:#000009>p26</p>'></iframe></body>", 400);
    TEST_ASSERT_EQUAL(9, l73c("p"));
}

static void test_l74_ifr_27(void) {
    lot52_render("<body><iframe id=f srcdoc='<p id=p style=color:#000001>p27</p>'></iframe></body>", 400);
    TEST_ASSERT_EQUAL(1, l73c("p"));
}

static void test_l74_ifr_28(void) {
    lot52_render("<body><iframe id=f srcdoc='<p id=p style=color:#000002>p28</p>'></iframe></body>", 400);
    TEST_ASSERT_EQUAL(2, l73c("p"));
}

static void test_l74_ifr_29(void) {
    lot52_render("<body><iframe id=f srcdoc='<p id=p style=color:#000003>p29</p>'></iframe></body>", 400);
    TEST_ASSERT_EQUAL(3, l73c("p"));
}

static void test_l74_ifr_30(void) {
    lot52_render("<body><iframe id=f srcdoc='<p id=p style=color:#000004>p30</p>'></iframe></body>", 400);
    TEST_ASSERT_EQUAL(4, l73c("p"));
}

static void test_l74_ifr_31(void) {
    lot52_render("<body><iframe id=f srcdoc='<p id=p style=color:#000005>p31</p>'></iframe></body>", 400);
    TEST_ASSERT_EQUAL(5, l73c("p"));
}

static void test_l74_ifr_32(void) {
    lot52_render("<body><iframe id=f srcdoc='<p id=p style=color:#000006>p32</p>'></iframe></body>", 400);
    TEST_ASSERT_EQUAL(6, l73c("p"));
}

static void test_l74_ifr_33(void) {
    lot52_render("<body><iframe id=f srcdoc='<p id=p style=color:#000007>p33</p>'></iframe></body>", 400);
    TEST_ASSERT_EQUAL(7, l73c("p"));
}

static void test_l74_ifr_34(void) {
    lot52_render("<body><iframe id=f srcdoc='<p id=p style=color:#000008>p34</p>'></iframe></body>", 400);
    TEST_ASSERT_EQUAL(8, l73c("p"));
}

static void test_l74_ifr_35(void) {
    lot52_render("<body><iframe id=f srcdoc='<p id=p style=color:#000009>p35</p>'></iframe></body>", 400);
    TEST_ASSERT_EQUAL(9, l73c("p"));
}

static void test_l74_ifr_36(void) {
    lot52_render("<body><iframe id=f srcdoc='<p id=p style=color:#000001>p36</p>'></iframe></body>", 400);
    TEST_ASSERT_EQUAL(1, l73c("p"));
}

static void test_l74_ifr_37(void) {
    lot52_render("<body><iframe id=f srcdoc='<p id=p style=color:#000002>p37</p>'></iframe></body>", 400);
    TEST_ASSERT_EQUAL(2, l73c("p"));
}

static void test_l74_ifr_38(void) {
    lot52_render("<body><iframe id=f srcdoc='<p id=p style=color:#000003>p38</p>'></iframe></body>", 400);
    TEST_ASSERT_EQUAL(3, l73c("p"));
}

static void test_l74_ifr_39(void) {
    lot52_render("<body><iframe id=f srcdoc='<p id=p style=color:#000004>p39</p>'></iframe></body>", 400);
    TEST_ASSERT_EQUAL(4, l73c("p"));
}

static void test_l74_dlg_0(void) {
    l74_open("<dialog id=d open><form method=dialog><button id=ok type=submit>ok0</button></form></dialog>");
    TEST_ASSERT_EQUAL(WE_DISP_BLOCK, g_doc.nodes[l74_node("d")].display);
    TEST_ASSERT_EQUAL(WE_FACT_CHANGED, l74_click("ok"));
    l74_render();
    TEST_ASSERT_EQUAL(WE_DISP_NONE, g_doc.nodes[l74_node("d")].display);
    l74_done();
}

static void test_l74_dlg_1(void) {
    l74_open("<dialog id=d open><form method=dialog><button id=ok type=submit>ok1</button></form></dialog>");
    TEST_ASSERT_EQUAL(WE_DISP_BLOCK, g_doc.nodes[l74_node("d")].display);
    TEST_ASSERT_EQUAL(WE_FACT_CHANGED, l74_click("ok"));
    l74_render();
    TEST_ASSERT_EQUAL(WE_DISP_NONE, g_doc.nodes[l74_node("d")].display);
    l74_done();
}

static void test_l74_dlg_2(void) {
    l74_open("<dialog id=d open><form method=dialog><button id=ok type=submit>ok2</button></form></dialog>");
    TEST_ASSERT_EQUAL(WE_DISP_BLOCK, g_doc.nodes[l74_node("d")].display);
    TEST_ASSERT_EQUAL(WE_FACT_CHANGED, l74_click("ok"));
    l74_render();
    TEST_ASSERT_EQUAL(WE_DISP_NONE, g_doc.nodes[l74_node("d")].display);
    l74_done();
}

static void test_l74_dlg_3(void) {
    l74_open("<dialog id=d open><form method=dialog><button id=ok type=submit>ok3</button></form></dialog>");
    TEST_ASSERT_EQUAL(WE_DISP_BLOCK, g_doc.nodes[l74_node("d")].display);
    TEST_ASSERT_EQUAL(WE_FACT_CHANGED, l74_click("ok"));
    l74_render();
    TEST_ASSERT_EQUAL(WE_DISP_NONE, g_doc.nodes[l74_node("d")].display);
    l74_done();
}

static void test_l74_dlg_4(void) {
    l74_open("<dialog id=d open><form method=dialog><button id=ok type=submit>ok4</button></form></dialog>");
    TEST_ASSERT_EQUAL(WE_DISP_BLOCK, g_doc.nodes[l74_node("d")].display);
    TEST_ASSERT_EQUAL(WE_FACT_CHANGED, l74_click("ok"));
    l74_render();
    TEST_ASSERT_EQUAL(WE_DISP_NONE, g_doc.nodes[l74_node("d")].display);
    l74_done();
}

static void test_l74_dlg_5(void) {
    l74_open("<dialog id=d open><form method=dialog><button id=ok type=submit>ok5</button></form></dialog>");
    TEST_ASSERT_EQUAL(WE_DISP_BLOCK, g_doc.nodes[l74_node("d")].display);
    TEST_ASSERT_EQUAL(WE_FACT_CHANGED, l74_click("ok"));
    l74_render();
    TEST_ASSERT_EQUAL(WE_DISP_NONE, g_doc.nodes[l74_node("d")].display);
    l74_done();
}

static void test_l74_dlg_6(void) {
    l74_open("<dialog id=d open><form method=dialog><button id=ok type=submit>ok6</button></form></dialog>");
    TEST_ASSERT_EQUAL(WE_DISP_BLOCK, g_doc.nodes[l74_node("d")].display);
    TEST_ASSERT_EQUAL(WE_FACT_CHANGED, l74_click("ok"));
    l74_render();
    TEST_ASSERT_EQUAL(WE_DISP_NONE, g_doc.nodes[l74_node("d")].display);
    l74_done();
}

static void test_l74_dlg_7(void) {
    l74_open("<dialog id=d open><form method=dialog><button id=ok type=submit>ok7</button></form></dialog>");
    TEST_ASSERT_EQUAL(WE_DISP_BLOCK, g_doc.nodes[l74_node("d")].display);
    TEST_ASSERT_EQUAL(WE_FACT_CHANGED, l74_click("ok"));
    l74_render();
    TEST_ASSERT_EQUAL(WE_DISP_NONE, g_doc.nodes[l74_node("d")].display);
    l74_done();
}

static void test_l74_dlg_8(void) {
    l74_open("<dialog id=d open><form method=dialog><button id=ok type=submit>ok8</button></form></dialog>");
    TEST_ASSERT_EQUAL(WE_DISP_BLOCK, g_doc.nodes[l74_node("d")].display);
    TEST_ASSERT_EQUAL(WE_FACT_CHANGED, l74_click("ok"));
    l74_render();
    TEST_ASSERT_EQUAL(WE_DISP_NONE, g_doc.nodes[l74_node("d")].display);
    l74_done();
}

static void test_l74_dlg_9(void) {
    l74_open("<dialog id=d open><form method=dialog><button id=ok type=submit>ok9</button></form></dialog>");
    TEST_ASSERT_EQUAL(WE_DISP_BLOCK, g_doc.nodes[l74_node("d")].display);
    TEST_ASSERT_EQUAL(WE_FACT_CHANGED, l74_click("ok"));
    l74_render();
    TEST_ASSERT_EQUAL(WE_DISP_NONE, g_doc.nodes[l74_node("d")].display);
    l74_done();
}

static void test_l74_dlg_10(void) {
    l74_open("<dialog id=d open><form method=dialog><button id=ok type=submit>ok10</button></form></dialog>");
    TEST_ASSERT_EQUAL(WE_DISP_BLOCK, g_doc.nodes[l74_node("d")].display);
    TEST_ASSERT_EQUAL(WE_FACT_CHANGED, l74_click("ok"));
    l74_render();
    TEST_ASSERT_EQUAL(WE_DISP_NONE, g_doc.nodes[l74_node("d")].display);
    l74_done();
}

static void test_l74_dlg_11(void) {
    l74_open("<dialog id=d open><form method=dialog><button id=ok type=submit>ok11</button></form></dialog>");
    TEST_ASSERT_EQUAL(WE_DISP_BLOCK, g_doc.nodes[l74_node("d")].display);
    TEST_ASSERT_EQUAL(WE_FACT_CHANGED, l74_click("ok"));
    l74_render();
    TEST_ASSERT_EQUAL(WE_DISP_NONE, g_doc.nodes[l74_node("d")].display);
    l74_done();
}

static void test_l74_dlg_12(void) {
    l74_open("<dialog id=d open><form method=dialog><button id=ok type=submit>ok12</button></form></dialog>");
    TEST_ASSERT_EQUAL(WE_DISP_BLOCK, g_doc.nodes[l74_node("d")].display);
    TEST_ASSERT_EQUAL(WE_FACT_CHANGED, l74_click("ok"));
    l74_render();
    TEST_ASSERT_EQUAL(WE_DISP_NONE, g_doc.nodes[l74_node("d")].display);
    l74_done();
}

static void test_l74_dlg_13(void) {
    l74_open("<dialog id=d open><form method=dialog><button id=ok type=submit>ok13</button></form></dialog>");
    TEST_ASSERT_EQUAL(WE_DISP_BLOCK, g_doc.nodes[l74_node("d")].display);
    TEST_ASSERT_EQUAL(WE_FACT_CHANGED, l74_click("ok"));
    l74_render();
    TEST_ASSERT_EQUAL(WE_DISP_NONE, g_doc.nodes[l74_node("d")].display);
    l74_done();
}

static void test_l74_dlg_14(void) {
    l74_open("<dialog id=d open><form method=dialog><button id=ok type=submit>ok14</button></form></dialog>");
    TEST_ASSERT_EQUAL(WE_DISP_BLOCK, g_doc.nodes[l74_node("d")].display);
    TEST_ASSERT_EQUAL(WE_FACT_CHANGED, l74_click("ok"));
    l74_render();
    TEST_ASSERT_EQUAL(WE_DISP_NONE, g_doc.nodes[l74_node("d")].display);
    l74_done();
}

static void test_l74_dlg_15(void) {
    l74_open("<dialog id=d open><form method=dialog><button id=ok type=submit>ok15</button></form></dialog>");
    TEST_ASSERT_EQUAL(WE_DISP_BLOCK, g_doc.nodes[l74_node("d")].display);
    TEST_ASSERT_EQUAL(WE_FACT_CHANGED, l74_click("ok"));
    l74_render();
    TEST_ASSERT_EQUAL(WE_DISP_NONE, g_doc.nodes[l74_node("d")].display);
    l74_done();
}

static void test_l74_dlg_16(void) {
    l74_open("<dialog id=d open><form method=dialog><button id=ok type=submit>ok16</button></form></dialog>");
    TEST_ASSERT_EQUAL(WE_DISP_BLOCK, g_doc.nodes[l74_node("d")].display);
    TEST_ASSERT_EQUAL(WE_FACT_CHANGED, l74_click("ok"));
    l74_render();
    TEST_ASSERT_EQUAL(WE_DISP_NONE, g_doc.nodes[l74_node("d")].display);
    l74_done();
}

static void test_l74_dlg_17(void) {
    l74_open("<dialog id=d open><form method=dialog><button id=ok type=submit>ok17</button></form></dialog>");
    TEST_ASSERT_EQUAL(WE_DISP_BLOCK, g_doc.nodes[l74_node("d")].display);
    TEST_ASSERT_EQUAL(WE_FACT_CHANGED, l74_click("ok"));
    l74_render();
    TEST_ASSERT_EQUAL(WE_DISP_NONE, g_doc.nodes[l74_node("d")].display);
    l74_done();
}

static void test_l74_dlg_18(void) {
    l74_open("<dialog id=d open><form method=dialog><button id=ok type=submit>ok18</button></form></dialog>");
    TEST_ASSERT_EQUAL(WE_DISP_BLOCK, g_doc.nodes[l74_node("d")].display);
    TEST_ASSERT_EQUAL(WE_FACT_CHANGED, l74_click("ok"));
    l74_render();
    TEST_ASSERT_EQUAL(WE_DISP_NONE, g_doc.nodes[l74_node("d")].display);
    l74_done();
}

static void test_l74_dlg_19(void) {
    l74_open("<dialog id=d open><form method=dialog><button id=ok type=submit>ok19</button></form></dialog>");
    TEST_ASSERT_EQUAL(WE_DISP_BLOCK, g_doc.nodes[l74_node("d")].display);
    TEST_ASSERT_EQUAL(WE_FACT_CHANGED, l74_click("ok"));
    l74_render();
    TEST_ASSERT_EQUAL(WE_DISP_NONE, g_doc.nodes[l74_node("d")].display);
    l74_done();
}

static void test_l74_dlg_20(void) {
    l74_open("<dialog id=d open><form method=dialog><button id=ok type=submit>ok20</button></form></dialog>");
    TEST_ASSERT_EQUAL(WE_DISP_BLOCK, g_doc.nodes[l74_node("d")].display);
    TEST_ASSERT_EQUAL(WE_FACT_CHANGED, l74_click("ok"));
    l74_render();
    TEST_ASSERT_EQUAL(WE_DISP_NONE, g_doc.nodes[l74_node("d")].display);
    l74_done();
}

static void test_l74_dlg_21(void) {
    l74_open("<dialog id=d open><form method=dialog><button id=ok type=submit>ok21</button></form></dialog>");
    TEST_ASSERT_EQUAL(WE_DISP_BLOCK, g_doc.nodes[l74_node("d")].display);
    TEST_ASSERT_EQUAL(WE_FACT_CHANGED, l74_click("ok"));
    l74_render();
    TEST_ASSERT_EQUAL(WE_DISP_NONE, g_doc.nodes[l74_node("d")].display);
    l74_done();
}

static void test_l74_dlg_22(void) {
    l74_open("<dialog id=d open><form method=dialog><button id=ok type=submit>ok22</button></form></dialog>");
    TEST_ASSERT_EQUAL(WE_DISP_BLOCK, g_doc.nodes[l74_node("d")].display);
    TEST_ASSERT_EQUAL(WE_FACT_CHANGED, l74_click("ok"));
    l74_render();
    TEST_ASSERT_EQUAL(WE_DISP_NONE, g_doc.nodes[l74_node("d")].display);
    l74_done();
}

static void test_l74_dlg_23(void) {
    l74_open("<dialog id=d open><form method=dialog><button id=ok type=submit>ok23</button></form></dialog>");
    TEST_ASSERT_EQUAL(WE_DISP_BLOCK, g_doc.nodes[l74_node("d")].display);
    TEST_ASSERT_EQUAL(WE_FACT_CHANGED, l74_click("ok"));
    l74_render();
    TEST_ASSERT_EQUAL(WE_DISP_NONE, g_doc.nodes[l74_node("d")].display);
    l74_done();
}

static void test_l74_dlg_24(void) {
    l74_open("<dialog id=d open><form method=dialog><button id=ok type=submit>ok24</button></form></dialog>");
    TEST_ASSERT_EQUAL(WE_DISP_BLOCK, g_doc.nodes[l74_node("d")].display);
    TEST_ASSERT_EQUAL(WE_FACT_CHANGED, l74_click("ok"));
    l74_render();
    TEST_ASSERT_EQUAL(WE_DISP_NONE, g_doc.nodes[l74_node("d")].display);
    l74_done();
}

static void test_l74_dlg_25(void) {
    l74_open("<dialog id=d open><form method=dialog><button id=ok type=submit>ok25</button></form></dialog>");
    TEST_ASSERT_EQUAL(WE_DISP_BLOCK, g_doc.nodes[l74_node("d")].display);
    TEST_ASSERT_EQUAL(WE_FACT_CHANGED, l74_click("ok"));
    l74_render();
    TEST_ASSERT_EQUAL(WE_DISP_NONE, g_doc.nodes[l74_node("d")].display);
    l74_done();
}

static void test_l74_dlg_26(void) {
    l74_open("<dialog id=d open><form method=dialog><button id=ok type=submit>ok26</button></form></dialog>");
    TEST_ASSERT_EQUAL(WE_DISP_BLOCK, g_doc.nodes[l74_node("d")].display);
    TEST_ASSERT_EQUAL(WE_FACT_CHANGED, l74_click("ok"));
    l74_render();
    TEST_ASSERT_EQUAL(WE_DISP_NONE, g_doc.nodes[l74_node("d")].display);
    l74_done();
}

static void test_l74_dlg_27(void) {
    l74_open("<dialog id=d open><form method=dialog><button id=ok type=submit>ok27</button></form></dialog>");
    TEST_ASSERT_EQUAL(WE_DISP_BLOCK, g_doc.nodes[l74_node("d")].display);
    TEST_ASSERT_EQUAL(WE_FACT_CHANGED, l74_click("ok"));
    l74_render();
    TEST_ASSERT_EQUAL(WE_DISP_NONE, g_doc.nodes[l74_node("d")].display);
    l74_done();
}

static void test_l74_dlg_28(void) {
    l74_open("<dialog id=d open><form method=dialog><button id=ok type=submit>ok28</button></form></dialog>");
    TEST_ASSERT_EQUAL(WE_DISP_BLOCK, g_doc.nodes[l74_node("d")].display);
    TEST_ASSERT_EQUAL(WE_FACT_CHANGED, l74_click("ok"));
    l74_render();
    TEST_ASSERT_EQUAL(WE_DISP_NONE, g_doc.nodes[l74_node("d")].display);
    l74_done();
}

static void test_l74_dlg_29(void) {
    l74_open("<dialog id=d open><form method=dialog><button id=ok type=submit>ok29</button></form></dialog>");
    TEST_ASSERT_EQUAL(WE_DISP_BLOCK, g_doc.nodes[l74_node("d")].display);
    TEST_ASSERT_EQUAL(WE_FACT_CHANGED, l74_click("ok"));
    l74_render();
    TEST_ASSERT_EQUAL(WE_DISP_NONE, g_doc.nodes[l74_node("d")].display);
    l74_done();
}

static void lot74_sys_tests(void) {
    RUN_TEST(test_l74_pat_0);
    RUN_TEST(test_l74_pat_1);
    RUN_TEST(test_l74_pat_2);
    RUN_TEST(test_l74_pat_3);
    RUN_TEST(test_l74_pat_4);
    RUN_TEST(test_l74_pat_5);
    RUN_TEST(test_l74_pat_6);
    RUN_TEST(test_l74_pat_7);
    RUN_TEST(test_l74_pat_8);
    RUN_TEST(test_l74_pat_9);
    RUN_TEST(test_l74_pat_10);
    RUN_TEST(test_l74_pat_11);
    RUN_TEST(test_l74_pat_12);
    RUN_TEST(test_l74_pat_13);
    RUN_TEST(test_l74_pat_14);
    RUN_TEST(test_l74_pat_15);
    RUN_TEST(test_l74_pat_16);
    RUN_TEST(test_l74_pat_17);
    RUN_TEST(test_l74_pat_18);
    RUN_TEST(test_l74_pat_19);
    RUN_TEST(test_l74_pat_20);
    RUN_TEST(test_l74_pat_21);
    RUN_TEST(test_l74_pat_22);
    RUN_TEST(test_l74_pat_23);
    RUN_TEST(test_l74_pat_24);
    RUN_TEST(test_l74_pat_25);
    RUN_TEST(test_l74_pat_26);
    RUN_TEST(test_l74_pat_27);
    RUN_TEST(test_l74_pat_28);
    RUN_TEST(test_l74_pat_29);
    RUN_TEST(test_l74_pat_30);
    RUN_TEST(test_l74_pat_31);
    RUN_TEST(test_l74_pat_32);
    RUN_TEST(test_l74_pat_33);
    RUN_TEST(test_l74_pat_34);
    RUN_TEST(test_l74_pat_35);
    RUN_TEST(test_l74_pat_36);
    RUN_TEST(test_l74_pat_37);
    RUN_TEST(test_l74_pat_38);
    RUN_TEST(test_l74_pat_39);
    RUN_TEST(test_l74_pat_40);
    RUN_TEST(test_l74_pat_41);
    RUN_TEST(test_l74_pat_42);
    RUN_TEST(test_l74_pat_43);
    RUN_TEST(test_l74_pat_44);
    RUN_TEST(test_l74_pat_45);
    RUN_TEST(test_l74_pat_46);
    RUN_TEST(test_l74_pat_47);
    RUN_TEST(test_l74_pat_48);
    RUN_TEST(test_l74_pat_49);
    RUN_TEST(test_l74_pat_50);
    RUN_TEST(test_l74_pat_51);
    RUN_TEST(test_l74_pat_52);
    RUN_TEST(test_l74_pat_53);
    RUN_TEST(test_l74_pat_54);
    RUN_TEST(test_l74_pat_55);
    RUN_TEST(test_l74_pat_56);
    RUN_TEST(test_l74_pat_57);
    RUN_TEST(test_l74_pat_58);
    RUN_TEST(test_l74_pat_59);
    RUN_TEST(test_l74_pat_60);
    RUN_TEST(test_l74_pat_61);
    RUN_TEST(test_l74_pat_62);
    RUN_TEST(test_l74_pat_63);
    RUN_TEST(test_l74_pat_64);
    RUN_TEST(test_l74_pat_65);
    RUN_TEST(test_l74_pat_66);
    RUN_TEST(test_l74_pat_67);
    RUN_TEST(test_l74_pat_68);
    RUN_TEST(test_l74_pat_69);
    RUN_TEST(test_l74_pat_70);
    RUN_TEST(test_l74_pat_71);
    RUN_TEST(test_l74_pat_72);
    RUN_TEST(test_l74_pat_73);
    RUN_TEST(test_l74_pat_74);
    RUN_TEST(test_l74_pat_75);
    RUN_TEST(test_l74_pat_76);
    RUN_TEST(test_l74_pat_77);
    RUN_TEST(test_l74_pat_78);
    RUN_TEST(test_l74_pat_79);
    RUN_TEST(test_l74_nestg_0);
    RUN_TEST(test_l74_nestg_1);
    RUN_TEST(test_l74_nestg_2);
    RUN_TEST(test_l74_nestg_3);
    RUN_TEST(test_l74_nestg_4);
    RUN_TEST(test_l74_nestg_5);
    RUN_TEST(test_l74_nestg_6);
    RUN_TEST(test_l74_nestg_7);
    RUN_TEST(test_l74_nestg_8);
    RUN_TEST(test_l74_nestg_9);
    RUN_TEST(test_l74_nestg_10);
    RUN_TEST(test_l74_nestg_11);
    RUN_TEST(test_l74_nestg_12);
    RUN_TEST(test_l74_nestg_13);
    RUN_TEST(test_l74_nestg_14);
    RUN_TEST(test_l74_nestg_15);
    RUN_TEST(test_l74_nestg_16);
    RUN_TEST(test_l74_nestg_17);
    RUN_TEST(test_l74_nestg_18);
    RUN_TEST(test_l74_nestg_19);
    RUN_TEST(test_l74_nestg_20);
    RUN_TEST(test_l74_nestg_21);
    RUN_TEST(test_l74_nestg_22);
    RUN_TEST(test_l74_nestg_23);
    RUN_TEST(test_l74_nestg_24);
    RUN_TEST(test_l74_nestg_25);
    RUN_TEST(test_l74_nestg_26);
    RUN_TEST(test_l74_nestg_27);
    RUN_TEST(test_l74_nestg_28);
    RUN_TEST(test_l74_nestg_29);
    RUN_TEST(test_l74_nestg_30);
    RUN_TEST(test_l74_nestg_31);
    RUN_TEST(test_l74_nestg_32);
    RUN_TEST(test_l74_nestg_33);
    RUN_TEST(test_l74_nestg_34);
    RUN_TEST(test_l74_nestg_35);
    RUN_TEST(test_l74_nestg_36);
    RUN_TEST(test_l74_nestg_37);
    RUN_TEST(test_l74_nestg_38);
    RUN_TEST(test_l74_nestg_39);
    RUN_TEST(test_l74_nestg_40);
    RUN_TEST(test_l74_nestg_41);
    RUN_TEST(test_l74_nestg_42);
    RUN_TEST(test_l74_nestg_43);
    RUN_TEST(test_l74_nestg_44);
    RUN_TEST(test_l74_nestg_45);
    RUN_TEST(test_l74_nestg_46);
    RUN_TEST(test_l74_nestg_47);
    RUN_TEST(test_l74_nestg_48);
    RUN_TEST(test_l74_nestg_49);
    RUN_TEST(test_l74_nestg_50);
    RUN_TEST(test_l74_nestg_51);
    RUN_TEST(test_l74_nestg_52);
    RUN_TEST(test_l74_nestg_53);
    RUN_TEST(test_l74_nestg_54);
    RUN_TEST(test_l74_nestg_55);
    RUN_TEST(test_l74_nestg_56);
    RUN_TEST(test_l74_nestg_57);
    RUN_TEST(test_l74_nestg_58);
    RUN_TEST(test_l74_nestg_59);
    RUN_TEST(test_l74_tf_0);
    RUN_TEST(test_l74_tf_1);
    RUN_TEST(test_l74_tf_2);
    RUN_TEST(test_l74_tf_3);
    RUN_TEST(test_l74_tf_4);
    RUN_TEST(test_l74_tf_5);
    RUN_TEST(test_l74_tf_6);
    RUN_TEST(test_l74_tf_7);
    RUN_TEST(test_l74_tf_8);
    RUN_TEST(test_l74_tf_9);
    RUN_TEST(test_l74_tf_10);
    RUN_TEST(test_l74_tf_11);
    RUN_TEST(test_l74_tf_12);
    RUN_TEST(test_l74_tf_13);
    RUN_TEST(test_l74_tf_14);
    RUN_TEST(test_l74_tf_15);
    RUN_TEST(test_l74_tf_16);
    RUN_TEST(test_l74_tf_17);
    RUN_TEST(test_l74_tf_18);
    RUN_TEST(test_l74_tf_19);
    RUN_TEST(test_l74_tf_20);
    RUN_TEST(test_l74_tf_21);
    RUN_TEST(test_l74_tf_22);
    RUN_TEST(test_l74_tf_23);
    RUN_TEST(test_l74_tf_24);
    RUN_TEST(test_l74_tf_25);
    RUN_TEST(test_l74_tf_26);
    RUN_TEST(test_l74_tf_27);
    RUN_TEST(test_l74_tf_28);
    RUN_TEST(test_l74_tf_29);
    RUN_TEST(test_l74_tf_30);
    RUN_TEST(test_l74_tf_31);
    RUN_TEST(test_l74_tf_32);
    RUN_TEST(test_l74_tf_33);
    RUN_TEST(test_l74_tf_34);
    RUN_TEST(test_l74_tf_35);
    RUN_TEST(test_l74_tf_36);
    RUN_TEST(test_l74_tf_37);
    RUN_TEST(test_l74_tf_38);
    RUN_TEST(test_l74_tf_39);
    RUN_TEST(test_l74_ifr_0);
    RUN_TEST(test_l74_ifr_1);
    RUN_TEST(test_l74_ifr_2);
    RUN_TEST(test_l74_ifr_3);
    RUN_TEST(test_l74_ifr_4);
    RUN_TEST(test_l74_ifr_5);
    RUN_TEST(test_l74_ifr_6);
    RUN_TEST(test_l74_ifr_7);
    RUN_TEST(test_l74_ifr_8);
    RUN_TEST(test_l74_ifr_9);
    RUN_TEST(test_l74_ifr_10);
    RUN_TEST(test_l74_ifr_11);
    RUN_TEST(test_l74_ifr_12);
    RUN_TEST(test_l74_ifr_13);
    RUN_TEST(test_l74_ifr_14);
    RUN_TEST(test_l74_ifr_15);
    RUN_TEST(test_l74_ifr_16);
    RUN_TEST(test_l74_ifr_17);
    RUN_TEST(test_l74_ifr_18);
    RUN_TEST(test_l74_ifr_19);
    RUN_TEST(test_l74_ifr_20);
    RUN_TEST(test_l74_ifr_21);
    RUN_TEST(test_l74_ifr_22);
    RUN_TEST(test_l74_ifr_23);
    RUN_TEST(test_l74_ifr_24);
    RUN_TEST(test_l74_ifr_25);
    RUN_TEST(test_l74_ifr_26);
    RUN_TEST(test_l74_ifr_27);
    RUN_TEST(test_l74_ifr_28);
    RUN_TEST(test_l74_ifr_29);
    RUN_TEST(test_l74_ifr_30);
    RUN_TEST(test_l74_ifr_31);
    RUN_TEST(test_l74_ifr_32);
    RUN_TEST(test_l74_ifr_33);
    RUN_TEST(test_l74_ifr_34);
    RUN_TEST(test_l74_ifr_35);
    RUN_TEST(test_l74_ifr_36);
    RUN_TEST(test_l74_ifr_37);
    RUN_TEST(test_l74_ifr_38);
    RUN_TEST(test_l74_ifr_39);
    RUN_TEST(test_l74_dlg_0);
    RUN_TEST(test_l74_dlg_1);
    RUN_TEST(test_l74_dlg_2);
    RUN_TEST(test_l74_dlg_3);
    RUN_TEST(test_l74_dlg_4);
    RUN_TEST(test_l74_dlg_5);
    RUN_TEST(test_l74_dlg_6);
    RUN_TEST(test_l74_dlg_7);
    RUN_TEST(test_l74_dlg_8);
    RUN_TEST(test_l74_dlg_9);
    RUN_TEST(test_l74_dlg_10);
    RUN_TEST(test_l74_dlg_11);
    RUN_TEST(test_l74_dlg_12);
    RUN_TEST(test_l74_dlg_13);
    RUN_TEST(test_l74_dlg_14);
    RUN_TEST(test_l74_dlg_15);
    RUN_TEST(test_l74_dlg_16);
    RUN_TEST(test_l74_dlg_17);
    RUN_TEST(test_l74_dlg_18);
    RUN_TEST(test_l74_dlg_19);
    RUN_TEST(test_l74_dlg_20);
    RUN_TEST(test_l74_dlg_21);
    RUN_TEST(test_l74_dlg_22);
    RUN_TEST(test_l74_dlg_23);
    RUN_TEST(test_l74_dlg_24);
    RUN_TEST(test_l74_dlg_25);
    RUN_TEST(test_l74_dlg_26);
    RUN_TEST(test_l74_dlg_27);
    RUN_TEST(test_l74_dlg_28);
    RUN_TEST(test_l74_dlg_29);
}
