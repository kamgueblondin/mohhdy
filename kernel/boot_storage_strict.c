/* Strict image: no Ring 0 FAT mount and no overlay load at boot.
 * The Ring 3 atadriver reads the boot sector and installs it. */
#include "boot_storage.h"
#include "ata.h"
#include "ata_job.h"

void print_string_serial(const char* str);

/* The boot journals (mount LBA 4222, events LBA 4224) are read by the
 * Ring 3 atadriver too (OS_ATA_FS_INSTALL_*_JOURNAL). No task can run at
 * service_registry_init, so the strict image does not read them there. */
static int g_journal_deferred;
int kernel_boot_journal_deferred(void) { return g_journal_deferred; }

void kernel_boot_storage(void) {
    g_journal_deferred = 1;
    ata_job_set_boot_flags(1U, ata_present_drive(ATA_DRIVE_SLAVE) ? 1U : 0U);
    kernel_storage_refresh_fences();
    print_string_serial("[ATA] boot FAT mount deferred to the Ring 3 atadriver\n");
}
