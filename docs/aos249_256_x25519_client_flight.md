# AOS-249 à AOS-256 — Flight client X25519 transactionnel

## Objet du macro-lot

Ce macro-lot relie le contexte X25519 au flight client TLS 1.2 réellement émis. L’API `net_tls_x25519_client_flight_build` construit, dans un buffer caller-owned, le `ClientKeyExchange`, le `ChangeCipherSpec` puis le `Finished` chiffré AES-128-GCM. Elle met en cohérence le transcript, l’automate de handshake, le master secret, le bloc de clés TLS et la session AEAD cliente.

| Élément | Résultat |
|---|---|
| ClientKeyExchange | Clé publique X25519 incluse dans un record TLS Handshake et ajoutée au transcript. |
| Master secret | Dérivé depuis le secret partagé X25519 et les randoms client/serveur. |
| ChangeCipherSpec | Émis en clair, puis transition d’état contrôlée. |
| Finished client | Calculé sur le transcript incluant ClientKeyExchange, ajouté au transcript puis chiffré avec la séquence AEAD 0. |
| Session AEAD | Rattachée au key block caller-owned ; sa séquence d’écriture vaut 1 après l’émission du Finished. |
| Atomicité | État de handshake, contexte X25519, longueur du transcript et longueur publiée du flight sont restaurés ou annulés en cas d’erreur. |

## Contrat mémoire

L’appelant fournit tous les buffers : transcript, sortie de records, master secret, key block, workspace X25519 et workspace PRF. La taille minimale de la sortie de records est `93` octets : 42 octets pour ClientKeyExchange, 6 octets pour ChangeCipherSpec et 45 octets pour le record Finished protégé.

> La fonction publie `records_length = 0` avant tout contrôle. Si une précondition, une capacité ou une primitive échoue, aucun flight partiel n’est déclaré publiable et les états internes manipulés sont restaurés.

## Préconditions

Le chemin cible exclusivement `TLS_ECDHE_RSA_WITH_AES_128_GCM_SHA256` (`0xC02F`) avec X25519. Le handshake doit être à l’état `SERVER_HELLO_DONE_RECEIVED`, disposer de la clé éphémère serveur X25519 et ne pas demander de certificat client. Les scénarios avec authentification client restent volontairement refusés pour éviter la construction d’un flight incomplet.

## Tests

Le test d’intégration construit un handshake X25519 prêt au flight, vérifie les trois records et le transcript, déchiffre le Finished avec une session serveur correspondante, confirme l’avancement de la séquence AEAD et vérifie le rollback lors d’une capacité de sortie insuffisante. La validation de soumission produit **341/341 tests réussis**, un build i386 freestanding réussi et les smoke tests `qemu-ai-provider` et `qemu-ne2k-status` réussis.

## Limites explicites

Le flight est construit localement mais son envoi TCP, la réception et la validation du `ChangeCipherSpec`/`Finished` serveur dans ce même contexte, la chaîne de confiance X.509, l’authentification d’hôte, les certificats clients, HTTP sécurisé et les appels LLM HTTPS de bout en bout ne sont pas encore intégrés. Le backend X25519 fondé sur bigint reste fonctionnel mais non constante-temps ; il ne doit pas protéger des secrets dans un environnement hostile avant son remplacement par un backend de champ auditée.
