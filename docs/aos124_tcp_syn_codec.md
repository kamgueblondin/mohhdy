# AOS-124 — Codec TCP SYN/SYN-ACK caller-owned

Le lot AOS-124 ajoute un codec TCP minimal pour construire un segment SYN et parser un segment TCP avec en-tête standard. Les ports nuls, les en-têtes inférieurs à 20 octets et les longueurs d’en-tête incohérentes sont rejetés. Les séquences, acquittements, drapeaux et payload sont exposés dans une vue bornée sans allocation dynamique.

Le codec ne fournit pas encore de checksum pseudo-en-tête, de retransmission, de temporisation, de fenêtre ou d’état de connexion. Il établit le contrat de représentation nécessaire avant l’intégration TCP sur IPv4 et la future couche TLS.

Le test `test_net_tcp.c` couvre la construction SYN, le parsing SYN-ACK et le rejet d’un port source nul. La validation locale est de **280 tests verts**, avec build i386 et initrd réussis.
