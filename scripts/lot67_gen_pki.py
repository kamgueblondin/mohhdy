#!/usr/bin/env python3
"""Lot 67: generates the offline test PKI (test CA only, never in the
production initrd) and the C header of DER fixtures for the X.509 host
tests. Keys are throw-away test keys. Run from the repo root."""
import datetime, ipaddress, os, sys
from cryptography import x509
from cryptography.x509.oid import NameOID, ExtendedKeyUsageOID
from cryptography.hazmat.primitives import hashes, serialization
from cryptography.hazmat.primitives.asymmetric import ec, rsa, padding

SERVER_ONLY = len(sys.argv) > 1
OUT = sys.argv[1] if SERVER_ONLY else "build/lot67_pki"
os.makedirs(OUT, exist_ok=True)
NOW = datetime.datetime(2026, 1, 1, tzinfo=datetime.timezone.utc)

def name(cn):
    return x509.Name([x509.NameAttribute(NameOID.ORGANIZATION_NAME, "MOHHDY test"),
                      x509.NameAttribute(NameOID.COMMON_NAME, cn)])

def cert(subject, issuer, key, ikey, ca, pathlen=None, sans=(), ips=(), days=(0, 3650),
         h=hashes.SHA256(), ku_sign=True, pss=False, extra_critical=False, bc=True):
    b = (x509.CertificateBuilder().subject_name(name(subject)).issuer_name(name(issuer))
         .public_key(key.public_key()).serial_number(x509.random_serial_number())
         .not_valid_before(NOW + datetime.timedelta(days=days[0]))
         .not_valid_after(NOW + datetime.timedelta(days=days[1])))
    if bc:
        b = b.add_extension(x509.BasicConstraints(ca=ca, path_length=pathlen), critical=True)
    if ca:
        b = b.add_extension(x509.KeyUsage(digital_signature=False, content_commitment=False, key_encipherment=False,
                                          data_encipherment=False, key_agreement=False, key_cert_sign=ku_sign,
                                          crl_sign=True, encipher_only=False, decipher_only=False), critical=True)
    else:
        b = b.add_extension(x509.ExtendedKeyUsage([ExtendedKeyUsageOID.SERVER_AUTH]), critical=False)
    alt = [x509.DNSName(s) for s in sans] + [x509.IPAddress(ipaddress.ip_address(i)) for i in ips]
    if alt:
        b = b.add_extension(x509.SubjectAlternativeName(alt), critical=False)
    if extra_critical:
        b = b.add_extension(x509.UnrecognizedExtension(x509.ObjectIdentifier("1.3.6.1.4.1.55555.1"), b"\x05\x00"),
                            critical=True)
    if pss:
        return b.sign(ikey, h, rsa_padding=padding.PSS(mgf=padding.MGF1(h), salt_length=h.digest_size))
    return b.sign(ikey, h)

root_k = ec.generate_private_key(ec.SECP384R1())
inter_k = rsa.generate_private_key(65537, 2048)
leaf_k = ec.generate_private_key(ec.SECP256R1())
rogue_k = ec.generate_private_key(ec.SECP256R1())

root = cert("MOHHDY Test Root", "MOHHDY Test Root", root_k, root_k, True, 1, h=hashes.SHA384())
inter = cert("MOHHDY Test Inter", "MOHHDY Test Root", inter_k, root_k, True, 0, h=hashes.SHA384())
SANS = ("mohhdy.test", "*.mohhdy.test")
leaf = cert("mohhdy.test", "MOHHDY Test Inter", leaf_k, inter_k, False, sans=SANS, ips=("10.0.2.2",))
leaf_pss = cert("pss.mohhdy.test", "MOHHDY Test Inter", leaf_k, inter_k, False, sans=("pss.mohhdy.test",), pss=True)
expired = cert("old.mohhdy.test", "MOHHDY Test Inter", leaf_k, inter_k, False, sans=("old.mohhdy.test",), days=(-400, -30))
future = cert("new.mohhdy.test", "MOHHDY Test Inter", leaf_k, inter_k, False, sans=("new.mohhdy.test",), days=(3000, 3600))
crit = cert("crit.mohhdy.test", "MOHHDY Test Inter", leaf_k, inter_k, False, sans=("crit.mohhdy.test",), extra_critical=True)
# Lot 69: RSA leaf for TLS 1.3 RSA-PSS CertificateVerify and TLS 1.2 ECDHE-RSA over P-256
leaf_rsa_k = rsa.generate_private_key(65537, 2048)
leaf_rsa = cert("mohhdy.test", "MOHHDY Test Inter", leaf_rsa_k, inter_k, False, sans=SANS, ips=("10.0.2.2",))
# a CA:false "intermediate" that signs a leaf
noca_k = ec.generate_private_key(ec.SECP256R1())
noca = cert("MOHHDY Not A CA", "MOHHDY Test Root", noca_k, root_k, False, h=hashes.SHA384())
noca_leaf = cert("x.mohhdy.test", "MOHHDY Not A CA", leaf_k, noca_k, False, sans=("x.mohhdy.test",))
# pathlen 0 intermediate signing another CA
sub_k = ec.generate_private_key(ec.SECP256R1())
sub = cert("MOHHDY Test Sub", "MOHHDY Test Inter", sub_k, inter_k, True)
sub_leaf = cert("deep.mohhdy.test", "MOHHDY Test Sub", leaf_k, sub_k, False, sans=("deep.mohhdy.test",))
# same names as the real chain but signed by a rogue key
forged = cert("mohhdy.test", "MOHHDY Test Inter", leaf_k, rogue_k, False, sans=SANS)
# SHA-1 signed leaf
try:
    sha1 = cert("sha1.mohhdy.test", "MOHHDY Test Inter", leaf_k, inter_k, False, sans=("sha1.mohhdy.test",), h=hashes.SHA1())
except Exception:
    sha1 = None   # this cryptography build refuses SHA-1 signing; the C test covers the OID path

def der(c): return c.public_bytes(serialization.Encoding.DER)
def pem(c): return c.public_bytes(serialization.Encoding.PEM)

# files for the QEMU HTTPS test server (test CA goes only into my_initrd_test.tar)
open(f"{OUT}/test_ca.pem", "wb").write(pem(root))
open(f"{OUT}/server_chain.pem", "wb").write(pem(leaf) + pem(inter))
open(f"{OUT}/server_key.pem", "wb").write(leaf_k.private_bytes(serialization.Encoding.PEM,
        serialization.PrivateFormat.PKCS8, serialization.NoEncryption()))
open(f"{OUT}/server_rsa_chain.pem", "wb").write(pem(leaf_rsa) + pem(inter))
open(f"{OUT}/server_rsa_key.pem", "wb").write(leaf_rsa_k.private_bytes(serialization.Encoding.PEM,
        serialization.PrivateFormat.PKCS8, serialization.NoEncryption()))

def arr(n, b):
    s = ",".join(f"0x{x:02x}" for x in b)
    return f"static const unsigned char {n}[] = {{{s}}};\n"

if SERVER_ONLY:
    print("ok server pki in", OUT)
    sys.exit(0)
items = [("root", root), ("inter", inter), ("leaf", leaf), ("leaf_pss", leaf_pss), ("expired", expired),
         ("future", future), ("crit", crit), ("noca", noca), ("noca_leaf", noca_leaf), ("sub", sub),
         ("sub_leaf", sub_leaf), ("forged", forged)]
if sha1 is not None:
    items.append(("sha1", sha1))
with open("tests/unit/userspace/wf_x509_fixtures.h", "w") as f:
    f.write("/* Generated by scripts/lot67_gen_pki.py: throw-away test PKI. */\n")
    f.write("#ifndef WF_X509_FIXTURES_H\n#define WF_X509_FIXTURES_H\n")
    for n, c in items:
        f.write(arr("fx_" + n, der(c)))
    f.write(f"#define FX_HAVE_SHA1 {1 if sha1 is not None else 0}\n")
    f.write("#endif\n")
print("ok", len(items))
