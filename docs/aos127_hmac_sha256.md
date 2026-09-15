# AOS-127 — HMAC-SHA-256 caller-owned

Le lot AOS-127 complète la primitive SHA-256 par HMAC-SHA-256. Les clés supérieures à 64 octets sont d’abord condensées, puis les pads interne et externe sont traités via le contexte SHA-256 existant. Les buffers temporaires sont locaux à l’appel; aucune allocation dynamique et aucun secret persistant ne sont introduits.

Le test couvre les vecteurs SHA-256 de la chaîne vide et de `abc`, ainsi que le vecteur RFC 4231 `key=0b…0b`, message `Hi There`. La validation locale est de **283 tests verts**, avec build i386 et initrd réussis. Cette brique prépare TLS mais ne constitue pas encore une implémentation X.509 ou HTTPS.
