#include "../../framework/unity.h"
#include "../../../include/os_ipc_deferred.h"

static os_ipc_message_t make_message(int32_t sender, uint32_t type,
                                     uint32_t request_id, uint8_t byte) {
    os_ipc_message_t message;
    uint32_t i;
    message.sender_pid = sender;
    message.type = type;
    message.request_id = request_id;
    message.size = 1U;
    for (i = 0U; i < OS_IPC_MAX_DATA; i++) message.data[i] = 0U;
    message.data[0] = byte;
    return message;
}

static void test_deferred_queue_starts_empty(void) {
    os_ipc_deferred_t queue;
    os_ipc_message_t out;
    os_ipc_deferred_init(&queue);
    TEST_ASSERT_EQUAL(0, queue.count);
    TEST_ASSERT_EQUAL(OS_IPC_EMPTY, os_ipc_deferred_take(&queue, &out));
}

static void test_deferred_queue_preserves_fifo(void) {
    os_ipc_deferred_t queue;
    os_ipc_message_t first = make_message(1, 3U, 10U, 'a');
    os_ipc_message_t second = make_message(2, 4U, 11U, 'b');
    os_ipc_message_t out;
    os_ipc_deferred_init(&queue);
    TEST_ASSERT_EQUAL(0, os_ipc_deferred_push(&queue, &first));
    TEST_ASSERT_EQUAL(0, os_ipc_deferred_push(&queue, &second));
    TEST_ASSERT_EQUAL(0, os_ipc_deferred_take(&queue, &out));
    TEST_ASSERT_EQUAL(1, out.sender_pid);
    TEST_ASSERT_EQUAL(10, out.request_id);
    TEST_ASSERT_EQUAL('a', out.data[0]);
    TEST_ASSERT_EQUAL(0, os_ipc_deferred_take(&queue, &out));
    TEST_ASSERT_EQUAL(2, out.sender_pid);
    TEST_ASSERT_EQUAL(11, out.request_id);
}

static void test_deferred_queue_extracts_match_without_losing_others(void) {
    os_ipc_deferred_t queue;
    os_ipc_message_t first = make_message(1, 9U, 2U, 'x');
    os_ipc_message_t match = make_message(3, 7U, 42U, 'y');
    os_ipc_message_t last = make_message(4, 9U, 3U, 'z');
    os_ipc_message_t out;
    os_ipc_deferred_init(&queue);
    TEST_ASSERT_EQUAL(0, os_ipc_deferred_push(&queue, &first));
    TEST_ASSERT_EQUAL(0, os_ipc_deferred_push(&queue, &match));
    TEST_ASSERT_EQUAL(0, os_ipc_deferred_push(&queue, &last));
    TEST_ASSERT_EQUAL(0, os_ipc_deferred_take_matching(&queue, 7U, 42U, &out));
    TEST_ASSERT_EQUAL(3, out.sender_pid);
    TEST_ASSERT_EQUAL('y', out.data[0]);
    TEST_ASSERT_EQUAL(2, queue.count);
    TEST_ASSERT_EQUAL(0, os_ipc_deferred_take(&queue, &out));
    TEST_ASSERT_EQUAL('x', out.data[0]);
    TEST_ASSERT_EQUAL(0, os_ipc_deferred_take(&queue, &out));
    TEST_ASSERT_EQUAL('z', out.data[0]);
}

static void test_deferred_queue_matches_sender_without_discarding_homonym(void) {
    os_ipc_deferred_t queue;
    os_ipc_message_t alien = make_message(8, 7U, 42U, 'a');
    os_ipc_message_t expected = make_message(3, 7U, 42U, 'b');
    os_ipc_message_t out;
    os_ipc_deferred_init(&queue);
    TEST_ASSERT_EQUAL(0, os_ipc_deferred_push(&queue, &alien));
    TEST_ASSERT_EQUAL(0, os_ipc_deferred_push(&queue, &expected));
    TEST_ASSERT_EQUAL(0, os_ipc_deferred_take_matching_from(&queue, 3, 7U, 42U, &out));
    TEST_ASSERT_EQUAL(3, out.sender_pid);
    TEST_ASSERT_EQUAL('b', out.data[0]);
    TEST_ASSERT_EQUAL(1U, queue.count);
    TEST_ASSERT_EQUAL(0, os_ipc_deferred_take_matching_from(&queue, 8, 7U, 42U, &out));
    TEST_ASSERT_EQUAL(8, out.sender_pid);
    TEST_ASSERT_EQUAL('a', out.data[0]);
}

static void test_deferred_queue_is_bounded_and_validates_message(void) {
    os_ipc_deferred_t queue;
    os_ipc_message_t message = make_message(1, 1U, 1U, 'q');
    uint32_t i;
    os_ipc_deferred_init(&queue);
    for (i = 0U; i < OS_IPC_DEFERRED_CAPACITY; i++) {
        TEST_ASSERT_EQUAL(0, os_ipc_deferred_push(&queue, &message));
    }
    TEST_ASSERT_EQUAL(OS_IPC_FULL, os_ipc_deferred_push(&queue, &message));
    message.size = OS_IPC_MAX_DATA + 1U;
    TEST_ASSERT_EQUAL(OS_IPC_BAD_MESSAGE, os_ipc_deferred_push(&queue, &message));
}

int main(void) {
    unity_init();
    RUN_TEST(test_deferred_queue_starts_empty);
    RUN_TEST(test_deferred_queue_preserves_fifo);
    RUN_TEST(test_deferred_queue_extracts_match_without_losing_others);
    RUN_TEST(test_deferred_queue_matches_sender_without_discarding_homonym);
    RUN_TEST(test_deferred_queue_is_bounded_and_validates_message);
    unity_print_results();
    unity_cleanup();
    return unity_stats.tests_failed == 0 ? 0 : 1;
}
