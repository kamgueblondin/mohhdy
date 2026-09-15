#!/usr/bin/env python3
"""Contrat QEMU : deux paires NE2000/TLS locales, isolées et séquentielles.

Chaque cycle lance un invité QEMU doté d'une MAC distincte et un pair Ethernet
TLS contrôlé, tous deux reliés uniquement par un socket vers 127.0.0.1. Les
cycles restent séquentiels : la machine de test ne masque donc pas la perte
d'IRQ PS/2 observée avec deux QEMU TCG injectés simultanément.
"""
import os
import subprocess
import sys


ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", ".."))
SCRIPT = os.path.join(ROOT, "tests", "scripts", "test_qemu_ne2k_tls_http.py")
LOG_DIR = os.path.join(ROOT, "test_logs")
PAIRS = (
    ("a", "52:54:00:a0:20:0a"),
    ("b", "52:54:00:a0:20:0b"),
)


def run_pair(label, mac):
    environment = os.environ.copy()
    environment["MOHHDY_NE2K_RUN_LABEL"] = "multipair-" + label
    environment["MOHHDY_NE2K_GUEST_MAC"] = mac
    result = subprocess.run([sys.executable, SCRIPT], cwd=ROOT, env=environment)
    if result.returncode:
        raise RuntimeError("paire locale %s en echec (%d)" % (label, result.returncode))
    log = os.path.join(LOG_DIR, "ne2k-tls-http-multipair-%s.log" % label)
    try:
        with open(log, errors="replace") as handle:
            content = handle.read()
    except OSError as error:
        raise RuntimeError("journal de paire %s absent: %s" % (label, error))
    if "TLS_COMPLETE" not in content:
        raise RuntimeError("TLS_COMPLETE absent pour la paire %s" % label)
    if "Secrets OpenAI     : jamais integres a l'image de boot" not in content:
        raise RuntimeError("diagnostic d'absence de secret absent pour la paire %s" % label)


def main():
    os.makedirs(LOG_DIR, exist_ok=True)
    for label, mac in PAIRS:
        run_pair(label, mac)
    print("QEMU NE2000 TLS multi-pairs local sequential contract passed.")
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except Exception as error:
        print("QEMU NE2000 TLS multi-pairs local sequential contract failed: %s" % error)
        raise SystemExit(1)
