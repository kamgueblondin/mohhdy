# Diagnostic QEMU OS-UI Input & GUI Behavior

## Overview
This document summarizes findings regarding host and guest mouse input, GTK grab behavior, and UHCI USB tablet enumeration in Mohhdy OS.

## 1. Test Harness & Build Setup
- The local unit test framework harness (`tests/framework/unity.c`) required `#include <stdio.h>` to declare `putchar` properly during 32-bit compilation (`gcc -m32`).
- 32-bit multilib libraries (`gcc-multilib`, `libc6-dev-i386`) are required for host execution of kernel unit tests (`make -C tests`).

## 2. QEMU GUI Flags & UHCI USB Tablet
- When QEMU is launched without `-usb -device usb-tablet`, the guest kernel reports:
  `USB Tablet: Controller UHCI non trouve`
- When QEMU is launched with `-usb -device usb-tablet` (e.g. `test_qemu_osui_gui.py`, `qemu_gui_fit.py`, `test_qemu_gui_fit.py`), UHCI controller initialization and USB enumeration complete successfully, reporting:
  `USB Tablet: Controller UHCI initialise et enumere`
- Implementation details & measured reality for guest UHCI USB tablet enumeration:
  - TD status active bit is bit 23 (`0x00800000`). The `uhci_td_t.status` field is qualified `volatile` to prevent GCC `-O3` from caching TD status in CPU registers during control transfer polling loops.
  - Device speed is detected dynamically via UHCI PORTSC bit 8 (`0x0100`). Full-speed USB devices (like QEMU `usb-tablet` on port 1 with `PORTSC = 0x0087`) set `ls_bit = 0`.
  - Port reset requires a ~10ms reset pulse (`0x0200`) followed by a ~10ms reset-recovery delay before executing SET_ADDRESS and SET_CONFIGURATION control transfers.
  - On failure, formatted ASCII debug logs record TD status and error codes.
  - Smoke tests (`tests/scripts/test_qemu_osui_gui.py`) explicitly require the success line and fail immediately if failure strings (e.g. `Enumeration non terminee`) appear in serial logs.
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

## 5. Guest dock hit-test click smoke (monitor)
- After `gui` and the UHCI tablet success line, `tests/scripts/test_qemu_osui_gui.py` drives QMP `input-send-event` absolute tablet axes plus left button onto dock icon index 2 (`/shell`). HMP `mouse_move`/`mouse_button` alone is unreliable after `screendump` in this harness.
- Guest `gfx_desktop_handle_click` injects the slash command into the keyboard buffer; the smoke asserts serial `osui gui line=/shell` and `live_eval=true`.
- This proves guest hit-test wiring without a host GTK grab. Interactive `make run-gui` may still require `Ctrl+Alt+G` to release the host pointer grab after clicking inside the GTK window.
- Keyboard OS-UI path (`/browser`, `/shell`, `whoami`, `ai hello`, `/center`, `console`) remains covered in the same script after the click smoke.
