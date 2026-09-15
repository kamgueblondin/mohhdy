#!/usr/bin/env python3
"""Create the deterministic FAT16 fixture after the 64-sector AIOV area."""
import argparse
from pathlib import Path

SECTOR = 512
BASE_LBA = 64
TOTAL_DISK_SECTORS = 4224
TOTAL_FAT_SECTORS = TOTAL_DISK_SECTORS - BASE_LBA
FAT_SECTORS = 16
ROOT_ENTRIES = 32
ROOT_SECTORS = (ROOT_ENTRIES * 32 + SECTOR - 1) // SECTOR
DATA_RELATIVE = 1 + 2 * FAT_SECTORS + ROOT_SECTORS


def put16(buf, off, value):
    buf[off:off + 2] = int(value).to_bytes(2, "little")


def put32(buf, off, value):
    buf[off:off + 4] = int(value).to_bytes(4, "little")


def short_name(name):
    base, ext = name.split(".", 1)
    return base.upper().ljust(8)[:8].encode("ascii") + ext.upper().ljust(3)[:3].encode("ascii")


def root_entry(buf, index, name, size, first_cluster):
    off = (BASE_LBA * SECTOR) + (1 + 2 * FAT_SECTORS) * SECTOR + index * 32
    buf[off:off + 11] = short_name(name)
    buf[off + 11] = 0x20
    put16(buf, off + 26, first_cluster)
    put32(buf, off + 28, size)


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--image", required=True)
    args = parser.parse_args()
    image = bytearray(TOTAL_DISK_SECTORS * SECTOR)
    boot = BASE_LBA * SECTOR
    image[boot:boot + 3] = b"\xeb\x3c\x90"
    image[boot + 3:boot + 11] = b"MOHHDY  "
    put16(image, boot + 11, SECTOR)
    image[boot + 13] = 1
    put16(image, boot + 14, 1)
    image[boot + 16] = 2
    put16(image, boot + 17, ROOT_ENTRIES)
    put16(image, boot + 19, TOTAL_FAT_SECTORS)
    image[boot + 21] = 0xF8
    put16(image, boot + 22, FAT_SECTORS)
    put16(image, boot + 24, 32)
    put16(image, boot + 26, 64)
    put32(image, boot + 28, BASE_LBA)
    put32(image, boot + 32, 0)
    image[boot + 36] = 0x80
    image[boot + 38] = 0x29
    put32(image, boot + 39, 0x20260816)
    image[boot + 43:boot + 54] = b"MOHHDY F16 "
    image[boot + 54:boot + 62] = b"FAT16   "
    image[boot + 510:boot + 512] = b"\x55\xaa"

    fat_start = (BASE_LBA + 1) * SECTOR
    for copy in range(2):
        fat = fat_start + copy * FAT_SECTORS * SECTOR
        put16(image, fat + 0, 0xFFF8)
        put16(image, fat + 2, 0xFFFF)
        put16(image, fat + 4, 0xFFFF)
        for cluster in range(3, 11):
            put16(image, fat + cluster * 2, 0xFFFF if cluster == 10 else cluster + 1)

    root_entry(image, 0, "FATOK.TXT", len(b"FAT16 fixture OK\n"), 2)
    root_entry(image, 1, "BIGFILE.BIN", 4096, 3)
    data_start = (BASE_LBA + DATA_RELATIVE) * SECTOR
    image[data_start:data_start + len(b"FAT16 fixture OK\n")] = b"FAT16 fixture OK\n"
    big = bytes((i % 251 for i in range(4096)))
    image[data_start + SECTOR:data_start + SECTOR + len(big)] = big
    Path(args.image).write_bytes(image)


if __name__ == "__main__":
    main()
