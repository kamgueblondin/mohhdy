#!/usr/bin/env python3
"""Contrat leger : synthese + probe KVM + argv Multiboot accel=kvm."""
import importlib.util
import os
import tempfile

ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", ".."))
TARGET = os.path.join(ROOT, "tests", "scripts", "benchmark_qemu_gguf_kvm_latency.py")
spec = importlib.util.spec_from_file_location("gguf_kvm_latency_benchmark", TARGET)
module = importlib.util.module_from_spec(spec)
spec.loader.exec_module(module)


def main():
    summary = module.summarize([10.0, 20.0, 40.0])
    if summary["min_seconds"] != 10.0:
        raise RuntimeError("minimum GGUF KVM incorrect")
    if summary["median_seconds"] != 20.0:
        raise RuntimeError("mediane GGUF KVM incorrecte")
    if summary["max_seconds"] != 40.0:
        raise RuntimeError("maximum GGUF KVM incorrect")
    if summary["spread_seconds"] != 30.0:
        raise RuntimeError("dispersion GGUF KVM incorrecte")
    if summary["spread_ratio"] != 1.5:
        raise RuntimeError("ratio de dispersion GGUF KVM incorrect")
    try:
        module.summarize([])
    except ValueError:
        pass
    else:
        raise RuntimeError("echantillon vide accepte")

    missing_ok, missing_reason = module.kvm_usable("/no/such/kvm/device")
    if missing_ok or "kvm_device_missing" not in missing_reason:
        raise RuntimeError("probe missing KVM incorrect: %s" % missing_reason)

    with tempfile.NamedTemporaryFile() as handle:
        os.chmod(handle.name, 0o000)
        try:
            denied_ok, denied_reason = module.kvm_usable(handle.name)
        finally:
            os.chmod(handle.name, 0o600)
        if denied_ok or "kvm_device_permission_denied" not in denied_reason:
            # Some environments run as root and still open the file; accept open_failed too.
            if denied_ok or (
                "kvm_device_permission_denied" not in denied_reason
                and "kvm_device_open_failed" not in denied_reason
            ):
                raise RuntimeError("probe denied KVM incorrect: %s" % denied_reason)

    argv = module.qemu_kvm_argv("/tmp/log", "/tmp/mon.sock")
    joined = " ".join(argv)
    if "accel=kvm" not in joined:
        raise RuntimeError("argv KVM manquant accel=kvm")
    if "accel=tcg" in joined:
        raise RuntimeError("argv KVM ne doit pas utiliser accel=tcg")
    if "-kernel" not in argv or "-initrd" not in argv:
        raise RuntimeError("argv KVM doit rester Multiboot -kernel/-initrd")
    if module.TCG_REF_FIRST_MEDIAN_S != 48.739:
        raise RuntimeError("seuil TCG premier token incorrect")
    if module.TCG_REF_CONTINUATION_MEDIAN_S != 22.781:
        raise RuntimeError("seuil TCG continuation incorrect")
    if module.SUB_SECOND_CLAIM_SECONDS != 1.0:
        raise RuntimeError("seuil sous-seconde incorrect")

    live_ok, live_reason = module.kvm_usable()
    print("GGUF KVM protocol check passed (live_kvm=%s reason=%s)" %
          (1 if live_ok else 0, live_reason))


if __name__ == "__main__":
    main()
