#include "../../framework/unity.h"
#include "../../../kernel/ipc.h"

static ipc_endpoint_t endpoint;

static os_ipc_payload_t make_payload(uint32_t type, const char* text) {
    os_ipc_payload_t payload;
    uint32_t i = 0U;
    payload.type = type;
    payload.request_id = 0U;
    while (text[i] != '\0' && i < OS_IPC_MAX_DATA) {
        payload.data[i] = (uint8_t)text[i];
        i++;
    }
    payload.size = i;
    while (i < OS_IPC_MAX_DATA) payload.data[i++] = 0U;
    return payload;
}

static void test_endpoint_is_empty_after_init(void) {
    os_ipc_message_t message;
    ipc_endpoint_init(&endpoint);
    TEST_ASSERT_EQUAL(OS_IPC_EMPTY, ipc_endpoint_receive(&endpoint, &message));
}

static void test_send_copies_payload_and_sender(void) {
    os_ipc_payload_t payload = make_payload(7U, "bonjour");
    os_ipc_message_t message;
    ipc_endpoint_init(&endpoint);
    TEST_ASSERT_EQUAL(0, ipc_endpoint_send(&endpoint, 42, &payload));
    payload.data[0] = (uint8_t)'X';
    TEST_ASSERT_EQUAL(0, ipc_endpoint_receive(&endpoint, &message));
    TEST_ASSERT_EQUAL(42, message.sender_pid);
    TEST_ASSERT_EQUAL(7, message.type);
    TEST_ASSERT_EQUAL(7, message.size);
    TEST_ASSERT_EQUAL('b', message.data[0]);
    TEST_ASSERT_EQUAL('r', message.data[6]);
}

static void test_request_id_is_copied_with_message(void) {
    os_ipc_payload_t payload = make_payload(9U, "correlation");
    os_ipc_message_t message;
    payload.request_id = 0x4a7c11U;
    ipc_endpoint_init(&endpoint);
    TEST_ASSERT_EQUAL(0, ipc_endpoint_send(&endpoint, 27, &payload));
    payload.request_id = 0U;
    TEST_ASSERT_EQUAL(0, ipc_endpoint_receive(&endpoint, &message));
    TEST_ASSERT_EQUAL(0x4a7c11U, message.request_id);
}

static void test_messages_preserve_fifo_order(void) {
    os_ipc_payload_t first = make_payload(1U, "un");
    os_ipc_payload_t second = make_payload(2U, "deux");
    os_ipc_message_t message;
    ipc_endpoint_init(&endpoint);
    TEST_ASSERT_EQUAL(0, ipc_endpoint_send(&endpoint, 10, &first));
    TEST_ASSERT_EQUAL(0, ipc_endpoint_send(&endpoint, 11, &second));
    TEST_ASSERT_EQUAL(0, ipc_endpoint_receive(&endpoint, &message));
    TEST_ASSERT_EQUAL(10, message.sender_pid);
    TEST_ASSERT_EQUAL(1, message.type);
    TEST_ASSERT_EQUAL('u', message.data[0]);
    TEST_ASSERT_EQUAL(0, ipc_endpoint_receive(&endpoint, &message));
    TEST_ASSERT_EQUAL(11, message.sender_pid);
    TEST_ASSERT_EQUAL(2, message.type);
    TEST_ASSERT_EQUAL('d', message.data[0]);
}

static void test_full_endpoint_keeps_existing_messages(void) {
    os_ipc_payload_t payload = make_payload(3U, "x");
    os_ipc_message_t message;
    uint32_t i;
    ipc_endpoint_init(&endpoint);
    for (i = 0U; i < IPC_ENDPOINT_CAPACITY; i++) {
        payload.type = i;
        TEST_ASSERT_EQUAL(0, ipc_endpoint_send(&endpoint, (int32_t)i, &payload));
    }
    TEST_ASSERT_EQUAL(OS_IPC_FULL, ipc_endpoint_send(&endpoint, 99, &payload));
    TEST_ASSERT_EQUAL(0, ipc_endpoint_receive(&endpoint, &message));
    TEST_ASSERT_EQUAL(0, message.sender_pid);
    TEST_ASSERT_EQUAL(0, message.type);
}

static void test_invalid_payload_is_rejected(void) {
    os_ipc_payload_t payload = make_payload(1U, "x");
    ipc_endpoint_init(&endpoint);
    payload.size = OS_IPC_MAX_DATA + 1U;
    TEST_ASSERT_EQUAL(OS_IPC_BAD_MESSAGE, ipc_endpoint_send(&endpoint, 1, &payload));
    TEST_ASSERT_EQUAL(OS_IPC_BAD_MESSAGE, ipc_endpoint_send(0, 1, &payload));
}

int main(void) {
    unity_init();
    RUN_TEST(test_endpoint_is_empty_after_init);
    RUN_TEST(test_send_copies_payload_and_sender);
    RUN_TEST(test_request_id_is_copied_with_message);
    RUN_TEST(test_messages_preserve_fifo_order);
    RUN_TEST(test_full_endpoint_keeps_existing_messages);
    RUN_TEST(test_invalid_payload_is_rejected);
    unity_print_results();
    unity_cleanup();
    return unity_stats.tests_failed == 0 ? 0 : 1;
}
