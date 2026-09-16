#!/bin/sh
# Boot Multiboot Mohhdy OS under QEMU with a graphical display.
# Canonical guest command after MOHHDY> : gui  (aliases: graphics, desktop).
# Leave with: console  (or ESC).
# Produit visuel : framebuffer VBE dans la fenetre QEMU (zoom-to-fit, pas HTML).
# Default Docker image stays nographic for CI.
set -eu

KERNEL="${MOHHDY_KERNEL:-/os/mohhdy.bin}"
INITRD="${MOHHDY_INITRD:-/os/my_initrd.tar}"
DISK="${MOHHDY_DISK:-/os/overlay.img}"
RAM="${MOHHDY_RAM:-256M}"
DISPLAY_KIND="${MOHHDY_QEMU_DISPLAY:-gtk,zoom-to-fit=on,show-menubar=off}"

FIT="${MOHHDY_FIT_SCRIPT:-}"
if [ -z "$FIT" ] && [ -f /os/qemu_gui_fit.py ]; then FIT=/os/qemu_gui_fit.py; fi
if [ -z "$FIT" ] && [ -f "$(dirname "$0")/../scripts/qemu_gui_fit.py" ]; then
    FIT="$(dirname "$0")/../scripts/qemu_gui_fit.py"
fi
if [ -n "$FIT" ] && [ -f "$FIT" ] && command -v python3 >/dev/null 2>&1; then
    export MOHHDY_KERNEL="$KERNEL"
    export MOHHDY_INITRD="$INITRD"
    export DISK_IMAGE="$DISK"
    export GPT2_RAM="$RAM"
    exec python3 "$FIT"
fi

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
