#!/usr/bin/env python3
"""Synthetic GPT-2 GGUF v3 fixture for the aiworker GGUF contract.

CI has no GPT-2 GGUF weights. This writes a deterministic, structurally valid
GGUF v3 "gpt2" model that exercises the three K-quant kernels of
kernel/llm/gpt2_quant.c (Q3_K, Q4_K, Q6_K) on the same runtime as
gpt2-Q3_K_M:

  channels C = 768 (the smallest width that is both a multiple of the 12
  heads and of the 256-element K-quant block), 1 layer, vocabulary 16 (the
  tokenizer of tests/scripts/ai_worker_fixture.py), 32 positions.

  token_embd F32, position_embd F32, norms/biases F32,
  attn_qkv Q4_K, attn_output Q6_K, ffn_up Q3_K, ffn_down Q4_K, output Q6_K.

Block contents are seeded random bytes with small fp16 scales so the forward
pass stays finite. Usage: ai_gguf_fixture.py OUT_DIR -> OUT_DIR/gpt2.gguf and
OUT_DIR/gguf_fat16.img (FAT16 disk with GPT2.GGU, scripts/make_gguf_fat16_image.py).
"""
import os
import random
import struct
import subprocess
import sys

SEED = 20261004
C = 768
VOCAB = 16
POSITIONS = 32
ALIGN = 32
F32, Q3_K, Q4_K, Q6_K = 0, 11, 12, 14
BLOCK = {Q3_K: 110, Q4_K: 144, Q6_K: 210}


def gguf_string(text):
    raw = text.encode("ascii")
    return struct.pack("<Q", len(raw)) + raw


def f32_tensor(rng, count, center, spread):
    return struct.pack("<%df" % count, *[center + rng.uniform(-spread, spread) for _ in range(count)])


def half(value):
    return struct.pack("<e", value)


def quant_tensor(rng, kind, elements):
    blocks = elements // 256
    out = bytearray()
    for _ in range(blocks):
        if kind == Q4_K:
            out += half(rng.uniform(1.0e-4, 3.0e-4)) + half(rng.uniform(0.5e-4, 1.5e-4))
            out += bytes(rng.getrandbits(8) for _ in range(12 + 128))
        elif kind == Q6_K:
            out += bytes(rng.getrandbits(8) for _ in range(128 + 64))
            out += bytes((rng.randint(-32, 32) & 0xFF) for _ in range(16))
            out += half(rng.uniform(1.0e-4, 3.0e-4))
        else:  # Q3_K
            out += bytes(rng.getrandbits(8) for _ in range(32 + 64 + 12))
            out += half(rng.uniform(2.0e-4, 6.0e-4))
    assert len(out) == blocks * BLOCK[kind]
    return bytes(out)


def build(rng):
    tensors = []  # (name, dims, type, data)
    tensors.append(("token_embd.weight", [C, VOCAB], F32, f32_tensor(rng, C * VOCAB, 0.0, 0.2)))
    tensors.append(("position_embd.weight", [C, POSITIONS], F32, f32_tensor(rng, C * POSITIONS, 0.0, 0.05)))
    tensors.append(("output_norm.weight", [C], F32, f32_tensor(rng, C, 1.0, 0.1)))
    tensors.append(("output_norm.bias", [C], F32, f32_tensor(rng, C, 0.0, 0.05)))
    tensors.append(("output.weight", [C, VOCAB], Q6_K, quant_tensor(rng, Q6_K, C * VOCAB)))
    p = "blk.0."
    tensors.append((p + "attn_norm.weight", [C], F32, f32_tensor(rng, C, 1.0, 0.1)))
    tensors.append((p + "attn_norm.bias", [C], F32, f32_tensor(rng, C, 0.0, 0.05)))
    tensors.append((p + "attn_qkv.weight", [C, 3 * C], Q4_K, quant_tensor(rng, Q4_K, C * 3 * C)))
    tensors.append((p + "attn_qkv.bias", [3 * C], F32, f32_tensor(rng, 3 * C, 0.0, 0.02)))
    tensors.append((p + "attn_output.weight", [C, C], Q6_K, quant_tensor(rng, Q6_K, C * C)))
    tensors.append((p + "attn_output.bias", [C], F32, f32_tensor(rng, C, 0.0, 0.02)))
    tensors.append((p + "ffn_norm.weight", [C], F32, f32_tensor(rng, C, 1.0, 0.1)))
    tensors.append((p + "ffn_norm.bias", [C], F32, f32_tensor(rng, C, 0.0, 0.05)))
    tensors.append((p + "ffn_up.weight", [C, 4 * C], Q3_K, quant_tensor(rng, Q3_K, C * 4 * C)))
    tensors.append((p + "ffn_up.bias", [4 * C], F32, f32_tensor(rng, 4 * C, 0.0, 0.02)))
    tensors.append((p + "ffn_down.weight", [4 * C, C], Q4_K, quant_tensor(rng, Q4_K, 4 * C * C)))
    tensors.append((p + "ffn_down.bias", [C], F32, f32_tensor(rng, C, 0.0, 0.02)))
    return tensors


def write_gguf(path):
    rng = random.Random(SEED)
    tensors = build(rng)
    header = bytearray(b"GGUF" + struct.pack("<IQQ", 3, len(tensors), 2))
    header += gguf_string("general.architecture") + struct.pack("<I", 8) + gguf_string("gpt2")
    header += gguf_string("general.alignment") + struct.pack("<II", 4, ALIGN)
    offset = 0
    offsets = []
    for name, dims, kind, data in tensors:
        offsets.append(offset)
        header += gguf_string(name) + struct.pack("<I", len(dims))
        header += b"".join(struct.pack("<Q", d) for d in dims)
        header += struct.pack("<IQ", kind, offset)
        offset += len(data)
        offset = (offset + ALIGN - 1) // ALIGN * ALIGN
    header += b"\0" * ((-len(header)) % ALIGN)
    with open(path, "wb") as handle:
        handle.write(header)
        for (name, dims, kind, data), start in zip(tensors, offsets):
            handle.write(b"\0" * (len(header) + start - handle.tell()))
            handle.write(data)
    return os.path.getsize(path)


def main():
    if len(sys.argv) != 2:
        raise SystemExit("usage: ai_gguf_fixture.py OUT_DIR")
    out = sys.argv[1]
    os.makedirs(out, exist_ok=True)
    model = os.path.join(out, "gpt2.gguf")
    size = write_gguf(model)
    root = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", ".."))
    subprocess.run([sys.executable, os.path.join(root, "scripts", "make_gguf_fat16_image.py"),
                    "--model", model, "--image", os.path.join(out, "gguf_fat16.img"),
                    "--reserve-clusters", "4000"], check=True, stdout=subprocess.DEVNULL)
    print("ai gguf fixture: %d bytes, C=%d layers=1 vocab=%d positions=%d in %s" %
          (size, C, VOCAB, POSITIONS, out))


if __name__ == "__main__":
    main()
