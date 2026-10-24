#!/bin/sh
# pack-initrd must repack when the model profile changes. The old stamp
# only compared mtimes, so MODEL_DIR=<dir with older or no models> kept
# the previous image. Restores the default profile when done.
set -e
cd "$(dirname "$0")/../.."
empty=$(mktemp -d)
trap 'rmdir "$empty" 2>/dev/null || true' EXIT
fail() { echo "FAIL initrd model stamp: $1"; exit 1; }

make --no-print-directory pack-initrd >/dev/null
out=$(make --no-print-directory pack-initrd)
echo "$out" | grep -q "initrd unchanged" || fail "second pack with the same profile repacked"

out=$(make --no-print-directory pack-initrd MODEL_DIR="$empty")
echo "$out" | grep -q "initrd unchanged" && fail "MODEL_DIR change kept the stale image"
tar tf my_initrd.tar | grep -q "models/gpt2_124M.bin" && fail "stale FP32 checkpoint left in the image"

out=$(make --no-print-directory pack-initrd)
echo "$out" | grep -q "initrd unchanged" && fail "back to the default profile kept the empty image"
if [ -f models/gpt2_124M.bin ]; then
    tar tf my_initrd.tar | grep -q "models/gpt2_124M.bin" || fail "default profile lost the FP32 checkpoint"
fi
echo "OK initrd model stamp: profile change repacks, same profile is reused"
