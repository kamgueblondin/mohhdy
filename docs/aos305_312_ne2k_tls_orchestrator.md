# AOS-305 à AOS-312 — orchestrateur TLS depuis NE2000

## Périmètre livré

Le pilote NE2000 expose désormais un contexte `ne2k_tls_client_t` entièrement caller-owned et deux opérations de haut niveau : `ne2k_tls_client_start` pour émettre le ClientHello sur une connexion TCP établie, puis `ne2k_tls_client_poll` pour traiter les segments TLS reçus par NE2000.

| Étape | Opération fournie |
|---|---|
| Initialisation | `ne2k_tls_client_init` initialise le réassembleur TCP/TLS, le transcript, le contexte X25519, les secrets et la session AEAD avec des buffers fournis par l’appelant. |
| ClientHello | `ne2k_tls_client_start` construit le record TLS, note l’état/transcript, suit le payload TCP, l’émet par NE2000 puis avance la séquence seulement après soumission réussie. |
| Messages serveur | `ne2k_tls_client_poll` utilise le réassembleur authentifié TCP/TLS existant ; les fragments incomplets sont conservés et renvoient `1`. |
| Identité serveur | Après `Certificate`, la feuille doit valider la chaîne RSA à une ancre fournie et correspondre au hostname demandé avant le flight. |
| Flight client | Après `ServerHelloDone`, le wrapper construit le flight X25519 (ClientKeyExchange, CCS, Finished), l’émet sur NE2000 et confirme la séquence TCP. |
| Post-flight | Le même polling accepte CCS puis Finished serveur AES-GCM ; le contexte indique `complete` lorsque le handshake est achevé. |

## Transactionnalité et mémoire

Les structures TCP et TLS sont sauvegardées avant toute réception qui peut échouer. Une erreur de framing, de signature, de chaîne, de hostname, de flight ou de post-flight restaure connexion, automate, transcript, contextes X25519/AEAD et longueurs publiées. Le TX ClientHello maintient également une copie de travail : l’état ne progresse qu’après soumission NE2000 et commit TCP.

Aucun buffer n’est alloué : l’appelant fournit les buffers records, handshake, transcript, trames RX/TX, segment TCP, flight, plaintext ainsi que les workspaces RSA, X25519 et PRF.

## Validation

Le test NE2000 initialise une connexion TCP établie et son cache ARP, initialise l’orchestrateur, émet un ClientHello, contrôle le record TLS à l’emplacement TCP attendu, l’avancement de `local_sequence`, le transcript et l’état `CLIENT_HELLO_SENT`. Il couvre également un polling NE2000 vide : aucun octet, flight ou état TLS n’est publié/modifié. Les tests TCP/TLS existants couvrent séparément le réassemblage authentifié, la construction transactionnelle du flight X25519, le rollback d’authentification et le post-flight AES-GCM.

> Le nouveau wrapper coordonne les primitives disponibles ; il ne transforme pas encore le système en client HTTPS de production autonome.

## Limites explicites

Le polling accepte encore un message TLS serveur par record au niveau du réassembleur existant. Les certificats clients sont refusés car le flight X25519 actuel ne les construit pas. La chaîne reste limitée à une ancre RSA directe, sans intermédiaires, dates, usages, révocation ni ECDSA. L’exemple d’intégration ne couvre pas un échange réseau réel complet dans QEMU contre un serveur TLS externe, la résolution/connexion end-to-end, HTTP LLM, l’authentification API, les retries et la fragmentation des grandes requêtes. X25519/bigint reste non constante-temps.
