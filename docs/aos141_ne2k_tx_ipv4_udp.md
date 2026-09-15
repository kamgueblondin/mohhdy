# AOS-141 — Émission IPv4/UDP Ethernet caller-owned

Le lot AOS-141 ajoute `ne2k_tx_udp`. La primitive construit dans un buffer fourni par l’appelant l’en-tête Ethernet, le paquet IPv4/UDP et son payload, puis réutilise `ne2k_tx_submit` pour le transfert PIO vers la RAM distante du NE2000 et le déclenchement TX.

La MAC source provient de l’identité locale validée du périphérique, tandis que la MAC destination est fournie par l’appelant. Les adresses IPv4, ports et payload sont transmis au codec UDP existant, qui calcule le checksum IPv4. La capacité maximale Ethernet est vérifiée avant toute émission ; le padding Ethernet minimal reste assuré par le TX NE2000.

| Élément | État AOS-141 |
|---|---|
| Construction Ethernet + IPv4/UDP | Implémentée. |
| Émission PIO NE2000 | Réutilisée et validée. |
| Buffers caller-owned | Respectés. |
| Résolution automatique ARP avant émission | Non orchestrée dans ce lot. |
| Échange UDP réel sous QEMU | Non déclaré. |
| DHCP/DNS/TCP/TLS/HTTP/OpenAI | Toujours non déclarés fonctionnels sur le transport matériel. |

La suite locale reste à **293 tests verts**, avec build i386 réussi. Les smokes QEMU vérifient le boot, l’absence de fournisseur matériel et la détection NE2000, mais pas encore la transmission UDP sur un réseau virtuel.
