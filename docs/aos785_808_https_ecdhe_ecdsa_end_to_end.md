# AOS-785 à AOS-808 — Transport HTTPS ECDHE_ECDSA de bout en bout

**Statut : implémenté et validé.** Ce macro-lot élargit la couverture du handshake ECDHE_ECDSA au transport TCP et à la première requête HTTPS applicative. Il ne crée pas une seconde implémentation TLS : il exerce le même chemin caller-owned jusqu’au postflight serveur, au premier record applicatif et au POST JSON LLM.

## Parcours couvert

| Étape | Contrôle |
|---|---|
| Handshake | Suite `TLS_ECDHE_ECDSA_WITH_AES_128_GCM_SHA256`, flight X25519 client et `Finished` serveur AES-GCM. |
| TCP | Encapsulation du flight dans un segment, vérification des séquences, ACK et fenêtres. |
| Postflight | Réception `ChangeCipherSpec` puis `Finished` serveur authentifié, avec séquences AEAD correctes. |
| Application | Premier record applicatif chiffré/déchiffré après passage à l’état TLS complet. |
| HTTPS LLM | Construction d’un POST Ollama JSON avec `Host`, chemin `/api/generate`, `Content-Type` et `Content-Length`, puis chiffrement AES-GCM et ouverture côté serveur simulé. |
| Mémoire | Tous les buffers restent bornés, statiques ou caller-owned ; aucune allocation dynamique n’est ajoutée. |

Le test conserve le chemin RSA/X25519 historique et ajoute explicitement sa variante ECDHE_ECDSA. Le certificat ECDSA et la signature `ServerKeyExchange` sont validés dans les tests TLS précédents ; ce lot vérifie que la suite négociée ne disparaît pas lors du transport TCP, du postflight et de la session applicative.

## Garanties transactionnelles

Le record `Finished` serveur est ouvert avant la publication de l’état TLS complet. Une authentification AEAD invalide restaure la session, le compteur de lecture, l’état du handshake et le transcript. Le premier record applicatif suit le même contrat : il doit correspondre à la séquence TCP attendue, être entièrement parsable comme record TLS et être authentifié avec le nonce dérivé de la séquence courante.

Le test a également corrigé une ambiguïté de fixture : `net_tcp_connection_commit_send` reçoit la **longueur du payload TCP**, et non la longueur totale du segment incluant les 20 octets d’en-tête TCP. Cette distinction est essentielle pour que le POST suivant commence à la même séquence que celle attendue par le serveur.

## POST JSON LLM

Le body Ollama non-streaming est construit avec le modèle `tinyllama` et le prompt `hi`, puis encapsulé dans un POST HTTP/1.1 vers `/api/generate`. Le test vérifie que le serveur simulé ouvre le record AES-GCM et retrouve le préfixe `POST` ainsi que la longueur exacte du plaintext. Aucun token, endpoint réel ou credential n’est introduit dans le test.

## Validation

| Vérification | Résultat |
|---|---|
| `test_net_tcp` ciblé | **18/18** scénarios passés, dont le nouveau ECDHE_ECDSA + POST HTTPS. |
| `make test-build` | Image i386 freestanding compilée avec succès. |
| `make test-all` | **395/395** tests passés. |
| `make qemu-ai-provider` | Smoke fournisseur IA réussi. |
| `make qemu-ne2k-status` | Smoke NE2000 réussi. |

Le lanceur global et la règle Make du test TCP lient maintenant `net_http_tls.c` lorsque le test exerce les builders HTTP/LLM. Cette dépendance est limitée au binaire concerné et ne modifie pas les autres règles de test.

## Limites

Ce lot valide le transport HTTPS sur serveur simulé dans le harness Unity. Il ne prétend pas constituer un test réseau externe contre OpenAI ou Ollama : la résolution DNS, DHCP, ARP, SYN/SYN-ACK et l’orchestration NE2000 restent couverts par leurs façades et smokes dédiés. Les prochains travaux portent sur l’intégration de la session ECDHE_ECDSA au chemin NE2000 réellement alimenté par des records reçus, puis sur la robustesse applicative et la disponibilité d’un endpoint configuré.
