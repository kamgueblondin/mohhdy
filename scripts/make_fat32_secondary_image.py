#!/usr/bin/env python3
"""Construit une image FAT32 secondaire minimale pour le smoke multi-disque MOHHDY."""
import argparse
from pathlib import Path

SECTOR = 512
SECTORS_PER_CLUSTER = 1
RESERVED = 32
FAT_COUNT = 2
# Avec un cluster d’un secteur, le monteur FAT32 exige au moins 65 525 clusters.
FAT_SECTORS = 600
TOTAL_SECTORS = 70000
ROOT_CLUSTER = 2
FIXTURE_CLUSTER = 3
FIXTURE = b"FAT32 secondary fixture OK\n"


def put16(buf, offset, value):
    buf[offset:offset + 2] = int(value).to_bytes(2, "little")


def put32(buf, offset, value):
    buf[offset:offset + 4] = int(value).to_bytes(4, "little")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--image", required=True, type=Path)
    args = parser.parse_args()
    data_lba = RESERVED + FAT_COUNT * FAT_SECTORS
    args.image.parent.mkdir(parents=True, exist_ok=True)
    with args.image.open("wb") as image:
        image.truncate(TOTAL_SECTORS * SECTOR)
        boot = bytearray(SECTOR)
        boot[:3] = b"\xeb\x58\x90"
        boot[3:11] = b"MOHHDY  "
        put16(boot, 11, SECTOR)
        boot[13] = SECTORS_PER_CLUSTER
        put16(boot, 14, RESERVED)
        boot[16] = FAT_COUNT
        put16(boot, 17, 0)
        put16(boot, 19, 0)
        boot[21] = 0xF8
        put16(boot, 22, 0)
        put16(boot, 24, 32)
        put16(boot, 26, 64)
        put32(boot, 28, 0)
        put32(boot, 32, TOTAL_SECTORS)
        put32(boot, 36, FAT_SECTORS)
        put32(boot, 44, ROOT_CLUSTER)
        boot[64] = 0x80
        boot[66] = 0x29
        put32(boot, 67, 0xA105F320)
        boot[71:82] = b"MOHHDY F32 "
        boot[82:90] = b"FAT32   "
        boot[510:512] = b"\x55\xaa"
        image.seek(0)
        image.write(boot)
        fat = bytearray(SECTOR)
        put32(fat, 0, 0x0FFFFFF8)
        put32(fat, 4, 0x0FFFFFFF)
        put32(fat, ROOT_CLUSTER * 4, 0x0FFFFFFF)
        put32(fat, FIXTURE_CLUSTER * 4, 0x0FFFFFFF)
        for copy in range(FAT_COUNT):
            image.seek((RESERVED + copy * FAT_SECTORS) * SECTOR)
            image.write(fat)
        root = bytearray(SECTOR)
        root[:11] = b"FAT32OK TXT"
        root[11] = 0x20
        put16(root, 20, 0)
        put16(root, 26, FIXTURE_CLUSTER)
        put32(root, 28, len(FIXTURE))
        image.seek((data_lba + (ROOT_CLUSTER - 2) * SECTORS_PER_CLUSTER) * SECTOR)
        image.write(root)
        image.seek((data_lba + (FIXTURE_CLUSTER - 2) * SECTORS_PER_CLUSTER) * SECTOR)
        image.write(FIXTURE)
    print(f"FAT32 secondaire: {args.image} ({TOTAL_SECTORS * SECTOR} octets)")


if __name__ == "__main__":
    main()
