#!/usr/bin/env python3
"""Contrat léger de synthèse pour le benchmark GGUF QEMU."""
import importlib.util
import os

ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", ".."))
TARGET = os.path.join(ROOT, "tests", "scripts", "benchmark_qemu_gguf_latency.py")
spec = importlib.util.spec_from_file_location("gguf_latency_benchmark", TARGET)
module = importlib.util.module_from_spec(spec)
spec.loader.exec_module(module)


def main():
    summary = module.summarize([10.0, 20.0, 40.0])
    if summary["min_seconds"] != 10.0:
        raise RuntimeError("minimum GGUF incorrect")
    if summary["median_seconds"] != 20.0:
        raise RuntimeError("mediane GGUF incorrecte")
    if summary["max_seconds"] != 40.0:
        raise RuntimeError("maximum GGUF incorrect")
    if summary["spread_seconds"] != 30.0:
        raise RuntimeError("dispersion GGUF incorrecte")
    if summary["spread_ratio"] != 1.5:
        raise RuntimeError("ratio de dispersion GGUF incorrect")
    try:
        module.summarize([])
    except ValueError:
        pass
    else:
        raise RuntimeError("echantillon vide accepte")
    print("GGUF benchmark protocol check passed")


if __name__ == "__main__":
    main()
