# AOS-1413 à AOS-1424 — Polling TLS authentifié sur API socket

## Objectif

Ce macro-lot retire du parcours TLS actif la dépendance restante à une instance publique de `net_tcp_connection_t`. Après le démarrage du ClientHello sur socket, le pilote NE2000 reçoit seulement l’identifiant du slot TCP statique, le contexte TLS et les buffers déjà détenus par l’appelant. Il peut ainsi absorber les fragments serveur, authentifier l’identité X.509, transmettre les ACK, construire le flight X25519 et terminer le post-flight `ChangeCipherSpec`/`Finished` sans allocation dynamique.

> Le slot du registre socket est l’unique autorité sur les numéros de séquence TCP. Les buffers de record, transcript, RSA, X25519, PRF, plaintext et flight restent fournis et dimensionnés par l’appelant.

## Contrat livré

| Primitive | Responsabilité | Publication et rollback |
|---|---|---|
| `net_socket_build_ack` | Sérialise un ACK depuis le slot TCP sélectionné. | Échec borné si le socket, le buffer ou la capacité sont invalides. |
| `net_socket_commit_send` | Confirme l’envoi d’un payload TCP déjà construit et transmis. | Refuse un commit sans émission pending. |
| `net_socket_accept_tls_authenticated_fragment` | Délègue au slot la réception TLS authentifiée et la progression TCP. | Les erreurs de protocole sont normalisées en erreur socket. |
| `net_socket_build_tls_x25519_flight` | Prépare le segment TCP et les records de flight X25519 depuis le slot. | Les structures TLS et TCP ne sont publiées qu’après succès de la transaction appelante. |
| `net_socket_accept_tls_x25519_postflight` | Accepte `ChangeCipherSpec` puis `Finished` serveur via le slot. | Les échecs TLS restent transactionnels. |
| `ne2k_socket_ack` | Encapsule et transmet un ACK du slot sous Ethernet/IPv4/TCP. | Échec ARP, framing ou TX propagé. |
| `ne2k_socket_tls_poll` | Ordonne RX TCP, TLS authentifié, X.509, ACK, flight et post-flight. | Restaure le slot TCP et le client TLS sur toute erreur après réception. |

## Séquence d’exécution

Le poller commence par une réception TCP NE2000 non bloquante. Une réception vide retourne `1` sans mutation. Lorsqu’une trame est disponible, il prend un snapshot du slot socket et du contexte TLS. Pendant la phase serveur initiale, il accepte un fragment authentifié puis émet immédiatement son ACK. Une identité de certificat est validée contre l’ancre fournie avec l’algorithme approprié : feuille directe, chaîne à un intermédiaire ou chaîne à deux intermédiaires.

Lorsque `ServerHelloDone` est atteint, que l’identité est validée et qu’aucun certificat client n’est demandé, le poller construit le flight X25519, le transmet comme segment TCP déjà sérialisé, puis confirme sa progression au moyen de `net_socket_commit_send`. Pendant le post-flight, les records `ChangeCipherSpec` et `Finished` sont ouverts via l’API socket, acquittés sur NE2000 et la marque `client.complete` est publiée uniquement après la fin effective du handshake.

En cas d’échec de parsing, de validation X.509, de construction du flight, d’encapsulation ou de transmission, `ne2k_socket_tls_poll` restaure le snapshot TCP et le contexte TLS, annule les longueurs `consumed` et `flight_records_length`, puis retourne une erreur négative. Les pointeurs et capacités de buffers caller-owned ne sont jamais modifiés.

## Validation

Les nouveaux vecteurs Unity couvrent l’ACK sérialisé par socket, les gardes des primitives de progression TLS, le refus d’un commit sans payload pending, la non-régression de l’état TCP, l’encapsulation ACK par NE2000 et le polling non bloquant sans trame. Le test du chemin historique conserve les vecteurs de fragments serveur invalides et de rollback TLS ; le nouveau poller réutilise strictement les mêmes primitives transactionnelles.

| Contrôle | Résultat |
|---|---|
| Compilation i386 `make all` | Réussie |
| Suite noyau | 36/36 exécutables verts |
| Suite complète `make test-all` | 440/440 tests verts |
| Recherche `kmalloc`, `malloc`, `calloc`, `realloc` dans les chemins modifiés | Aucune occurrence |

## Limites restantes

La migration couvre le polling TLS authentifié sur socket, mais l’orchestrateur actif de bout en bout reste à raccorder complètement à l’API socket : acquisition DHCP, DNS, SYN, SYN-ACK, ClientHello, poller TLS puis requête HTTP ou SSE depuis un unique contexte de session. Le provisionnement de configuration fournisseur, les timeouts/backoff périodiques, la fermeture TLS `close_notify` et les politiques de reconnexion restent également des étapes distinctes.

## Références

[1]: aos1401_1412_socket_tls_clienthello.md "ClientHello TLS actif sur socket"
[2]: aos1373_1384_llm_socket_http_sse.md "Réception HTTP et SSE LLM sur sockets TLS"
[3]: aos1385_1392_ne2k_socket_rx_bridge.md "Injection TCP NE2000 dans le registre socket"

[1] [2] [3]
