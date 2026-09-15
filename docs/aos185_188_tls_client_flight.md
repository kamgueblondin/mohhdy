# AOS-185 à AOS-188 — Flight client TLS 1.2 caller-owned

AOS-185 à AOS-188 prolongent le macro-lot TLS avec le framing du flight client après `ServerHelloDone`. Les constructeurs produisent seulement les octets protocolaires dans des buffers appartenant à l’appelant; ils ne calculent aucun secret, signature ou `verify_data`.

| Lot | Élément construit ou transitionné | Statut |
|---|---|---|
| AOS-185 | `Certificate` client vide lorsque le serveur a demandé un certificat | Implémenté |
| AOS-186 | `ClientKeyExchange` avec clé publique fournie par l’appelant | Implémenté |
| AOS-187 | Record `ChangeCipherSpec` de valeur `0x01` | Implémenté |
| AOS-188 | Record Handshake `Finished` avec `verify_data` de 12 octets fourni | Implémenté |

L’automate autorise deux séquences. Lorsque le serveur a envoyé `CertificateRequest`, le client passe de `SERVER_HELLO_DONE_RECEIVED` à `CLIENT_CERTIFICATE_SENT`, puis à `CLIENT_KEY_EXCHANGE_SENT`, `CHANGE_CIPHER_SPEC_SENT` et `FINISHED_SENT`. Lorsqu’aucun certificat client n’est demandé, le passage direct vers `CLIENT_KEY_EXCHANGE_SENT` est accepté. Les transitions hors ordre sont rejetées.

Le `verify_data` de Finished est un paramètre caller-owned. Cette disposition sert à découpler le framing actuel de la future implémentation des secrets de session, du PRF TLS 1.2 et du hash du transcript. Le code ne doit donc pas être interprété comme une validation de Finished ni comme un chiffrement activé.

| Validation | Résultat |
|---|---|
| Test unitaire TLS du flight client | Validé |
| Suite Unity complète | 321/321 tests verts |
| Allocation dynamique | Absente |
| Dérivation de la clé partagée ECDHE | Non implémentée |
| PRF TLS 1.2 et `verify_data` authentique | Non implémentés |
| Chiffrement des records post-CCS | Non implémenté |
| Authentification X.509, HTTP et appels LLM sécurisés | Non fonctionnels |

Le macro-lot livre donc une représentation contrôlée de l’ordre et du framing du flight client. La prochaine étape cryptographique devra fournir l’ECDHE, la vérification du certificat serveur, la dérivation des clés de trafic et la protection AEAD des records.
