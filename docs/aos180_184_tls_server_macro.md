# AOS-180 à AOS-184 — Macro-lot messages serveur TLS et transcript caller-owned

Ce macro-lot accélère le traitement du handshake TLS 1.2 en regroupant le framing des messages de handshake, l’assemblage de fragments, la conservation du transcript, le parsing des messages serveur ECDHE et l’intégration transactionnelle avec TCP. Les structures de travail sont toutes fournies par l’appelant : aucun `kmalloc`, buffer global ou copie implicite n’est introduit.

Le codec `net_tls_handshake_parse` valide l’en-tête de quatre octets et publie une vue sans copie du corps. L’accumulateur `net_tls_handshake_accumulator_t` absorbe des fragments dans un buffer borné fourni par l’appelant et ne publie le message qu’une fois intégralement disponible. Il rejette un message dont la taille dépasse la capacité ou dont les fragments excèdent la longueur annoncée.

Le transcript `net_tls_transcript_t` conserve les messages de handshake exacts dans un buffer caller-owned. Chaque ajout vérifie le framing avant la copie et échoue sans modifier la longueur lorsqu’il n’y a pas de place. Cette primitive prépare le calcul futur de `Finished`, sans encore calculer de hash ni de MAC TLS.

| Lot | Portée livrée | Contrat mémoire |
|---|---|---|
| AOS-180 | Vue générique de handshake et accumulateur de fragments | Buffer et vue fournis par l’appelant |
| AOS-181 | Transcript ordonné et borné | Copie explicite dans le buffer caller-owned |
| AOS-182 | Parseur `ServerKeyExchange` ECDHE nommé | Clé publique et signature publiées sans copie |
| AOS-183 | Parseur `CertificateRequest` TLS 1.2 | Vecteurs de types, algorithmes et autorités sans copie |
| AOS-184 | Dispatch serveur et intégration transactionnelle TCP | Restauration TCP si record ou handshake invalide |

`ServerKeyExchange` contrôle le type de courbe nommé, l’identifiant de courbe, la longueur de la clé publique, les algorithmes de signature et la longueur de signature. L’automate conserve la courbe et la vue de clé publique, puis passe à `SERVER_KEY_EXCHANGE_RECEIVED`. `CertificateRequest` accepte les vecteurs TLS 1.2 bornés et fait passer l’automate à `CERTIFICATE_REQUEST_RECEIVED`, en indiquant explicitement qu’un certificat client a été demandé.

La fonction `net_tls_handshake_accept_server_message` concentre le dispatch des messages `ServerHello`, `Certificate`, `ServerKeyExchange`, `CertificateRequest` et `ServerHelloDone`. Si le transcript ne peut pas accepter le message, l’état du handshake est restauré. L’intégration `net_tcp_connection_accept_tls_handshake` applique la même propriété au transport : en cas de type TLS non-handshake ou de message serveur invalide, la séquence TCP et la fenêtre de réception sont restaurées.

| Validation | Résultat |
|---|---|
| Tests TLS ciblés | 13/13 verts |
| Test TCP de transaction TLS | Ajouté et validé |
| Suite Unity complète | 320/320 verts |
| Allocation dynamique dans ce macro-lot | Absente |
| Validation X.509 et signature ServerKeyExchange | Non implémentées |
| ECDHE, dérivation de clés, chiffrement et `Finished` | Non implémentés |
| HTTP et appels LLM sécurisés de bout en bout | Non fonctionnels |

Ce macro-lot établit un framing de handshake complet côté serveur et un transcript exploitable pour la suite cryptographique. Il ne doit toutefois pas être présenté comme une connexion TLS sécurisée tant que les signatures, les certificats, l’accord de clés, le PRF et le chiffrement des records ne sont pas implémentés.
