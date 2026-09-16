#!/bin/sh
# Boot Multiboot Mohhdy OS under QEMU with a graphical display.
# Canonical guest command after MOHHDY> : gui  (aliases: graphics, desktop).
# Leave with: console  (or ESC).
# Produit visuel : framebuffer VBE 1024x768 dans la fenetre QEMU (pas HTML).
# Default Docker image stays nographic for CI.
set -eu

KERNEL="${MOHHDY_KERNEL:-/os/mohhdy.bin}"
INITRD="${MOHHDY_INITRD:-/os/my_initrd.tar}"
DISK="${MOHHDY_DISK:-/os/overlay.img}"
RAM="${MOHHDY_RAM:-256M}"
DISPLAY_KIND="${MOHHDY_QEMU_DISPLAY:-gtk}"

exec qemu-system-i386 \
    -kernel "$KERNEL" \
    -initrd "$INITRD" \
    -drive "file=${DISK},format=raw,if=ide,cache=writethrough" \
    -m "$RAM" \
    -cpu pentium3 \
    -vga std \
    -display "$DISPLAY_KIND" \
    -serial mon:stdio \
    -no-reboot \
    -no-shutdown
