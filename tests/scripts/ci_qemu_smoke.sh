#!/bin/bash
# ci_qemu_smoke.sh - Boot QEMU headless, type ls/cat/ps/uptime, require serial markers.
# Keyboard is PS/2: HMP sendkey injects scancodes (host TTY would not reach SYS_GETS).
# Hard timeout: QEMU stderr must not be a PIPE (deadlock on GitHub Actions).
# Core, shell extras et syscalls sont isolés par boot afin que les frappes ne se propagent pas d’un scénario à l’autre.
# Overlay disk is zeroed before core, extras, syscalls and spawn; persist uses its own image.

set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
cd "$ROOT"

KERNEL="${KERNEL:-build/mohhdy.bin}"
INITRD="${INITRD:-my_initrd.tar}"
CORE_TIMEOUT="${CORE_TIMEOUT:-180}"
EXTRAS_TIMEOUT="${EXTRAS_TIMEOUT:-150}"
PERSIST_TIMEOUT="${PERSIST_TIMEOUT:-180}"
SPAWN_TIMEOUT="${SPAWN_TIMEOUT:-240}"
EXEC_TIMEOUT="${EXEC_TIMEOUT:-90}"
SYSCALL_TIMEOUT="${SYSCALL_TIMEOUT:-420}"
OVERLAY_DISK="${OVERLAY_DISK:-$ROOT/build/overlay.img}"
PERSIST_DISK="${PERSIST_DISK:-$ROOT/build/overlay-persist.img}"
export OVERLAY_DISK PERSIST_DISK

reset_overlay_disk() {
    local img="$1"
    mkdir -p "$(dirname "$img")"
    dd if=/dev/zero of="$img" bs=512 count=64 conv=notrunc status=none
}

if [ ! -f "$KERNEL" ] || [ ! -f "$INITRD" ]; then
    echo "ERROR: missing $KERNEL or $INITRD - run 'make all' first"
    exit 1
fi

export KERNEL INITRD
export LOG="${LOG:-test_logs/ci-qemu-serial.log}"
export PYTHONUNBUFFERED=1

if ! grep -a -qF "Initrd / VFS" userspace/shell; then
    echo "ERROR: userspace/shell is stale (no syscall ls). Expected 'make -C userspace all'."
    file userspace/shell || true
    exit 1
fi
if ! grep -a -qF "yield ok" userspace/shell; then
    echo "ERROR: userspace/shell is stale (no yield ok needle)."
    exit 1
fi
if ! grep -a -qF "exec ok" userspace/ok; then
    echo "ERROR: userspace/ok is stale (no exec ok needle). Expected 'make -C userspace all'."
    exit 1
fi

run_python() {
    local name="$1"
    local timeout_s="$2"
    local script="$3"
    echo "=== QEMU ${name} (timeout ${timeout_s}s) ==="
    set +e
    timeout --foreground --signal=KILL "${timeout_s}s" python3 "$script"
    local rc=$?
    set -e
    if [ "$rc" -eq 124 ] || [ "$rc" -eq 137 ]; then
        echo "FAIL: qemu-smoke ${name} killed after ${timeout_s}s"
        return 1
    fi
    return "$rc"
}

reset_overlay_disk "$OVERLAY_DISK"
run_python core "$CORE_TIMEOUT" "$ROOT/tests/scripts/ci_qemu_core_smoke.py"
reset_overlay_disk "$OVERLAY_DISK"
run_python extras "$EXTRAS_TIMEOUT" "$ROOT/tests/scripts/ci_qemu_shell_extras.py"
run_python persist "$PERSIST_TIMEOUT" "$ROOT/tests/scripts/ci_qemu_persist.py"
reset_overlay_disk "$OVERLAY_DISK"
run_python spawn "$SPAWN_TIMEOUT" "$ROOT/tests/scripts/ci_qemu_spawn.py"
reset_overlay_disk "$OVERLAY_DISK"
run_python syscall "$SYSCALL_TIMEOUT" "$ROOT/tests/scripts/ci_qemu_syscalls.py"
reset_overlay_disk "$OVERLAY_DISK"
run_python exec "$EXEC_TIMEOUT" "$ROOT/tests/scripts/ci_qemu_exec.py"
