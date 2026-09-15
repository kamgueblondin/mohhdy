# AOS-171 — Composition TLS sur TCP caller-owned

AOS-171 ajoute `net_tcp_connection_build_tls_record`. La primitive construit d’abord un record TLS dans un buffer fourni par l’appelant, puis l’encapsule comme payload d’un segment TCP caller-owned. Le payload pending référence le record fourni ; aucune allocation, copie cachée ou génération de clé n’est effectuée.

Le sequence TCP reste inchangé avant `net_tcp_connection_commit_send`. Après commit, la longueur consommée est celle du record TLS complet, en-tête TLS de cinq octets compris. Le même buffer peut ainsi être présenté au mécanisme de retransmission bornée déjà existant.

Cette composition ne constitue pas un handshake TLS complet. Le ClientHello est seulement encadré et transportable ; SNI/ALPN, dérivation de clés, chiffrement, validation X.509, retransmission avec RTO adaptatif, HTTP et appels LLM en ligne restent à implémenter.

| Élément | Statut |
|---|---|
| Construction record TLS dans buffer appelant | Implémentée. |
| Encapsulation comme payload TCP | Implémentée. |
| Pending/retransmission sans allocation | Réutilise l’état TCP existant. |
| Sequence avancé après commit uniquement | Validé. |
| Handshake cryptographique TLS | Non implémenté. |
| HTTP et appels LLM en ligne | Non fonctionnels de bout en bout. |
