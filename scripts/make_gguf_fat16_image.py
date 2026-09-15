#!/usr/bin/env python3
"""Build a FAT16 MOHHDY deployment disk carrying one GPT-2 GGUF as GPT2.GGU."""

import argparse
import math
import shutil
from pathlib import Path

SECTOR = 512
BASE_LBA = 64
SECTORS_PER_CLUSTER = 8
FAT_COUNT = 2
ROOT_ENTRIES = 128
RESERVED_SECTORS = 1
FAT16_EOC = 0xFFFF
FIXTURE_TEXT = b"FAT16 fixture OK\n"


def put16(buffer, offset, value):
    buffer[offset:offset + 2] = int(value).to_bytes(2, "little")


def put32(buffer, offset, value):
    buffer[offset:offset + 4] = int(value).to_bytes(4, "little")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--model", required=True, type=Path)
    parser.add_argument("--image", required=True, type=Path)
    parser.add_argument("--reserve-clusters", type=int, default=1024)
    args = parser.parse_args()

    if not args.model.is_file():
        raise SystemExit(f"modele introuvable: {args.model}")
    if args.reserve_clusters < 0:
        raise SystemExit("reserve-clusters doit etre positif")

    model_size = args.model.stat().st_size
    cluster_bytes = SECTOR * SECTORS_PER_CLUSTER
    file_clusters = math.ceil(model_size / cluster_bytes)
    cluster_count = file_clusters + args.reserve_clusters
    if cluster_count < 4085 or cluster_count > 65524:
        raise SystemExit("nombre de clusters incompatible FAT16")
    fat_sectors = math.ceil((cluster_count + 2) * 2 / SECTOR)
    root_sectors = math.ceil(ROOT_ENTRIES * 32 / SECTOR)
    total_sectors = RESERVED_SECTORS + FAT_COUNT * fat_sectors + root_sectors + cluster_count * SECTORS_PER_CLUSTER
    total_disk_sectors = BASE_LBA + total_sectors
    data_lba = BASE_LBA + RESERVED_SECTORS + FAT_COUNT * fat_sectors + root_sectors

    args.image.parent.mkdir(parents=True, exist_ok=True)
    with args.image.open("wb") as image:
        image.truncate(total_disk_sectors * SECTOR)
        boot = bytearray(SECTOR)
        boot[:3] = b"\xeb\x3c\x90"
        boot[3:11] = b"MOHHDYG "
        put16(boot, 11, SECTOR)
        boot[13] = SECTORS_PER_CLUSTER
        put16(boot, 14, RESERVED_SECTORS)
        boot[16] = FAT_COUNT
        put16(boot, 17, ROOT_ENTRIES)
        put16(boot, 19, 0)
        boot[21] = 0xF8
        put16(boot, 22, fat_sectors)
        put16(boot, 24, 32)
        put16(boot, 26, 64)
        put32(boot, 28, BASE_LBA)
        put32(boot, 32, total_sectors)
        boot[36] = 0x80
        boot[38] = 0x29
        put32(boot, 39, 0xA1052026)
        boot[43:54] = b"MOHHDY GGUF"
        boot[54:62] = b"FAT16   "
        boot[510:512] = b"\x55\xaa"
        image.seek(BASE_LBA * SECTOR)
        image.write(boot)

        fat = bytearray(fat_sectors * SECTOR)
        put16(fat, 0, 0xFFF8)
        put16(fat, 2, FAT16_EOC)
        for cluster in range(2, file_clusters + 1):
            put16(fat, cluster * 2, cluster + 1)
        put16(fat, (file_clusters + 1) * 2, FAT16_EOC)
        fixture_cluster = file_clusters + 2
        put16(fat, fixture_cluster * 2, FAT16_EOC)
        for copy in range(FAT_COUNT):
            image.seek((BASE_LBA + RESERVED_SECTORS + copy * fat_sectors) * SECTOR)
            image.write(fat)

        root = bytearray(root_sectors * SECTOR)
        root[:11] = b"GPT2    GGU"
        root[11] = 0x20
        put16(root, 26, 2)
        put32(root, 28, model_size)
        root[32:43] = b"FATOK   TXT"
        root[32 + 11] = 0x20
        put16(root, 32 + 26, fixture_cluster)
        put32(root, 32 + 28, len(FIXTURE_TEXT))
        image.seek((BASE_LBA + RESERVED_SECTORS + FAT_COUNT * fat_sectors) * SECTOR)
        image.write(root)

        image.seek(data_lba * SECTOR)
        with args.model.open("rb") as source:
            shutil.copyfileobj(source, image, length=1024 * 1024)
        image.seek(data_lba * SECTOR + (fixture_cluster - 2) * cluster_bytes)
        image.write(FIXTURE_TEXT)

    print(f"GGUF FAT16: {args.image} ({total_disk_sectors * SECTOR} octets)")
    print(f"GPT2.GGU: {model_size} octets, {file_clusters} clusters de {cluster_bytes} octets")


if __name__ == "__main__":
    main()
