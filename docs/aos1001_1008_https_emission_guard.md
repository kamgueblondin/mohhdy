# AOS-1001 à AOS-1008 — émission HTTPS conditionnée par la session TLS complète

Le chemin applicatif dispose maintenant de `net_https_build_application_record_if_ready`. Cette fonction vérifie `net_https_application_ready` avant de déléguer à `net_tls_aes_gcm_session_build`. Un handshake intermédiaire, une clé absente, un IV absent ou un pointeur nul est rejeté avant toute mutation de séquence.

L’émission reste caller-owned : le record, la session, le plaintext et les capacités sont fournis par l’appelant. Le wrapper ne copie ni secret ni payload et ne réalise aucune attente. En cas de readiness valide, la sémantique de séquence et d’erreur du builder TLS historique est conservée.

Ce lot constitue le raccord effectif entre le postflight ECDHE_ECDSA et HTTPS applicatif. Aucun `kmalloc` n’est utilisé.

Auteur : **Manus AI**
