# AOS-150 — Premier ACK TCP et état de connexion caller-owned

Le lot AOS-150 complète le début du handshake TCP côté client. Il ajoute la construction d’un segment ACK de 20 octets et un état de connexion minimal fourni par l’appelant. L’implémentation ne fait appel à aucune allocation dynamique et ne conserve aucun état global.

Lors de l’ouverture, le numéro de séquence initial fourni par l’appelant est consommé par le SYN et `local_sequence` devient `ISN + 1`. Après réception d’un SYN-ACK validé par AOS-149, le numéro distant devient `remote_sequence + 1`, puis l’état passe de `SYN_SENT` à `ESTABLISHED`. Le premier ACK est alors construit avec ces deux valeurs et les ports local/distant inversés par rapport au SYN entrant.

| Élément | État |
|---|---|
| Construction ACK caller-owned | Implémentée. |
| Contrôle de capacité et ports nuls | Implémenté. |
| État `CLOSED`, `SYN_SENT`, `ESTABLISHED` | Implémenté dans une structure fournie par l’appelant. |
| Validation stricte du SYN-ACK | Réutilise AOS-149. |
| Émission physique du premier ACK | Non raccordée à l’orchestrateur NE2000 dans ce lot. |
| Données TCP, fenêtres, retransmissions et timers | Non implémentés. |
| TLS, HTTP et appels LLM en ligne | Non fonctionnels de bout en bout à ce stade. |

La validation locale atteint **295 tests verts**, avec build i386 réussi et smokes QEMU sans NIC et NE2000 réussis.
