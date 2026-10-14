/* Legacy image: Ring 0 probe, MAC read and reclaim after a worker loss. */
#include "nic_boot.h"
#include "net_llm_client.h"
#include "syscall/syscall.h"
#include "tls_test_leaf.h"

void print_string(const char* str);
void print_string_serial(const char* str);

void kernel_nic_boot_probe(void) {
    ne2k_device_t* device;
    ne2k_io_t* io;
    kernel_nic_prepare_io();
    if (!kernel_nic_io_ready()) return;
    if (!syscall_net_ring0_fallback_enabled()) {
        kernel_nic_set_deferred(1);
        print_string("NE2000 : sonde et reset delegues au networker Ring 3 (noyau strict).\n");
        print_string_serial("[NET] NE2000 probe deferred to the Ring 3 networker\n");
        return;
    }
    device = kernel_nic_device();
    io = kernel_nic_io();
    if (ne2k_probe(device, 0x300U, io) != 0) {
        print_string("NE2000 ISA absent; reseau reste desactive.\\n");
        return;
    }
    if (ne2k_prepare(device, io) != 0 ||
        ne2k_read_mac(device, io) != 0 ||
        ne2k_configure_rings(device, io) != 0) {
        print_string("NE2000 detecte mais initialisation ou MAC incomplete.\\n");
        return;
    }
    if (ne2k_irq_attach(device, io) != 0) {
        print_string("NE2000 detecte mais IRQ non attachee.\\n");
        return;
    }
    kernel_nic_set_present(1);
    net_llm_client_bind(device, io, 1);
    print_string("NE2000 ISA detecte, MAC valide et anneaux RX/TX configures.\\n");
    print_string(boot_llm_test_trust_anchor_ready ?
        "Ancre TLS de test locale prete (example.com).\n" :
        "Ancre TLS de test locale indisponible.\n");
}

int kernel_nic_reclaim_hw(void) {
    ne2k_device_t* device = kernel_nic_device();
    ne2k_io_t* io = kernel_nic_io();
    if (ne2k_probe(device, 0x300U, io) != 0 ||
        ne2k_prepare(device, io) != 0 ||
        ne2k_read_mac(device, io) != 0 ||
        ne2k_configure_rings(device, io) != 0) return -1;
    io->outb(io->context, (uint16_t)(0x300U + 0x0FU), 0x00U);
    (void)ne2k_irq_attach(device, io);
    return 0;
}
