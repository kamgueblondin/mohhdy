# AOS-142 — Cache ARP statique caller-owned

Le lot AOS-142 ajoute un cache ARP de capacité fixe (`NET_ARP_CACHE_CAPACITY`), stocké par l’appelant. Chaque entrée contient une IPv4, une MAC et un indicateur de validité. Les opérations d’initialisation, insertion, mise à jour, recherche et invalidation n’utilisent aucune allocation dynamique.

Une insertion met à jour l’entrée existante lorsque l’IPv4 est déjà présente ; sinon, elle utilise la première entrée libre. Lorsque le cache est plein, l’insertion échoue explicitement au lieu d’évincer silencieusement une entrée. Cette politique rend la consommation mémoire et le comportement déterministes dans le chemin réseau bare-metal.

| Élément | État AOS-142 |
|---|---|
| Cache à capacité fixe | Implémenté. |
| Lookup IPv4 → MAC | Implémenté. |
| Mise à jour et invalidation | Implémentées. |
| Allocation dynamique | Aucune. |
| Requête ARP et attente RX | Prochain sous-lot. |
| Intégration automatique à `ne2k_tx_udp` | Prochain sous-lot. |
| DHCP/DNS/TCP/TLS/HTTP/OpenAI | Toujours non déclarés fonctionnels sur le transport matériel. |

La validation locale atteint **294 tests verts**, avec build i386 réussi. Les smokes QEMU de boot et de détection NE2000 restent à rejouer avant publication.
