# AOS-166/AOS-167 — Polling TCP suivi d’ACK automatique

AOS-166 ajoute `ne2k_tcp_poll_ack`. La primitive appelle le polling TCP caller-owned, valide la trame et la fenêtre, copie le payload accepté, puis construit et transmet l’ACK avec les adresses IPv4 et le cache ARP fournis par l’appelant.

AOS-167 formalise les codes de retour. Une absence de trame retourne `1`, publie une longueur nulle et n’émet rien. Une trame valide avec payload retourne `0` uniquement après transmission réussie de l’ACK. Une erreur de cache ARP ou de transmission est propagée après l’acceptation du payload ; l’appelant dispose alors de la longueur et peut décider d’une nouvelle tentative ou d’une retransmission selon sa politique.

L’orchestration n’introduit aucun buffer statique caché, aucune allocation et aucun timer. Le buffer RX, le buffer TX, le payload, l’état TCP, le cache ARP et les adresses IP sont tous caller-owned.

| Cas | Résultat |
|---|---|
| RX vide | Retour `1`, longueur `0`, aucune émission. |
| Trame invalide | Erreur négative, payload non publié. |
| Payload accepté et ACK transmis | Retour `0`, longueur publiée. |
| Payload accepté mais ARP/TX échoue | Erreur négative, longueur conservée pour décision appelante. |
| TLS, HTTP, RTO et LLM en ligne | Non fonctionnels de bout en bout. |

Les tests NE2000 couvrent au minimum le cas RX vide, l’absence d’émission ACK et la conservation de la longueur nulle.
