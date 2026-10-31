#!/bin/sh
# Lot 67: regenerates initrd_content/etc/ca-roots.pem from the host Mozilla
# bundle (ca-certificates). Public certificates only.
set -e
out=initrd_content/etc/ca-roots.pem
mkdir -p initrd_content/etc
{
  echo "# MOHHDY Lot 67 root CA store (public certificates copied from the Mozilla"
  echo "# bundle shipped by ca-certificates). Regenerate: scripts/lot67_ca_roots.sh"
  while read -r f; do
    [ -z "$f" ] && continue
    p=/etc/ssl/certs/$f.pem
    [ -f "$p" ] || { echo "missing $f" >&2; continue; }
    echo "# $f"; cat "$p"
  done < scripts/lot67_ca_roots.list
} > "$out"
grep -c BEGIN "$out"
