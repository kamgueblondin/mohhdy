#ifndef ATA_H
#define ATA_H

#include <stdint.h>

/* ATA PIO LBA28 primaire (0x1F0), maître ou esclave. No IRQ14. */
#define ATA_DRIVE_MASTER 0U
#define ATA_DRIVE_SLAVE  1U

int ata_init(void);
/* Tranche 4 slice 2: kernel PIO refused (OS_ATA_CONTROLLER_BUSY) when gate() == 0. */
void ata_set_kernel_gate(int (*gate)(void));
int ata_present(void);
int ata_channel_reset(void);
/* 1 if BSY or DRQ is set. 0 if the channel is idle or absent. */
int ata_channel_busy(void);
/* Journal sectors. syscall.c provides the image copy (Ring 3 when the
 * driver can take the job). service_registry.c keeps a weak copy that
 * calls ata_read_sectors / ata_write_sectors for host tests. */
int kernel_journal_sector_io(uint32_t lba, uint32_t count, void* buf, int write);
int ata_present_drive(uint8_t drive);
int ata_read_sectors(uint32_t lba, uint32_t count, void* buf);
int ata_read_sectors_drive(uint8_t drive, uint32_t lba, uint32_t count, void* buf);
int ata_write_sectors(uint32_t lba, uint32_t count, const void* buf);
int ata_write_sectors_drive(uint8_t drive, uint32_t lba, uint32_t count, const void* buf);
/* Kernel PIO channel recoveries (stale transfer reset or failed transfer reset). */
uint32_t ata_pio_recoveries(void);

#endif
