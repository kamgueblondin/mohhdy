/* Strict image: no Ring 0 FAT mount and no overlay load at boot.
 * The Ring 3 atadriver reads the boot sector and installs it. */
#include "boot_storage.h"
#include "ata.h"
#include "ata_job.h"

void print_string_serial(const char* str);

void kernel_boot_storage(void) {
    ata_job_set_boot_flags(1U, ata_present_drive(ATA_DRIVE_SLAVE) ? 1U : 0U);
    kernel_storage_refresh_fences();
    print_string_serial("[ATA] boot FAT mount deferred to the Ring 3 atadriver\n");
}
