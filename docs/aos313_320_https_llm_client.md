# AOS-313 à AOS-320 — client HTTPS LLM sur NE2000

## Périmètre livré

Le pilote NE2000 fournit deux points d’entrée HTTP sécurisé destinés à un client LLM caller-owned. `ne2k_https_llm_post_json` construit un POST JSON HTTP/1.1, le chiffre dans un record TLS AES-128-GCM de la session du contexte NE2000, l’encapsule dans TCP et l’émet. `ne2k_https_llm_poll_response` reçoit un segment TCP depuis NE2000, ouvre son record TLS, puis transmet le plaintext à l’accumulateur HTTP `Content-Length` existant.

| Opération | Précondition | Résultat |
|---|---|---|
| `ne2k_https_llm_post_json` | Handshake TLS `complete`, session AES-GCM prête, cache ARP et connexion TCP établie. | POST `application/json` avec `Content-Length`, record applicatif AES-GCM et commit TCP après TX réussi. |
| `ne2k_https_llm_poll_response` | Même session TLS complète, accumulateur HTTP caller-owned. | Retourne `1` tant que la réponse est incomplète ; publie la vue body seulement à la taille Content-Length exacte. |

Les deux appels ne réalisent aucune allocation dynamique. Ils reçoivent les trames, le request buffer, le record TLS, le plaintext HTTP, l’accumulateur de réponse et les états TCP/TLS par l’appelant.

## Sécurité et transactionnalité

L’émission refuse toute requête tant que le Finished serveur n’a pas été vérifié par l’orchestrateur. Avant un POST, la séquence TCP et le compteur AEAD d’écriture sont sauvegardés ; une erreur de construction, d’ARP, de TX ou de commit les restaure. La réception applique le rollback déjà assuré par l’ouverture TLS/HTTP et restaure également le contexte NE2000 et TCP si l’ACK ne peut pas être émis.

Le test HTTP/TLS existant couvre le framing exact du POST JSON, les bornes et son round-trip AES-GCM ; le test NE2000 couvre le transport ClientHello préalable et le polling vide. Le Makefile et le runner lient explicitement HTTP/TLS au test NE2000 pour vérifier l’intégration de compilation.

> Ces wrappers sont une couche de transport applicative. Ils n’intègrent ni clé API ni choix automatique de fournisseur, et ne doivent pas être présentés comme un accès OpenAI ou Ollama opérationnel de bout en bout.

## Limites explicites

La résolution DNS, SYN/SYN-ACK, DHCP/ARP de connexion, l’alimentation d’une ancre de production, le test QEMU contre un serveur TLS externe, les clés `Authorization`, la sérialisation OpenAI/Ollama spécifique, le streaming SSE, chunked, HTTP/2, retries, pagination, compression, gros bodies fragmentés et le parsing JSON restent hors périmètre. La chaîne X.509 reste RSA à une ancre directe ; dates, intermédiaires, révocation et ECDSA manquent. X25519/bigint reste non constante-temps.
