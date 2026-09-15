# AOS-281 à AOS-288 — HTTP POST JSON borné sur TLS

## Périmètre livré

Ce macro-lot apporte l’émission d’une requête HTTP/1.1 `POST` contenant un body JSON fourni par l’appelant. Le constructeur écrit tous les octets dans un buffer de requête caller-owned, calcule sa longueur au format décimal et fixe les en-têtes nécessaires à un échange HTTP/1.1 minimal.

| Élément | Garantie livrée |
|---|---|
| Méthode et cible | `POST <path> HTTP/1.1`, avec chemin obligatoire commençant par `/`. |
| En-têtes | `Host`, `Content-Type: application/json`, `Content-Length` et `Connection: close`. |
| Longueur du body | Valeur décimale de `0` à `65 535`, calculée à partir de `json_length`. |
| Corps | Suite d’octets caller-owned, copiée sans terminaison implicite ni allocation. |
| Transport | `net_http_tls_build_post_json` chiffre le plaintext HTTP dans un record applicatif TLS AES-GCM puis l’encapsule dans TCP. |
| Erreurs | Les pointeurs requis, les chemins invalides et les insuffisances de capacité sont rejetés avant l’envoi. |

## Validation appliquée

Les tests unitaires vérifient la requête complète destinée à une API de complétions, y compris `Content-Length: 30` pour le JSON de test. Ils couvrent également un buffer trop petit, un chemin sans `/` initial, un body non nul absent et le round-trip réel : POST construit, chiffré AES-GCM, encapsulé TCP, parsé et déchiffré côté serveur simulé. Le plaintext déchiffré est comparé octet pour octet à la requête attendue.

> Le module construit un framing HTTP sûr et borné ; il ne valide pas la syntaxe ou la sémantique du document JSON. Celle-ci reste sous la responsabilité de l’appelant qui fournit le body et sa taille.

## Contrat mémoire

Les buffers de requête HTTP, record TLS, segment TCP et plaintext de vérification appartiennent tous à l’appelant. Les fonctions n’allouent pas de mémoire dynamique et n’utilisent pas `kmalloc`.

## Limites explicites

Le POST est livré comme un record applicatif TLS unique ; il ne fragmente pas encore une très grande requête HTTP sur plusieurs records TLS ou segments TCP. `Transfer-Encoding: chunked`, les réponses sans `Content-Length`, HTTP/2, les en-têtes d’autorisation, Bearer/API keys, OAuth, les certificats clients, la compression, les retries applicatifs et le streaming de génération ne sont pas fournis. Le handshake de production déclenché depuis le pilote, la validation du nom d’hôte, la chaîne de confiance X.509, les dates et le backend X25519 constante-temps restent hors périmètre.
