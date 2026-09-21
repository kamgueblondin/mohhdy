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

## 5. Guest hit-test click smoke (monitor)
- After `gui` and the UHCI tablet success line, `tests/scripts/test_qemu_osui_gui.py` drives QMP `input-send-event` absolute tablet axes plus left button onto guest hit regions. HMP `mouse_move`/`mouse_button` alone is unreliable after `screendump` in this harness.
- Covered regions (OS-UI-G-1): stage badge (`/stage`), top menu Browser-OS (`/browser`), pane traffic close (`/center`), chat send (empty enter), dock icon index 2 (`/shell` + `live_eval=true`).
- Guest `gfx_desktop_handle_click` injects the slash command (or `\n` for send) into the keyboard buffer; serial asserts use `osui gui line=...` plus the matching runtime marker.
- Unit coverage: `tests/unit/kernel/test_gfx_desktop.c` clicks dock/menu/stage/send/close layout centers and checks the keyboard buffer.
- This proves guest hit-test wiring without a host GTK grab. Interactive `make run-gui` may still require `Ctrl+Alt+G` to release the host pointer grab after clicking inside the GTK window.
- Keyboard OS-UI path (`/browser`, `/shell`, `whoami`, `ai hello`, `/center`, `console`) remains covered in the same script after the click smoke.

## 6. Resize-safe tablet input (OS-UI-G-2 / AOS-003)
- `make qemu-osui-gui-fit` still proves VBE follows the GTK window via COM2 WxH (grow then shrink).
- `make qemu-osui-gui-fit-input` extends that path: after `gui` and UHCI tablet enum, QMP absolute tablet clicks hit dock `/shell` at the boot size, again after grow, and again after shrink. Serial must show `osui gui line=/shell` and `live_eval=true` after each click.
- Guest mapping uses current `gfx_fb_width`/`gfx_fb_height` in `usb_tablet_poll`. After a COM2-driven VBE resize, `gfx_desktop_clamp_mouse` keeps stored pointer coords inside the new framebuffer so hit-test and cursor stay consistent.
- Existing `make qemu-osui-gui` hit-test smoke (OS-UI-G-1) is unchanged and does not resize the window.
- Grow/shrink window sizes default to 1200x750 and 800x600 (override with OSUI_FIT_GROW_W/H and OSUI_FIT_SHRINK_W/H) so the smoke fits common 1280x800 displays.

## 7. PS/2 relative fallback (OS-UI-G-3 / AOS-003)
- `make qemu-osui-gui-ps2` launches QEMU **without** `-usb -device usb-tablet`. Serial must show `USB Tablet: Controller UHCI non trouve` (no absolute tablet path).
- After `gui`, the harness drives QMP `input-send-event` **relative** axes (`type=rel`) plus left button. Guest `mouse_handle_byte` feeds `gfx_desktop_move_mouse` (x2 gain when `!usb_tablet_present()`).
- IRQ12 stays masked on this PIC map; `ps2_mouse_poll` drains i8042 aux bytes from the 100 Hz timer while the desktop is active (parallel to `usb_tablet_poll`).
- Edge clamping: large relative floods past the framebuffer; `clamp_mouse_to_fb` / `gfx_desktop_clamp_mouse` keep coords in `[0..w-1] x [0..h-1]` (negative and positive). Unit: `test_mouse_clamped_to_fb`.
- Smoke then relative-moves from the clamped corner onto dock `/shell` (`osui gui line=/shell`, `live_eval=true`) and returns via `console`.
- `make qemu-osui-gui` (with tablet) remains the absolute-path contract and must stay green.

