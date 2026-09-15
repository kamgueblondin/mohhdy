# AOS-353 à AOS-360 — politique temporelle dans l’orchestrateur NE2000

`ne2k_tls_client_poll` reçoit désormais un instant UTC `YYYYMMDDhhmmssZ` fourni par l’appelant. Lors de la réception du certificat serveur, le polling appelle `x509_certificate_tls_identity_validate` : la chaîne RSA directe, le hostname DNS et la période temporelle doivent réussir avant que `peer_identity_validated` soit publié et avant toute construction du flight X25519.

La mise à jour conserve le rollback du polling : une date manquante ou invalide empêche le flight et restaure connexion, automate, transcript et compteurs. L’heure reste une dépendance caller-owned ; le pilote NE2000 ne lit ni RTC ni NTP et ne fournit donc pas lui-même une source de temps fiable.

Le test NE2000 existant fournit explicitement un instant UTC lors du polling vide, ce qui vérifie le nouveau contrat d’API. Les tests X.509 de politique couvrent ensuite le succès et les rejets chaîne/hostname/date.

> L’intégration ne prouve pas encore un handshake réel complet dans QEMU vers un serveur externe, et ne fournit pas de synchronisation d’heure sécurisée.
