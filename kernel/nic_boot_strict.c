/* Strict image: Ring 0 does not probe or reset the NE2000. */
#include "nic_boot.h"

void print_string(const char* str);
void print_string_serial(const char* str);

void kernel_nic_boot_probe(void) {
    kernel_nic_prepare_io();
    if (!kernel_nic_io_ready()) return;
    kernel_nic_set_deferred(1);
    print_string("NE2000 : sonde et reset delegues au networker Ring 3 (noyau strict).\n");
    print_string_serial("[NET] NE2000 probe deferred to the Ring 3 networker\n");
}

int kernel_nic_reclaim_hw(void) {
    print_string_serial("[NET] NE2000 released after worker loss; reset left to the next Ring 3 networker\n");
    return 0;
}
