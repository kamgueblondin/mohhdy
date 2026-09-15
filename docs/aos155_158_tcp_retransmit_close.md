# AOS-155 à AOS-158 — Retransmission et fermeture TCP caller-owned

AOS-155 raccorde la retransmission bornée au chemin NE2000. `ne2k_tcp_retransmit` réutilise le pointeur et la longueur conservés dans `net_tcp_connection_t`, reconstruit le segment avec le même numéro de séquence logique, transmet la trame et incrémente le compteur seulement après succès de la transmission. La temporisation reste pilotée par l’appelant ; aucun timer noyau n’est introduit.

AOS-156 ajoute la validation d’un ACK distant. Les ports, le drapeau ACK et la borne d’acquittement sont contrôlés. Lorsqu’un payload pending est confirmé, seules ses métadonnées sont réinitialisées ; aucune libération mémoire n’est effectuée puisque le stockage appartient à l’appelant.

AOS-157 et AOS-158 ajoutent le codec FIN+ACK, les états `FIN_WAIT_1`, `FIN_WAIT_2`, `CLOSE_WAIT` et `CLOSED`, ainsi que l’émission NE2000 du FIN. La transition vers `FIN_WAIT_1` est publiée après succès TX. Un ACK distant fait progresser `FIN_WAIT_1` vers `FIN_WAIT_2`, et un FIN reçu dans `FIN_WAIT_2` clôt la connexion après consommation de son numéro de séquence. Les états `CLOSE_WAIT` et `FIN_WAIT_2` peuvent construire un ACK caller-owned.

| Fonctionnalité | Statut |
|---|---|
| Retransmission NE2000 bornée | Implémentée. |
| Confirmation et purge du pending | Implémentée. |
| Codec FIN+ACK | Implémenté. |
| Émission FIN+ACK via NE2000 | Implémentée. |
| États FIN_WAIT_1/2, CLOSE_WAIT/CLOSED | Implémentés minimalement. |
| Timers/RTO/congestion | Non implémentés. |
| TLS, HTTP et appels LLM en ligne | Non fonctionnels de bout en bout. |

Les tests ciblés TCP et NE2000 couvrent la reprise du sequence, la limite de retransmission, l’ACK final, les transitions FIN et l’ACK après FIN distant.
