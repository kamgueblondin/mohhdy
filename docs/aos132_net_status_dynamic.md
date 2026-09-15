# AOS-132 — Statut NIC dynamique et smoke NE2000

Le lot AOS-132 remplace le diagnostic réseau statique par le syscall `SYS_NET_STATUS`. Le noyau retourne un bitmask sans pointeur utilisateur : le bit 0 indique qu’une carte NE2000 est détectée et le bit 1 que ses anneaux RX/TX ont été configurés. Le shell traduit ce résultat vers `net-status` et `net-status json`, tout en maintenant ARP, IPv4, DHCP, DNS, TCP et TLS explicitement absents tant que leurs chemins matériels ne sont pas raccordés.

La sonde NE2000 est renforcée contre les faux positifs. Les ports flottants QEMU sans carte renvoient `0xff` et restent absents ; le contrôleur `ne2k_isa` QEMU est reconnu malgré le readback DCR non disponible sur ce modèle. La configuration DCR est toujours écrite, mais le critère de présence repose sur les valeurs reset et ISR matériellement observables.

La validation locale est de **292 tests Unity verts**, avec build i386 réussi, smoke fournisseur sans NIC réussi et smoke matériel `qemu-ne2k-status` réussi avec `-netdev user -device ne2k_isa,netdev=n0`.

Le lot ne fournit pas encore DHCP effectif, DMA distant, IRQ réseau, lecture PROM MAC, handshake TLS, validation X.509 ou client HTTP/OpenAI.

## Contrats exposés

| Élément | Contrat |
|---|---|
| `SYS_NET_STATUS` | Retourne un bitmask sans buffer utilisateur. |
| `net-status` | Affiche la présence ou l’absence réelle de la NIC sondée au boot. |
| `net-status json` | Produit un diagnostic machine-lisible ; les couches supérieures restent `absent`. |
| QEMU sans NIC | Retour attendu : `"nic":"absent"`. |
| QEMU avec `ne2k_isa` | Retour attendu : `"nic":"detected"`. |
