# AOS-168/AOS-169 — Contrôle FIN et ACK de fermeture

AOS-168 ajoute `ne2k_tcp_poll_fin_ack`. Cette primitive lit une trame TCP depuis la RAM NE2000, vérifie Ethernet/IPv4/TCP et les deux checksums, exige le flag FIN, puis délègue la transition de fermeture à l’état TCP caller-owned avant d’émettre l’ACK de fermeture.

AOS-169 sépare le traitement des FIN des ACK purs. Un ACK sans payload ne déclenche pas automatiquement un nouvel ACK, ce qui évite une boucle de contrôle. Le chemin FIN→ACK reste explicitement piloté par l’appelant et réutilise les buffers RX/TX ainsi que le cache ARP fournis.

Le codec TLS record existant (`kernel/net_tls_record.c`) reste limité au framing TLS 1.2 caller-owned. Il ne réalise encore ni négociation cryptographique, ni validation de certificats, ni chiffrement ; son raccordement au transport TCP constitue un prochain lot distinct.

| Fonction | Statut |
|---|---|
| FIN reçu et validé | Implémenté dans le polling dédié. |
| ACK FIN transmis via NE2000 | Implémenté après transition d’état. |
| ACK pur sans réponse automatique | Protégé contre les boucles. |
| Framing TLS record | Existant et testé séparément. |
| Handshake TLS cryptographique | Non implémenté. |
| HTTP et appels LLM en ligne | Non fonctionnels de bout en bout. |
