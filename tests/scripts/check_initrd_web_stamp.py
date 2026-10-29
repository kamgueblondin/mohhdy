#!/usr/bin/env python3
"""Exercise initrd invalidation with actual added, changed and removed HTML."""
import pathlib
import subprocess
import tarfile

ROOT = pathlib.Path(__file__).resolve().parents[2]
PAGE = ROOT / "initrd_content/web/lot59-stamp-check.html"


def pack(changed):
    result = subprocess.run(["make", "--no-print-directory", "pack-initrd"],
                            cwd=ROOT, check=True, capture_output=True, text=True)
    reused = "initrd unchanged" in result.stdout
    if reused == changed:
        raise RuntimeError("stale initrd reused" if changed else "unchanged initrd repacked")


def archived(expected):
    for name in ("my_initrd.tar", "my_initrd_test.tar"):
        with tarfile.open(ROOT / name) as archive:
            matches = [m for m in archive.getmembers() if m.name.endswith("web/" + PAGE.name)]
            if expected is None:
                if matches:
                    raise RuntimeError(name + " retains removed HTML")
            elif len(matches) != 1 or archive.extractfile(matches[0]).read() != expected:
                raise RuntimeError(name + " contains stale HTML")


def main():
    if PAGE.exists():
        raise RuntimeError("temporary fixture already exists")
    subprocess.run(["make", "--no-print-directory", "pack-initrd"], cwd=ROOT, check=True)
    pack(False)
    try:
        PAGE.write_bytes(b"<p>before</p>\n")
        pack(True)
        archived(b"<p>before</p>\n")
        PAGE.write_bytes(b"<p>after</p>\n")
        pack(True)
        archived(b"<p>after</p>\n")
        PAGE.unlink()
        pack(True)
        archived(None)
        pack(False)
    finally:
        if PAGE.exists():
            PAGE.unlink()
            subprocess.run(["make", "--no-print-directory", "pack-initrd"], cwd=ROOT, check=True)
    print("OK initrd web stamp: added/edited/removed HTML repacks both archives; unchanged reuses")


if __name__ == "__main__":
    main()
