# AOS-118 — Validation de l’adresse MAC NE2000

Le lot AOS-118 complète la sonde NE2000 par `ne2k_set_mac`, une opération caller-owned qui copie une adresse MAC locale après validation. Les adresses nulles et multicast sont rejetées; aucune allocation ni référence vers le buffer d’entrée n’est conservée.

Le test NE2000 vérifie une adresse unicast valide, le rejet de `00:00:00:00:00:00` et le rejet d’une adresse dont le bit multicast est positionné. La validation locale est de **274 tests verts** avec compilation Unity complète. Le pilote ne lit pas encore la PROM matérielle et ne transmet pas de trame; cette extension sécurise le contrat de configuration avant l’implémentation RX/TX.
