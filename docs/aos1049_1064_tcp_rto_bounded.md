# AOS-1049 à AOS-1064 — RTO TCP caller-owned borné

Le module TCP expose désormais `net_tcp_rto_timer_t`, un minuteur de retransmission appartenant à l’appelant. `net_tcp_rto_init` initialise un délai strictement positif, `net_tcp_rto_arm` programme une échéance bornée, et `net_tcp_rto_ready` permet un polling non bloquant.

`net_tcp_rto_consume` ne déclenche ni émission ni attente active. Lorsque l’échéance est atteinte, il consomme une reprise déjà autorisée par `net_tcp_connection_t`, incrémente le compteur de retransmission, double le délai jusqu’à `max_delay`, puis réarme le minuteur. En cas d’échec, le timer et la connexion sont restaurés.

La conception évite tout timer global, toute logique réseau dans IRQ0 et toute allocation dynamique. Le caller reste responsable de lire l’horloge, de reconstruire le segment depuis `pending_payload` et de le transmettre via NE2000. La limite existante `retransmit_limit` reste la source d’autorité pour l’épuisement du budget.

> Le RTO fournit une échéance ; il ne prend pas possession de la boucle réseau.

| Élément | Garantie |
|---|---|
| Horloge | Tick fourni par l’appelant |
| Attente | Aucun blocage, polling uniquement |
| Backoff | Doublement borné par `max_delay` |
| Budget | Réutilise `retransmit_limit` existant |
| Rollback | Timer et connexion restaurés en cas d’erreur |
| Mémoire | Aucune allocation, état caller-owned |

Validation locale : **414/414 tests verts**, dont le test d’échéance, du backoff et du budget RTO.

Auteur : **Manus AI**

