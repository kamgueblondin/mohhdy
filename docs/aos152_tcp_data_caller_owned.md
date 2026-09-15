# AOS-152 — Données TCP caller-owned

AOS-152 ajoute la construction bornée d’un segment TCP `ACK+payload` dans `net_tcp_build_data` et son émission Ethernet/IPv4 via `ne2k_tcp_data`. Le buffer de segment, le payload, le cache ARP et l’état de connexion restent fournis par l’appelant. Le constructeur ne modifie pas `local_sequence` : l’appelant conserve la responsabilité de confirmer l’émission et de faire évoluer son état selon la politique de retransmission future.

La voie NE2000 construit une trame IPv4 de longueur exacte, calcule le checksum TCP sur pseudo-en-tête IPv4, calcule le checksum IPv4 et transmet par `ne2k_tx_submit`. Les contrôles rejettent les ports ou pointeurs invalides, les capacités insuffisantes, les payloads incohérents et les MAC distantes absentes du cache ARP.

| Élément | Statut réel |
|---|---|
| Codec TCP ACK+payload | Implémenté et testé. |
| Parsing caller-owned du payload | Réutilise `net_tcp_parse`. |
| Émission NE2000 ACK+payload | Implémentée et testée. |
| Mise à jour automatique du sequence après émission | Non réalisée volontairement. |
| Retransmission, fenêtre, congestion, FIN/RST | Non implémentés. |
| TLS, HTTP et client LLM en ligne | Non fonctionnels de bout en bout. |

La non-régression locale atteint **297 tests verts**. Le premier smoke `qemu-ai-provider` a eu un échec ponctuel sans rapport avec TCP, puis a réussi lors de la relance ; le smoke NE2000 est vert.
