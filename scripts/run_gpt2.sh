#!/usr/bin/env bash
# One command: fetch the GPT-2 assets if missing, build the image and the
# GGUF FAT16 disk, then boot Mohhdy in a QEMU GTK window.
#   GPT2_PROFILE=gguf (default): tokenizer in the initrd, GPT2.GGU on disk.
#   GPT2_PROFILE=fp32: also the 475 MiB FP32 checkpoint in the initrd.
#   QEMU_ACCEL=kvm|tcg forces the accelerator (default: kvm if /dev/kvm
#   is readable and writable, else tcg).
#   GPT2_DISPLAY=gtk|curses|none (none: serial on stdio).
set -euo pipefail
cd "$(dirname "$0")/.."

RELEASE_URL="${GPT2_RELEASE_URL:-https://github.com/kamgueblondin/mohhdy/releases/download/gpt2-124m-assets}"
PROFILE="${GPT2_PROFILE:-gguf}"
DISPLAY_MODE="${GPT2_DISPLAY:-gtk}"
MODELS=models

fetch() {
    if [ ! -s "$MODELS/$1" ]; then
        echo "[run-gpt2] download $1"
        curl -fL --retry 3 -o "$MODELS/$1.part" "$RELEASE_URL/$1"
        mv "$MODELS/$1.part" "$MODELS/$1"
    fi
}

mkdir -p "$MODELS"
fetch gpt2-124m-assets.sha256
fetch gpt2_tokenizer.bin
fetch gpt2-Q3_K_M.gguf
case "$PROFILE" in
    gguf) ;;
    fp32) fetch gpt2_124M.bin ;;
    *) echo "[run-gpt2] GPT2_PROFILE must be gguf or fp32" >&2; exit 2 ;;
esac
(cd "$MODELS" && sha256sum -c --ignore-missing --quiet gpt2-124m-assets.sha256)
echo "[run-gpt2] checksums ok"

MODEL_DIR="$MODELS"
if [ "$PROFILE" = gguf ] && [ -f "$MODELS/gpt2_124M.bin" ]; then
    # Keep the GGUF profile small: tokenizer only in the initrd.
    MODEL_DIR=build/run_gpt2_models
    mkdir -p "$MODEL_DIR"
    cp -f "$MODELS/gpt2_tokenizer.bin" "$MODEL_DIR/"
fi
make -s all MODEL_DIR="$MODEL_DIR"
if [ ! -f build/gpt2_gguf_fat16.img ] || [ "$MODELS/gpt2-Q3_K_M.gguf" -nt build/gpt2_gguf_fat16.img ]; then
    make -s gguf-disk
fi

ACCEL="${QEMU_ACCEL:-}"
if [ -z "$ACCEL" ]; then
    if [ -r /dev/kvm ] && [ -w /dev/kvm ]; then
        ACCEL=kvm
    else
        ACCEL=tcg
        echo "[run-gpt2] /dev/kvm not usable: TCG emulation, about 70 s per sentence."
        echo "[run-gpt2] Linux: sudo usermod -aG kvm \$USER, then log in again."
    fi
fi

case "$DISPLAY_MODE" in
    gtk) DISP=(-vga std -usb -device usb-tablet
               -display gtk,zoom-to-fit=on,show-menubar=off,grab-on-hover=off,show-cursor=on
               -serial file:test_logs/run-gpt2-serial.log) ;;
    curses) DISP=(-display curses -serial file:test_logs/run-gpt2-serial.log) ;;
    none) DISP=(-display none -vga none -serial stdio) ;;
    *) echo "[run-gpt2] GPT2_DISPLAY must be gtk, curses or none" >&2; exit 2 ;;
esac
mkdir -p test_logs
echo "[run-gpt2] accel=$ACCEL profile=$PROFILE display=$DISPLAY_MODE"
echo "[run-gpt2] In the guest: ai-model use gpt2.gguf, then ai <english text>, then ai-continue"
exec qemu-system-i386 -name "Mohhdy OS GPT-2" -cpu pentium3 \
    -machine "type=pc,accel=$ACCEL" \
    -kernel build/mohhdy.bin -initrd my_initrd.tar -m 1024M \
    "${DISP[@]}" -no-reboot -no-shutdown \
    -drive file=build/gpt2_gguf_fat16.img,format=raw,if=ide,cache=writethrough
