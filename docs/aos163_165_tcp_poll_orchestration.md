# AOS-163 à AOS-165 — Polling TCP et orchestration RX caller-owned

AOS-163 ajoute `ne2k_tcp_poll`, une primitive d’orchestration qui lit une trame via `ne2k_rx_poll`, puis appelle `ne2k_tcp_receive` pour vérifier IPv4/TCP, checksums, ports, séquence et fenêtre avant copie du payload. Tous les buffers restent fournis par l’appelant.

AOS-164 formalise la sémantique non bloquante du polling : une absence de paquet retourne `1` et publie `payload_length = 0`. Une erreur de périphérique ou de trame conserve également une longueur de payload nulle, ce qui évite de réutiliser une valeur stale de l’appelant.

AOS-165 regroupe les contrôles RX déjà ajoutés — bornes de trame, capacité de payload, checksum IPv4, checksum TCP pseudo-en-tête IPv4 et séquence TCP — dans un chemin utilisable directement par l’orchestrateur NE2000 sans allocation ni buffer global.

| Élément | Statut |
|---|---|
| Polling NE2000 puis extraction TCP | Implémenté. |
| RX vide non bloquant | Implémenté. |
| Longueur caller-owned réinitialisée | Implémentée. |
| Checksums IPv4/TCP | Validés avant copie. |
| Fenêtre et séquence TCP | Validées par la connexion. |
| Réponse ACK automatique sur RX | Reste un lot suivant ; l’orchestrateur expose d’abord le payload. |
| TLS, HTTP et LLM en ligne | Non fonctionnels de bout en bout. |

Les tests ciblés NE2000 passent après le nouveau contrat RX vide. La non-régression et les smokes restent obligatoires avant publication.
