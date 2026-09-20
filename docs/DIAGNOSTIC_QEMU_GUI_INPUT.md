# Diagnostic QEMU OS-UI Input & GUI Behavior

## Overview
This document summarizes findings regarding host and guest mouse input, GTK grab behavior, and UHCI USB tablet enumeration in Mohhdy OS.

## 1. Test Harness & Build Setup
- The local unit test framework harness (`tests/framework/unity.c`) required `#include <stdio.h>` to declare `putchar` properly during 32-bit compilation (`gcc -m32`).
- 32-bit multilib libraries (`gcc-multilib`, `libc6-dev-i386`) are required for host execution of kernel unit tests (`make -C tests`).

## 2. QEMU GUI Flags & UHCI USB Tablet
- When QEMU is launched without `-usb -device usb-tablet` (e.g. headless smoke test `test_qemu_osui_gui.py`), the guest kernel reports:
  `USB Tablet: Controller UHCI non trouve`
- When QEMU is launched with `-usb -device usb-tablet` (e.g. `test_qemu_osui_gui.py`, `qemu_gui_fit.py`, `test_qemu_gui_fit.py`), UHCI initializes and reports:
  `USB Tablet: Controller UHCI initialise et enumere`
- On USB tablet presence, absolute coordinates are mapped directly to screen dimensions without mouse drift. On fallback PS/2 relative mouse mode, relative deltas x2 multiplier is applied.

## 3. Host GTK Grab Behavior
- Under QEMU GTK display mode, `grab-on-hover=off` prevents automatic cursor capture when hovering over the window.
- However, clicking inside the GTK window triggers host grab ("Press Ctrl+Alt+G to release grab").
- Adding `show-cursor=on` to GTK display options (`-display gtk,zoom-to-fit=on,show-menubar=off,grab-on-hover=off,show-cursor=on`) improves host/guest pointer visibility awareness. Use `Ctrl+Alt+G` to ungrab mouse from the host.

## 4. Verification Commands
- Kernel unit tests: `make -C tests test-kernel`
- Complete test suite: `make test-all`
- Headless QEMU OS-UI GUI test: `python3 tests/scripts/test_qemu_osui_gui.py`
- GTK interactive launcher: `make run-gui`
