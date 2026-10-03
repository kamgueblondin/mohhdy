#!/usr/bin/env python3
"""Synthetic llm.c checkpoint + tokenizer for the aiworker QEMU contract.

CI ships no GPT-2 weights, so this writes a tiny GPT-2-shaped model
(T=32, V=16, L=2, heads=2, C=32) with seeded pseudo-random weights in the
official llm.c v3 checkpoint format (magic 20240326) and a 16-piece
tokenizer (magic 20240328, version 2). It runs through the exact same
loader, BPE, forward pass and sampler as GPT-2 124M; only the sizes differ.
Non-zero weights make the generated token ids depend on every layer, so
equality between the Ring 0 and Ring 3 paths is meaningful. It does not
claim model quality.

usage: ai_worker_fixture.py OUT_DIR
"""
import os
import random
import struct
import sys

MAGIC = 20240326
VERSION = 3
TOKENIZER_MAGIC = 20240328
# max_seq_len, vocab, layers, heads, channels, padded_vocab
CONFIG = (32, 16, 2, 2, 32, 16)
PIECES = [bytes([c]) for c in b"abcdefghijklmn"] + [b" ", b"<|endoftext|>"]
EOT = 15
SEED = 20261003


def tensor_sizes(max_t, vocab, layers, heads, channels, padded_vocab):
    del vocab, heads
    c = channels
    return [
        ("wte", padded_vocab * c), ("wpe", max_t * c),
        ("ln1w", layers * c), ("ln1b", layers * c),
        ("qkvw", layers * 3 * c * c), ("qkvb", layers * 3 * c),
        ("attprojw", layers * c * c), ("attprojb", layers * c),
        ("ln2w", layers * c), ("ln2b", layers * c),
        ("fcw", layers * 4 * c * c), ("fcb", layers * 4 * c),
        ("fcprojw", layers * c * 4 * c), ("fcprojb", layers * c),
        ("lnfw", c), ("lnfb", c),
    ]


def write_checkpoint(path):
    rng = random.Random(SEED)
    header = [0] * 256
    header[0] = MAGIC
    header[1] = VERSION
    for index, value in enumerate(CONFIG, start=2):
        header[index] = value
    values = []
    for name, count in tensor_sizes(*CONFIG):
        for _ in range(count):
            if name in ("ln1w", "ln2w", "lnfw"):
                values.append(1.0 + rng.uniform(-0.1, 0.1))
            elif name.endswith("b"):
                values.append(rng.uniform(-0.05, 0.05))
            else:
                values.append(rng.uniform(-0.5, 0.5))
    with open(path, "wb") as handle:
        handle.write(struct.pack("<256I", *header))
        handle.write(struct.pack("<%df" % len(values), *values))
    return 1024 + 4 * len(values)


def write_tokenizer(path):
    header = [0] * 256
    header[0] = TOKENIZER_MAGIC
    header[1] = 2
    header[2] = len(PIECES)
    header[3] = EOT
    with open(path, "wb") as handle:
        handle.write(struct.pack("<256I", *header))
        for piece in PIECES:
            handle.write(bytes([len(piece)]))
            handle.write(piece)


def main(argv):
    if len(argv) != 2:
        print(__doc__)
        return 2
    out = argv[1]
    os.makedirs(out, exist_ok=True)
    size = write_checkpoint(os.path.join(out, "gpt2_124M.bin"))
    write_tokenizer(os.path.join(out, "gpt2_tokenizer.bin"))
    print("ai fixture: checkpoint %d bytes, tokenizer %d pieces in %s" % (size, len(PIECES), out))
    return 0


if __name__ == "__main__":
    raise SystemExit(main(sys.argv))
