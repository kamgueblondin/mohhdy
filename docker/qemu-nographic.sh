#!/bin/sh
# Boot Multiboot Mohhdy OS under QEMU (serial console). Not a Python HTTP sidecar.
set -eu

KERNEL="${MOHHDY_KERNEL:-/os/mohhdy.bin}"
INITRD="${MOHHDY_INITRD:-/os/my_initrd.tar}"
DISK="${MOHHDY_DISK:-/os/overlay.img}"
RAM="${MOHHDY_RAM:-256M}"

exec qemu-system-i386 \
    -kernel "$KERNEL" \
    -initrd "$INITRD" \
    -drive "file=${DISK},format=raw,if=ide,cache=writethrough" \
    -m "$RAM" \
    -cpu pentium3 \
    -display none \
    -serial mon:stdio \
    -no-reboot \
    -no-shutdown
