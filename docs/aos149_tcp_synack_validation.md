# AOS-149 — Validation SYN-ACK caller-owned

Le lot AOS-149 ajoute `net_tcp_is_syn_ack_for`. À partir d’une vue TCP caller-owned, la primitive vérifie les ports inversés attendus, la présence simultanée des drapeaux SYN et ACK et un acquittement égal au numéro de séquence local augmenté de un. Le numéro de séquence distant est copié vers une sortie fournie par l’appelant.

Cette validation prépare l’émission du premier ACK mais ne crée pas encore d’état de connexion persistant. Aucun timer, aucune retransmission et aucune allocation ne sont introduits.

| Élément | État AOS-149 |
|---|---|
| Validation des ports SYN-ACK | Implémentée. |
| Validation SYN et ACK | Implémentée. |
| Validation de l’acquittement | Implémentée. |
| Capture du sequence distant | Caller-owned et implémentée. |
| Construction du premier ACK | Prochain lot. |
| État TCP persistant/retransmission | Non implémenté. |
| TLS, HTTP et OpenAI | Toujours non déclarés fonctionnels sur le transport matériel. |

La validation locale atteint **294 tests verts**, avec build i386 réussi.
