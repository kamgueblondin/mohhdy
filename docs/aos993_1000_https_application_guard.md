# AOS-993 à AOS-1000 — garde HTTPS applicatif après handshake ECDHE_ECDSA

`net_https_application_ready` est le point de synchronisation entre le handshake TLS et la couche HTTPS. Il exige l’état `NET_TLS_HANDSHAKE_SERVER_FINISHED_RECEIVED` ainsi que les pointeurs `write_key`, `write_fixed_iv`, `read_key` et `read_fixed_iv` de la session AES-GCM.

Le prédicat ne bloque pas, ne modifie aucun état et ne copie aucun secret. Les appels applicatifs peuvent le tester avant `net_tls_aes_gcm_session_build` ou `net_tls_aes_gcm_session_open`. Une session partiellement initialisée reste donc inutilisable, même si la structure de handshake a atteint un état intermédiaire.

Ce lot ferme le raccordement logique entre postflight ECDHE_ECDSA et HTTPS applicatif sans modifier les builders HTTP/LLM existants. Aucun `kmalloc` n’est utilisé.

Auteur : **Manus AI**
