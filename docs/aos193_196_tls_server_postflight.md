# AOS-193 à AOS-196 — Post-flight serveur TLS 1.2 et vérification Finished

AOS-193 à AOS-196 prolongent l’automate TLS avec la réception du post-flight serveur après le Finished client. Le code traite explicitement un record `ChangeCipherSpec` de valeur `0x01`, puis un message Handshake `Finished` de douze octets. Tous les buffers et le `verify_data` attendu restent fournis par l’appelant.

`net_tls_finished_parse` vérifie le type de handshake, la longueur exacte et les douze octets de `verify_data`. La comparaison agrège les différences sans sortie anticipée sur un octet particulier. L’automate n’accepte ChangeCipherSpec qu’après `FINISHED_SENT`, puis Finished uniquement après `SERVER_CHANGE_CIPHER_SPEC_RECEIVED`. L’état `SERVER_FINISHED_RECEIVED` est le seul état considéré complet par `net_tls_handshake_is_complete`.

| Lot | Fonction | Contrôle |
|---|---|---|
| AOS-193 | Parsing ChangeCipherSpec | Payload exact `0x01` |
| AOS-194 | Parsing Finished serveur | Type 20, longueur 12 et `verify_data` attendu |
| AOS-195 | Transitions post-flight | Ordre Finished client → CCS serveur → Finished serveur |
| AOS-196 | Intégration TCP transactionnelle | Rollback TCP, automate et longueur transcript en erreur |

La réception TCP du post-flight accepte un record TLS CCS ou Handshake. Après validation du Finished, celui-ci est ajouté au transcript. En cas de type de record invalide, de CCS invalide, de Finished incorrect ou de capacité transcript insuffisante, la connexion TCP, l’automate et la longueur utile du transcript sont restaurés.

| Validation | Résultat |
|---|---|
| Tests TLS ciblés | Validation CCS, Finished et états complets |
| Test TCP post-flight | CCS, Finished, transcript et rollback validés |
| Suite Unity complète | 324/324 tests verts |
| Déchiffrement TLS réel | Non implémenté |
| AEAD et protection de Finished serveur | Non implémentés |
| X.509, ECDHE et clés de trafic | Non implémentés |

> En TLS 1.2 réel, le Finished serveur arrive après activation du chiffrement négocié. Le présent lot valide uniquement un record de framing en clair et le compare à un `verify_data` calculé par l’appelant. Il ne déchiffre ni n’authentifie des records AEAD et ne rend donc pas le canal TLS sécurisé de bout en bout.
