# AOS-153/AOS-154 — Progression TCP et retransmission bornée

AOS-153 ajoute la progression explicite des numéros de séquence. Après émission confirmée par l’appelant, `net_tcp_connection_commit_send` avance `local_sequence` de la longueur du payload. Pour la réception, `net_tcp_connection_accept_data` vérifie les ports, le drapeau ACK, le numéro de séquence attendu et l’acquittement compatible avant d’avancer `remote_sequence` et de retourner la longueur acceptée.

AOS-154 ajoute uniquement les métadonnées nécessaires à une retransmission bornée : pointeur vers le payload appartenant à l’appelant, longueur, compteur courant et limite de tentatives. Aucune donnée n’est copiée et aucun timer n’est créé. L’appelant doit piloter la temporisation, reconstruire le segment avec la vue caller-owned et appeler `net_tcp_connection_note_retransmit` après une tentative autorisée.

| Élément | État réel |
|---|---|
| Progression du sequence local après envoi confirmé | Implémentée. |
| Acceptation de données en séquence | Implémentée. |
| Rejet d’un segment hors séquence | Implémenté. |
| Métadonnées de retransmission caller-owned | Implémentées. |
| Limite stricte de tentatives | Implémentée. |
| Timer, RTO, fast retransmit et congestion | Non implémentés. |
| TLS, HTTP et LLM en ligne | Non fonctionnels de bout en bout. |

Les tests ciblés TCP passent après ces ajouts. La validation globale et la publication de la PR correspondante restent obligatoires avant fusion.
