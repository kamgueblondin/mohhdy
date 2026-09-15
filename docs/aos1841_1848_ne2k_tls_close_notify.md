# AOS-1841…1848 — Émission NE2000 de `close_notify`

## Objet

Ce macro-lot ajoute `ne2k_https_llm_close_notify()`. La primitive envoie un record TLS `warning/close_notify` chiffré sur une connexion TCP NE2000 établie, avec les mêmes buffers caller-owned, suivi de séquence TLS et suivi TCP que les requêtes HTTPS LLM existantes.

| Étape | Garantie |
|---|---|
| Handshake TLS incomplet | Rejet avant toute mutation |
| Construction du record | Utilise `net_tls_close_notify_build()` |
| Suivi TCP et émission NE2000 | Réutilise le chemin HTTPS transactionnel |
| Erreur avant commit | Connexion et séquence TLS restaurées |
| FIN TCP | Volontairement séparé, à appeler après l’alerte |

La séparation du FIN évite de prétendre qu’une transmission déjà remise au matériel peut être annulée transactionnellement. Elle laisse à l’appelant la possibilité d’attendre le traitement du record, puis d’appeler la primitive TCP de fermeture adaptée.

Le test NE2000 couvre les entrées nulles et les deux formes de handshake incomplet. Il vérifie que la connexion et la séquence d’écriture restent inchangées. Le vecteur TLS associé couvre le record chiffré et le `close_notify` lui-même.

## Références

[1] [AOS-1833 à AOS-1840 — record TLS close_notify](aos1833_1840_tls_close_notify.md)  
[2] [RFC 5246 — TLS 1.2, §7.2.1](https://www.rfc-editor.org/rfc/rfc5246#section-7.2.1)
