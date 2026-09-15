# AOS-159 à AOS-162 — Envoi suivi d’ACK, réception et fenêtre TCP

AOS-159 ajoute `net_tcp_connection_build_data`, qui construit un segment à partir du sequence courant et enregistre le payload fourni par l’appelant comme pending. Le sequence local n’avance qu’après `net_tcp_connection_commit_send`, ce qui sépare explicitement la construction, la transmission et la confirmation locale du TX.

AOS-160 ajoute `ne2k_tcp_receive`. La primitive vérifie la trame Ethernet IPv4, la version et la longueur IPv4, le protocole TCP, les checksums IPv4 et TCP, puis délègue la validation des ports, du flag ACK et de la séquence à la connexion caller-owned. Le payload est copié uniquement dans le buffer fourni par l’appelant et sa capacité est contrôlée avant toute copie.

AOS-161 ajoute une fenêtre de réception caller-owned. Elle est initialisée à une valeur maximale prudente, peut être configurée explicitement et diminue après chaque payload accepté. Un segment supérieur à la fenêtre restante est rejeté.

AOS-162 durcit les contrôles cryptographiques de transport local avec validation du checksum IPv4 et du checksum TCP pseudo-en-tête IPv4 sur les trames reçues. Cette validation ne fournit pas TLS : elle protège uniquement l’intégrité de la trame TCP reçue au niveau IPv4/TCP.

| Fonctionnalité | Statut réel |
|---|---|
| Construction data avec pending caller-owned | Implémentée. |
| Commit du sequence après TX | Implémenté. |
| Réception TCP NE2000 bornée | Implémentée. |
| Checksum IPv4 en réception | Validé. |
| Checksum TCP pseudo-en-tête en réception | Validé. |
| Fenêtre de réception explicite | Implémentée. |
| TLS, HTTP, congestion et LLM en ligne | Non fonctionnels de bout en bout. |

Les tests ciblés TCP et NE2000 passent. La suite globale, le build i386 et les smokes QEMU restent à exécuter avant publication.
