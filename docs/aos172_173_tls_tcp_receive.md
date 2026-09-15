# AOS-172/AOS-173 — Parsing TLS depuis TCP caller-owned

AOS-172 ajoute `net_tls_record_parse_stream`, qui distingue un en-tête incomplet, un record incomplet, un record invalide et un record complet. Lorsque le record est complet, la primitive publie une vue caller-owned et le nombre exact d’octets consommés.

AOS-173 ajoute `net_tcp_connection_accept_tls_record`. La connexion TCP parse le premier record TLS contenu dans son payload, exige que le record occupe exactement le payload reçu, puis avance la séquence distante uniquement après validation complète du record et des contrôles TCP existants.

Les fragments TLS restent sous la responsabilité de l’appelant. Le noyau ne maintient ni buffer d’assemblage global ni allocation implicite : l’appelant doit conserver un buffer caller-owned, concaténer les fragments et rappeler le parseur lorsque la longueur annoncée est disponible.

| Élément | Statut |
|---|---|
| Parsing stream TLS borné | Implémenté. |
| Acceptation TLS depuis une vue TCP | Implémentée. |
| Record incomplet sans progression TCP | Validé. |
| Octets supplémentaires non consommés | Rejetés par le chemin strict. |
| Buffer d’assemblage TLS | Caller-owned, non alloué par le noyau. |
| Cryptographie, X.509, HTTP et LLM en ligne | Non fonctionnels de bout en bout. |
