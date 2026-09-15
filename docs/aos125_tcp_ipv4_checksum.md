# AOS-125 — Checksum TCP IPv4

Le lot AOS-125 complète le codec TCP par le calcul du checksum avec pseudo-en-tête IPv4. La fonction additionne les adresses source et destination, le protocole TCP, la longueur et les octets du segment, puis replie les retenues 16 bits. Elle accepte les segments de longueur paire ou impaire et ne modifie jamais le buffer caller-owned.

Ce lot prépare la validation du segment TCP avant émission sur IPv4. Il ne fournit pas encore de machine d’état, de retransmission, de contrôle de fenêtre, de checksum écrit automatiquement dans l’en-tête, de TLS ou de HTTP.

Le test TCP couvre la stabilité du checksum pour un segment SYN-ACK. La validation locale est de **280 tests verts**, avec build i386 et initrd réussis.
