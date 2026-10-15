/* Both images: install a boot sector or overlay snapshot copied by Ring 3.
 * The read callback serves that one buffer, then the volume is rebound to
 * the kernel disk callbacks. The legacy image serves those with Ring 0 PIO.
 * The strict image refuses them; a later driver death respawns bin/atadriver. */
#include "boot_storage.h"
#include "fs/fat16.h"
#include "fs/fat32.h"
#include "../fs/overlay.h"
#include "mem/string.h"

void print_string(const char* str);
void print_string_serial(const char* str);

static const uint8_t* g_install_sector;

static int install_sector_read(uint32_t lba, void* buffer) {
    (void)lba;
    if (!g_install_sector) return -1;
    memcpy(buffer, g_install_sector, 512);
    return 0;
}

int kernel_boot_storage_install(uint32_t kind, const uint8_t* data, uint32_t len) {
    if (!data || len == 0U) return -1;
    if (kind == OS_ATA_FS_INSTALL_FAT16) {
        if (len < 512U || fat16_is_mounted(fat16_root())) return fat16_is_mounted(fat16_root()) ? 0 : -1;
        g_install_sector = data;
        if (fat16_mount(fat16_root(), install_sector_read, 64U) != 0) {
            g_install_sector = 0;
            return -1;
        }
        g_install_sector = 0;
        kernel_fat16_bind_io();
        kernel_storage_refresh_fences();
        print_string_serial("[ATA] FAT16 boot sector installed by Ring 3 atadriver\n");
        return 0;
    }
    if (kind == OS_ATA_FS_INSTALL_FAT32) {
        if (len < 512U || fat32_is_mounted(fat32_root())) return fat32_is_mounted(fat32_root()) ? 0 : -1;
        g_install_sector = data;
        if (fat32_mount(fat32_root(), install_sector_read, 0U) != 0) {
            g_install_sector = 0;
            return -1;
        }
        g_install_sector = 0;
        kernel_fat32_bind_io();
        kernel_storage_refresh_fences();
        print_string_serial("[ATA] FAT32 boot sector installed by Ring 3 atadriver\n");
        return 0;
    }
    if (kind == OS_ATA_FS_INSTALL_OVERLAY) {
        int restored;
        if (len > 64U * 512U) len = 64U * 512U;
        /* An empty disk is not a failure: RAM keeps the overlay_init image. */
        restored = overlay_restore(data, len);
        if (restored == 0) {
            print_string("Overlay FS charge depuis le disque IDE.\n");
        } else {
            print_string("Overlay FS initialise (disque IDE vide).\n");
        }
        print_string_serial("[ATA] overlay snapshot installed by Ring 3 atadriver\n");
        return 0;
    }
    return -1;
}
