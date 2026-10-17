/* Lot 4 (US-001): inventory published by the Ring 3 pcidriver. */
#include "../../framework/unity.h"
#include "../../../kernel/pci_service.h"

static os_pci_record_t rec(uint8_t slot, uint8_t function, uint16_t vendor, uint8_t class_code,
                           uint8_t subclass, uint32_t bar0) {
    os_pci_record_t r = {0};
    r.slot = slot; r.function = function; r.vendor_id = vendor; r.device_id = 0x1111U;
    r.class_code = class_code; r.subclass = subclass; r.bar0 = bar0;
    return r;
}

static void test_publish_requires_live_owner(void) {
    os_pci_record_t r[1];
    os_pci_status_t st;
    pci_service_reset();
    r[0] = rec(2, 0, 0x1234, 0x03, 0x00, 0xFD000008U);
    TEST_ASSERT_EQUAL(OS_PCI_DENIED, pci_service_publish(7, 0, r, 1));
    TEST_ASSERT_EQUAL(OS_PCI_DENIED, pci_service_publish(7, 9, r, 1));
    TEST_ASSERT_EQUAL(0, pci_service_handed_off());
    TEST_ASSERT_EQUAL(OS_PCI_NO_INVENTORY, pci_service_list(r, 1));
    TEST_ASSERT_EQUAL(1, pci_service_publish(9, 9, r, 1));
    TEST_ASSERT_EQUAL(1, pci_service_handed_off());
    pci_service_status(9, &st);
    TEST_ASSERT_EQUAL(9, st.owner_pid);
    TEST_ASSERT_EQUAL(9, st.publisher_pid);
    TEST_ASSERT_EQUAL(1U, st.count);
    TEST_ASSERT_EQUAL(1U, st.generation);
    TEST_ASSERT_EQUAL(2U, st.rejected_publish);
}

static void test_publish_validates_records(void) {
    os_pci_record_t r[3];
    pci_service_reset();
    r[0] = rec(0, 0, 0x8086, 0x06, 0x00, 0);
    r[1] = rec(1, 1, 0xFFFF, 0x01, 0x01, 0);
    TEST_ASSERT_EQUAL(OS_PCI_BAD_REQUEST, pci_service_publish(3, 3, r, 2));
    r[1] = rec(40, 0, 0x8086, 0x01, 0x01, 0);
    TEST_ASSERT_EQUAL(OS_PCI_BAD_REQUEST, pci_service_publish(3, 3, r, 2));
    r[1] = rec(0, 0, 0x8086, 0x01, 0x01, 0);
    TEST_ASSERT_EQUAL(OS_PCI_BAD_REQUEST, pci_service_publish(3, 3, r, 2));
    r[1] = rec(1, 1, 0x8086, 0x01, 0x01, 0);
    r[1].reserved = 1U;
    TEST_ASSERT_EQUAL(OS_PCI_BAD_REQUEST, pci_service_publish(3, 3, r, 2));
    TEST_ASSERT_EQUAL(OS_PCI_BAD_REQUEST, pci_service_publish(3, 3, r, 0));
    TEST_ASSERT_EQUAL(OS_PCI_BAD_REQUEST, pci_service_publish(3, 3, r, OS_PCI_MAX_RECORDS + 1U));
    TEST_ASSERT_EQUAL(OS_PCI_BAD_REQUEST, pci_service_publish(3, 3, 0, 1));
    TEST_ASSERT_EQUAL(0, pci_service_handed_off());
    r[1].reserved = 0U;
    r[2] = rec(2, 0, 0x1234, 0x03, 0x00, 0xFD000008U);
    TEST_ASSERT_EQUAL(3, pci_service_publish(3, 3, r, 3));
}

static void test_list_and_find_class(void) {
    os_pci_record_t r[3], out[4], vga;
    pci_service_reset();
    r[0] = rec(0, 0, 0x8086, 0x06, 0x00, 0);
    r[1] = rec(1, 1, 0x8086, 0x01, 0x01, 0);
    r[2] = rec(2, 0, 0x1234, 0x03, 0x00, 0xFD000008U);
    TEST_ASSERT_EQUAL(3, pci_service_publish(5, 5, r, 3));
    TEST_ASSERT_EQUAL(2, pci_service_list(out, 2));
    TEST_ASSERT_EQUAL(3, pci_service_list(out, 4));
    TEST_ASSERT_EQUAL(0x1234U, out[2].vendor_id);
    TEST_ASSERT_EQUAL(0, pci_service_find_class(0x03, 0x80, 1, &vga));
    TEST_ASSERT_EQUAL(0xFD000008U, vga.bar0);
    TEST_ASSERT_EQUAL(-1, pci_service_find_class(0x03, 0x80, 0, &vga));
    TEST_ASSERT_EQUAL(0, pci_service_find_class(0x01, 0x01, 0, &vga));
    TEST_ASSERT_EQUAL(-1, pci_service_find_class(0x02, 0x00, 1, 0));
    TEST_ASSERT_EQUAL(OS_PCI_BAD_REQUEST, pci_service_list(0, 4));
}

static void test_supervisor_respawn_budget_and_stale(void) {
    os_pci_record_t r[1];
    os_pci_status_t st;
    uint32_t i;
    pci_service_reset();
    /* Avant toute publication : aucun redemarrage (le chemin Ring 0 de boot sert). */
    TEST_ASSERT_EQUAL(0, pci_service_should_respawn(0, 0));
    r[0] = rec(0, 0, 0x8086, 0x06, 0x00, 0);
    TEST_ASSERT_EQUAL(1, pci_service_publish(4, 4, r, 1));
    TEST_ASSERT_EQUAL(0, pci_service_should_respawn(4, 0));
    pci_service_owner_gone();
    pci_service_status(0, &st);
    TEST_ASSERT_EQUAL(1U, st.stale);
    TEST_ASSERT_EQUAL(1U, st.count);
    TEST_ASSERT_EQUAL(1, pci_service_list(r, 1));
    TEST_ASSERT_EQUAL(0, pci_service_should_respawn(0, 1));
    for (i = 0U; i < PCI_SERVICE_RESPAWN_LIMIT; i++) {
        TEST_ASSERT_EQUAL(1, pci_service_should_respawn(0, 0));
        pci_service_note_respawn();
    }
    TEST_ASSERT_EQUAL(0, pci_service_should_respawn(0, 0));
    /* Republication par le nouveau proprietaire : plus perime, generation 2. */
    TEST_ASSERT_EQUAL(1, pci_service_publish(11, 11, r, 1));
    pci_service_status(11, &st);
    TEST_ASSERT_EQUAL(0U, st.stale);
    TEST_ASSERT_EQUAL(2U, st.generation);
    TEST_ASSERT_EQUAL(PCI_SERVICE_RESPAWN_LIMIT, st.respawns);
    pci_service_note_ring0_denied();
    pci_service_status(11, &st);
    TEST_ASSERT_EQUAL(1U, st.ring0_denied);
}

int main(void) {
    unity_init();
    RUN_TEST(test_publish_requires_live_owner);
    RUN_TEST(test_publish_validates_records);
    RUN_TEST(test_list_and_find_class);
    RUN_TEST(test_supervisor_respawn_budget_and_stale);
    unity_print_results();
    unity_cleanup();
    return unity_stats.tests_failed == 0 ? 0 : 1;
}
