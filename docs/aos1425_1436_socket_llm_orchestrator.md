# AOS-1425 à AOS-1436 — Orchestrateur LLM HTTP/SSE actif sur socket

## Objectif

Ce macro-lot relie les façades LLM sur socket TLS au pilote NE2000. Une session TLS déjà établie peut désormais construire et transmettre une requête Ollama ou OpenAI, puis poller une réponse HTTP ou un flux SSE, sans exposer de `net_tcp_connection_t` privé. Les séquences TCP, les nonces AEAD et les accumulateurs applicatifs restent sous contrôle transactionnel.

> L’émission TLS du registre socket engage un payload TCP avant sa transmission matérielle. Les nouvelles façades NE2000 prennent donc systématiquement un snapshot du slot et de la session AEAD pour annuler cette publication si le framing Ethernet, ARP ou TX échoue.

## Contrat livré

| Primitive | Responsabilité | Invariant de publication |
|---|---|---|
| `ne2k_socket_llm_request` | Compose JSON, HTTP et record TLS, puis transmet le segment TCP du socket. | La séquence TCP et `write_sequence` TLS sont restaurées si le TX NE2000 échoue. |
| `ne2k_socket_llm_poll_response` | Reçoit un segment TCP, ouvre le record TLS, alimente l’accumulateur HTTP et émet l’ACK. | Socket, session TLS, accumulateur, réponse et consommation sont restaurés si l’ouverture ou l’ACK échoue. |
| `ne2k_socket_llm_poll_sse` | Reçoit, déchiffre, décode un delta SSE fournisseur et émet l’ACK. | Socket, session, état SSE et longueurs de sortie restent transactionnels. |

Les trois appels ne conservent ni prompt, endpoint, bearer token, record, plaintext ni buffer de réponse. Toute mémoire est statique ou explicitement détenue par l’appelant.

## Chemin actif

Pour l’émission, la façade appelle `net_llm_socket_build_request`, qui choisit le JSON normal ou streaming, applique l’en-tête Bearer OpenAI quand requis, chiffre HTTP en TLS AES-GCM et construit le segment TCP correspondant. Le segment est ensuite enveloppé dans Ethernet/IPv4/TCP et transmis par NE2000. Un échec de transmission restaure le slot et la session tels qu’ils étaient avant l’émission.

Les deux pollers commencent par `ne2k_rx_poll_tcp`. Une réception vide retourne `1` immédiatement avec longueurs nulles. Une trame disponible est ouverte par les façades socket existantes, puis ACKée. Le poll HTTP publie les fragments ou la réponse complète dans l’accumulateur caller-owned ; le poll SSE publie seulement le delta de texte et sa longueur. Si l’ACK échoue après une ouverture valide, tous les états publiables sont restaurés.

## Validation

Le nouveau test Unity NE2000 vérifie une requête Ollama chiffrée effectivement encapsulée, la progression de la séquence TLS après succès, la restauration de la séquence TCP et de `write_sequence` après un TX volontairement indisponible, ainsi que le caractère non bloquant des pollers HTTP et SSE sans trame reçue.

| Contrôle | Résultat |
|---|---|
| Compilation i386 `make all` | Réussie |
| Suite complète `make test-all` | 441/441 tests verts |
| `git diff --check` | Propre |
| Recherche d’allocation dynamique dans les nouveaux chemins | Aucune occurrence |

La règle Makefile et le runner de tests lient tous deux `net_llm_socket.c` au test NE2000, ce qui préserve la parité des validations locales et CI.

## Limites restantes

Le macro-lot fournit l’orchestration active à partir d’une session TLS complète. Il reste à unifier cet enchaînement avec le contexte réseau persistant DHCP → DNS → SYN → ClientHello → polling TLS, à déclencher l’ensemble depuis la surface de contrôle noyau et à provisionner les identifiants OpenAI de manière sécurisée. Les timeouts, backoff, reconnexion SSE, fermeture `close_notify` et réutilisation de connexion sous une politique réseau réelle restent des travaux distincts.

## Références

[1]: aos1413_1424_socket_tls_poll.md "Polling TLS authentifié sur API socket"
[2]: aos1373_1384_llm_socket_http_sse.md "Réception HTTP et SSE LLM sur sockets TLS"
[3]: aos1353_1364_llm_socket_ne2k_bridge.md "Construction LLM socket TLS et pont NE2000"

[1] [2] [3]
