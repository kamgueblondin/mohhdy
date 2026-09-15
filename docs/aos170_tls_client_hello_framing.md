# AOS-170 — ClientHello TLS 1.2 minimal caller-owned

AOS-170 ajoute `net_tls_client_hello_build`. Cette primitive construit un record TLS Handshake contenant un ClientHello TLS 1.2 minimal : version annoncée, random de 32 octets fourni par l’appelant, session vide, une suite cryptographique explicitement annoncée et compression nulle.

La primitive est volontairement limitée au framing. Elle ne génère pas de random cryptographique, ne négocie pas les extensions SNI/ALPN, ne dérive aucune clé, ne chiffre aucun record et ne valide aucun certificat. Le raccordement au transport TCP doit donc être traité comme une étape ultérieure, avec une politique de vérification indépendante.

| Élément | Statut |
|---|---|
| Construction ClientHello caller-owned | Implémentée. |
| Version TLS 1.2 annoncée | Implémentée. |
| Random cryptographique | Fourni par l’appelant ; aucune génération interne. |
| SNI/ALPN, dérivation de clés, chiffrement | Non implémentés. |
| Validation X.509 et handshake complet | Non implémentés. |
| HTTP et appels LLM en ligne | Non fonctionnels de bout en bout. |

Le test TLS record couvre désormais la construction/parsing du record générique et le ClientHello minimal, y compris le rejet d’une capacité insuffisante.
