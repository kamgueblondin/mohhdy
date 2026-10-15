/* Both images: Ring 0 does not send ATA IDENTIFY (0xEC).
 * Presence is the floating-bus status probe in ata_init. The Ring 3
 * atadriver sends IDENTIFY after it owns the ports.
 * This file deliberately does not define kernel_ring0_ata_identify. */
#include "ata.h"

int ata_boot_identify(void) {
    return 0;
}
