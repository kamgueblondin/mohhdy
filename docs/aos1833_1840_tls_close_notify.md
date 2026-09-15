# AOS-1833…1840 — Alerte TLS `close_notify`

## Objet

Ce macro-lot ajoute `net_tls_close_notify_build()`, qui construit l’alerte TLS 1.2 standard `warning/close_notify` dans un record AES-128-GCM chiffré. Le constructeur réutilise la session TLS caller-owned et son mécanisme d’avancement transactionnel de numéro de séquence.

| Élément | Valeur |
|---|---|
| Type de record externe | `Alert` (21) |
| Alerte en clair avant chiffrement | `warning` (1), `close_notify` (0) |
| Protection | AES-128-GCM de la session TLS |
| Séquence d’écriture | Avancée uniquement après record construit |

Le vecteur de test ouvre le record côté serveur avec les clés directionnelles correspondantes, vérifie le type `Alert` et les deux octets de l’alerte. Il force également une capacité insuffisante et vérifie que la séquence cliente reste à zéro.

> Cette primitive prépare une fermeture TLS ordonnée. L’émission NE2000 et le handshake TCP `FIN` restent des responsabilités de la couche de transport appelante.

## Références

[1] [RFC 5246 — The Transport Layer Security Protocol Version 1.2, §7.2.1](https://www.rfc-editor.org/rfc/rfc5246#section-7.2.1)
