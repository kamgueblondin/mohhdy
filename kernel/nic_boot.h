#ifndef MOHHDY_NIC_BOOT_H
#define MOHHDY_NIC_BOOT_H
#include "ne2k.h"

/* Shared NIC gate in kernel.c. Probe and reclaim live in a file linked only
 * into the image that is allowed to touch the card from Ring 0. */
void kernel_nic_prepare_io(void);
ne2k_device_t* kernel_nic_device(void);
ne2k_io_t* kernel_nic_io(void);
void kernel_nic_set_present(int present);
void kernel_nic_set_deferred(int deferred);
int kernel_nic_deferred(void);
int kernel_nic_io_ready(void);

void kernel_nic_boot_probe(void);
/* 0 on success. The strict image stub never touches the card. */
int kernel_nic_reclaim_hw(void);
#endif
