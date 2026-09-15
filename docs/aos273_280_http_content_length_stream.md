# AOS-273 à AOS-280 — réponse HTTP Content-Length sur plusieurs records TLS

## Périmètre livré

Ce macro-lot étend la couche HTTP/1.1 caller-owned pour recevoir une réponse dont les en-têtes et le body peuvent être répartis sur plusieurs plaintexts TLS AES-GCM. Il se limite volontairement au framing `Content-Length`, qui apporte une terminaison déterministe et bornée sans allocation dynamique.

| Composant | Livraison |
|---|---|
| `net_http_response_accumulator_t` | Conserve buffer, longueur reçue, longueur des en-têtes, status, Content-Length attendu et état d’avancement. |
| `net_http_response_accumulator_feed` | Accumule les fragments HTTP ; retourne `1` tant que le body est incomplet, `0` lorsque le body est complet. |
| `Content-Length` | Requis pour le mode streaming ; la valeur est décimale, unique et bornée à 65 535 octets. |
| `net_http_tls_open_response_stream` | Ouvre un record TLS applicatif, alimente l’accumulateur et restaure TCP, session AEAD et état de l’accumulateur sur erreur. |
| Publication | La vue de réponse n’est publiée qu’à réception exacte de tous les octets du body. |

## Flux validé

Le test construit deux records TLS AES-GCM contenant respectivement les en-têtes `HTTP/1.1 200 OK` avec `Content-Length: 5` et les deux premiers octets du body, puis les trois derniers octets. Le premier record retourne l’état incomplet tout en conservant les séquences TCP/TLS et l’état des en-têtes. Le second publie le body complet `hello` avec le status `200`.

> Le parser rejette un `Content-Length` absent, dupliqué, non numérique, supérieur à 65 535, ainsi qu’un body dont la taille finale diffère de la longueur annoncée. Les anomalies de record TLS ou de framing HTTP restaurent les états TCP, AEAD et accumulateur qui précèdent l’appel.

## Contrat mémoire

L’appelant fournit le buffer de réponse, le buffer plaintext TLS, le contexte d’accumulation et tous les buffers transport/TLS. Les vues publiées pointent vers le buffer de l’accumulateur. Aucun chemin ne recourt à `kmalloc` ni à une autre allocation dynamique.

## Limites explicites

Le module ne prend pas encore en charge `Transfer-Encoding: chunked`, les réponses sans `Content-Length` terminées par fermeture de connexion, les trailers, la compression, HTTP/2, HTTP POST, l’authentification applicative, la pagination ni le streaming de génération LLM. Le handshake automatique de production, la chaîne X.509, les dates, le nom d’hôte et un backend X25519 constante-temps restent également absents.
