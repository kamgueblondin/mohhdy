# AOS-1009 à AOS-1016 — réception HTTPS conditionnée par la session TLS complète

`net_https_open_application_record_if_ready` protège l’ouverture des records applicatifs entrants. Il réutilise `net_https_application_ready`, puis délègue à `net_tls_aes_gcm_session_open` lorsque le handshake est terminé et que les clés et IV de lecture et d’écriture sont présents.

Le wrapper refuse une session partielle sans modifier la séquence de lecture. Les buffers de record, plaintext et vue sont fournis par l’appelant ; aucun secret ou payload n’est copié par cette couche et aucune attente bloquante n’est introduite.

Le chemin HTTPS possède ainsi une barrière symétrique pour émission et réception après le postflight ECDHE_ECDSA. Aucun `kmalloc` n’est utilisé.

Auteur : **Manus AI**
