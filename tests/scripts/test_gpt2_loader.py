#!/usr/bin/env python3
"""Validate GPT-2 checkpoint discovery in the MOHHDY initrd and boot log.

This test uses a structural checkpoint with the official llm.c v3 header and
zero-valued weights. It verifies loader safety and boot integration only; it
does not claim model-quality inference.
"""
import os
import shutil
import struct
import subprocess
import sys
import time

ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", ".."))
MODELS = os.path.join(ROOT, "models")
CHECKPOINT = os.path.join(MODELS, "gpt2_124M.bin")
TOKENIZER = os.path.join(MODELS, "gpt2_tokenizer.bin")
LOG = os.path.join(ROOT, "test_logs", "gpt2-loader-smoke.log")
ERR = os.path.join(ROOT, "test_logs", "gpt2-loader-smoke.err")

MAGIC = 20240326
VERSION = 3
# Small GPT-2-shaped checkpoint for safe structural validation: V=8, T=4,
# L=1, heads=1, channels=4, padded vocabulary=8.
CONFIG = (4, 8, 1, 1, 4, 8)


def parameter_count(max_t, vocab, layers, heads, channels, padded_vocab):
    del vocab, heads
    c = channels
    return (
        padded_vocab * c + max_t * c + layers * c + layers * c +
        layers * (3 * c) * c + layers * (3 * c) +
        layers * c * c + layers * c + layers * c + layers * c +
        layers * (4 * c) * c + layers * (4 * c) +
        layers * c * (4 * c) + layers * c + c + c
    )


def write_checkpoint():
    os.makedirs(MODELS, exist_ok=True)
    header = [0] * 256
    header[0] = MAGIC
    header[1] = VERSION
    for index, value in enumerate(CONFIG, start=2):
        header[index] = value
    count = parameter_count(*CONFIG)
    with open(CHECKPOINT, "wb") as handle:
        handle.write(struct.pack("<256I", *header))
        handle.write(b"\x00" * (count * 4))


def write_tokenizer():
    header = [0] * 256
    header[0] = 20240328
    header[1] = 2
    header[2] = 8
    header[3] = 7
    with open(TOKENIZER, "wb") as handle:
        handle.write(struct.pack("<256I", *header))
        for piece in (b"a", b"b", b"c", b"d", b"e", b"f", b"g", b" "):
            handle.write(bytes([len(piece)]))
            handle.write(piece)


def wait_for_log(proc, needle, timeout=15):
    deadline = time.time() + timeout
    while time.time() < deadline:
        if proc.poll() is not None:
            raise RuntimeError("QEMU stopped before GPT-2 loader status")
        try:
            with open(LOG, "r", errors="replace") as handle:
                if needle in handle.read():
                    return
        except OSError:
            pass
        time.sleep(0.1)
    raise RuntimeError("boot log missing: %s" % needle)


def main():
    previous = None
    tokenizer_previous = None
    if os.path.exists(CHECKPOINT):
        previous = CHECKPOINT + ".saved-for-test"
        shutil.move(CHECKPOINT, previous)
    if os.path.exists(TOKENIZER):
        tokenizer_previous = TOKENIZER + ".saved-for-test"
        shutil.move(TOKENIZER, tokenizer_previous)
    try:
        write_checkpoint()
        write_tokenizer()
        subprocess.run(["make", "all"], cwd=ROOT, check=True)
        for path in (LOG, ERR):
            try:
                os.remove(path)
            except OSError:
                pass
        command = [
            "qemu-system-i386", "-kernel", "build/mohhdy.bin", "-initrd", "my_initrd.tar",
            "-m", "128M", "-display", "none", "-vga", "none",
            "-serial", "file:" + LOG, "-monitor", "none", "-no-reboot", "-no-shutdown",
        ]
        with open(ERR, "wb") as err_handle:
            proc = subprocess.Popen(command, cwd=ROOT, stdout=err_handle, stderr=err_handle)
            try:
                wait_for_log(proc, "Modele GPT-2 local charge depuis l'initrd.")
                wait_for_log(proc, "Tokenizer GPT-2 local charge depuis l'initrd.")
                wait_for_log(proc, "GPT-2: prochain jeton genere localement")
            finally:
                if proc.poll() is None:
                    proc.terminate()
                    try:
                        proc.wait(timeout=2)
                    except subprocess.TimeoutExpired:
                        proc.kill()
        print("GPT-2 checkpoint loader smoke passed.")
        return 0
    finally:
        for path in (CHECKPOINT, TOKENIZER):
            try:
                os.remove(path)
            except OSError:
                pass
        if previous is not None:
            shutil.move(previous, CHECKPOINT)
        if tokenizer_previous is not None:
            shutil.move(tokenizer_previous, TOKENIZER)
        subprocess.run(["make", "all"], cwd=ROOT, check=True)


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except Exception as error:
        print("GPT-2 checkpoint loader smoke failed: %s" % error, file=sys.stderr)
        raise SystemExit(1)
