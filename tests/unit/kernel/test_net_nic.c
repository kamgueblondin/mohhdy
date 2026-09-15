#include "../../framework/unity.h"
#include "../../../kernel/net_nic.h"

void setUp(void) {}
void tearDown(void) {}

void test_queue_is_fifo_and_caller_owned(void) {
    uint8_t storage[NET_NIC_QUEUE_CAPACITY * 64U] = {0};
    net_nic_queue_t queue;
    uint8_t* buffer;
    uint16_t capacity;
    uint16_t length;
    TEST_ASSERT_EQUAL(0, net_nic_queue_init(&queue, storage, sizeof(storage), 64));
    TEST_ASSERT_EQUAL(0, net_nic_queue_acquire(&queue, &buffer, &capacity));
    TEST_ASSERT_EQUAL(64, capacity);
    buffer[0] = 0xa5;
    TEST_ASSERT_EQUAL(0, net_nic_queue_commit(&queue, 1));
    TEST_ASSERT_EQUAL(1, net_nic_queue_count(&queue));
    TEST_ASSERT_EQUAL(0, net_nic_queue_pop(&queue, &buffer, &length));
    TEST_ASSERT_EQUAL(1, length);
    TEST_ASSERT_EQUAL(0xa5, buffer[0]);
}

void test_queue_push_frame_validates_ethernet_size(void) {
    uint8_t storage[NET_NIC_QUEUE_CAPACITY * 64U] = {0};
    uint8_t frame[60] = {0};
    net_nic_queue_t queue;
    uint8_t* received;
    uint16_t length;
    frame[0] = 0xa5;
    TEST_ASSERT_EQUAL(0, net_nic_queue_init(&queue, storage, sizeof(storage), 64));
    TEST_ASSERT_NOT_EQUAL(0, net_nic_queue_push_frame(&queue, frame, 59));
    TEST_ASSERT_EQUAL(0, net_nic_queue_push_frame(&queue, frame, 60));
    TEST_ASSERT_EQUAL(0, net_nic_queue_pop(&queue, &received, &length));
    TEST_ASSERT_EQUAL(60, length);
    TEST_ASSERT_EQUAL(0xa5, received[0]);
}

void test_queue_rejects_bad_capacity_and_reset(void) {
    uint8_t storage[NET_NIC_QUEUE_CAPACITY * 64U] = {0};
    net_nic_queue_t queue;
    uint8_t* buffer;
    uint16_t capacity;
    TEST_ASSERT_NOT_EQUAL(0, net_nic_queue_init(&queue, storage, sizeof(storage), 0));
    TEST_ASSERT_EQUAL(0, net_nic_queue_init(&queue, storage, sizeof(storage), 64));
    TEST_ASSERT_EQUAL(0, net_nic_queue_acquire(&queue, &buffer, &capacity));
    TEST_ASSERT_EQUAL(0, net_nic_queue_commit(&queue, 3));
    net_nic_queue_reset(&queue);
    TEST_ASSERT_EQUAL(0, net_nic_queue_count(&queue));
}

int main(void) {
    unity_init();
    RUN_TEST(test_queue_is_fifo_and_caller_owned);
    RUN_TEST(test_queue_push_frame_validates_ethernet_size);
    RUN_TEST(test_queue_rejects_bad_capacity_and_reset);
    unity_print_results();
    unity_cleanup();
    return (unity_stats.tests_failed == 0) ? 0 : 1;
}
