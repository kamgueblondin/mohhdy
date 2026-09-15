#!/usr/bin/env python3
"""Minimal GGUF v3 inspector used only to design the freestanding AOS-020 loader."""
import struct
import sys
from collections import Counter

TYPE_NAMES = {
    0: "UINT8", 1: "INT8", 2: "UINT16", 3: "INT16", 4: "UINT32", 5: "INT32",
    6: "FLOAT32", 7: "BOOL", 8: "STRING", 9: "ARRAY", 10: "UINT64", 11: "INT64", 12: "FLOAT64",
}
TENSOR_TYPES = {
    0: "F32", 1: "F16", 2: "Q4_0", 3: "Q4_1", 6: "Q5_0", 7: "Q5_1", 8: "Q8_0",
    10: "Q2_K", 11: "Q3_K", 12: "Q4_K", 13: "Q5_K", 14: "Q6_K", 15: "Q8_K",
}


def read_exact(handle, count):
    data = handle.read(count)
    if len(data) != count:
        raise ValueError("truncated GGUF")
    return data


def u32(handle):
    return struct.unpack("<I", read_exact(handle, 4))[0]


def u64(handle):
    return struct.unpack("<Q", read_exact(handle, 8))[0]


def ggstr(handle):
    return read_exact(handle, u64(handle)).decode("utf-8", "replace")


def skip_value(handle, value_type):
    sizes = {0: 1, 1: 1, 2: 2, 3: 2, 4: 4, 5: 4, 6: 4, 7: 1, 10: 8, 11: 8, 12: 8}
    if value_type == 8:
        return ggstr(handle)
    if value_type == 9:
        element_type = u32(handle)
        count = u64(handle)
        for _ in range(count):
            skip_value(handle, element_type)
        return None
    if value_type not in sizes:
        raise ValueError("unsupported metadata type %d" % value_type)
    return read_exact(handle, sizes[value_type])


def main(path):
    with open(path, "rb") as handle:
        magic = read_exact(handle, 4)
        version = u32(handle)
        tensors = u64(handle)
        metadata_count = u64(handle)
        print("magic=%s version=%d tensors=%d metadata=%d" % (magic.decode("ascii", "replace"), version, tensors, metadata_count))
        metadata = {}
        for _ in range(metadata_count):
            key = ggstr(handle)
            kind = u32(handle)
            value = skip_value(handle, kind)
            if kind == 8:
                metadata[key] = value
        print("architecture=%s" % metadata.get("general.architecture"))
        print("alignment=%s" % metadata.get("general.alignment", "32(default)"))
        types = Counter()
        names = []
        for _ in range(tensors):
            name = ggstr(handle)
            dims = u32(handle)
            shape = tuple(u64(handle) for _ in range(dims))
            tensor_type = u32(handle)
            offset = u64(handle)
            types[TENSOR_TYPES.get(tensor_type, str(tensor_type))] += 1
            names.append((name, shape, TENSOR_TYPES.get(tensor_type, str(tensor_type)), offset))
        print("tensor_types=%s" % dict(types))
        for name, shape, tensor_type, offset in names[:20]:
            print("tensor name=%s shape=%s type=%s offset=%d" % (name, shape, tensor_type, offset))


if __name__ == "__main__":
    if len(sys.argv) != 2:
        raise SystemExit("usage: inspect_gguf.py model.gguf")
    main(sys.argv[1])
