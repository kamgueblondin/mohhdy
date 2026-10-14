/* Legacy image: Ring 0 mounts FAT and loads the overlay before any task. */
#include "boot_storage.h"
#include "ata.h"
#include "ata_job.h"
#include "fs/fat16.h"
#include "fs/fat32.h"
#include "llm/gpt2_gguf_infer.h"
#include "../fs/overlay.h"

void print_string(const char* str);

void kernel_ring0_boot_fat_mount(void) {
    if (overlay_load_disk() == 0) {
        print_string("Overlay FS charge depuis le disque IDE.\n");
    } else {
        print_string("Overlay FS initialise (disque IDE vide).\n");
    }
    if (ata_present_drive(ATA_DRIVE_SLAVE) &&
        fat32_mount(fat32_root(), kernel_fat32_ata_slave_read_sector, 0U) == 0) {
        if (fat32_attach_writer(fat32_root(), kernel_fat32_ata_slave_write_sector) != 0) {
            print_string("FAT32: writer ATA esclave indisponible; creation desactivee.\n");
        }
        print_string("FAT32 secondaire monte.\n");
    }
    if (fat16_mount(fat16_root(), kernel_fat16_ata_read_sector, 64U) == 0) {
        kernel_fat16_bind_io();
        print_string(fat16_status());
        print_string("\n");
        if (gpt2_gguf_infer_init_fat16(fat16_root(), "GPT2.GGU") == 0) {
            print_string(gpt2_gguf_infer_status());
            print_string("\n");
        } else {
            print_string("GGUF FAT16 optionnel indisponible; profil .gguf desactive.\n");
        }
    } else {
        print_string(fat16_status());
        print_string("\n");
    }
}

void kernel_boot_storage(void) {
    kernel_ring0_boot_fat_mount();
    ata_job_set_boot_flags(0U, ata_present_drive(ATA_DRIVE_SLAVE) ? 1U : 0U);
    kernel_storage_refresh_fences();
}
