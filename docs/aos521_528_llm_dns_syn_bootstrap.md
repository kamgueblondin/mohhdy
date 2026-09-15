# AOS-521 à AOS-528 — Bootstrap DNS, ARP et SYN pour hôtes LLM

**Auteur :** Manus AI  
**Statut :** implémenté et validé localement  
**Périmètre :** DNS A, ARP IPv4, TCP SYN, NE2000, préconnexion LLM

## Objectif

Ce macro-lot ajoute une façade de préconnexion dédiée aux hôtes LLM. Elle orchestre, dans un seul appel borné, l’émission d’une requête DNS de type A, l’attente de sa réponse, la résolution ARP de l’IPv4 obtenue et l’émission du premier SYN TCP.

> Le type DNS `A` publie l’adresse IPv4 d’un hôte [1]. Cette résolution ne constitue pas une authentification de l’hôte : l’identité continue d’être contrôlée plus tard par la validation X.509/TLS.

L’API `ne2k_llm_dns_syn_bootstrap` laisse la connexion dans `SYN_SENT` après succès. La réception et la validation du SYN-ACK restent explicitement séparées et emploient les primitives TCP existantes.

## Séquence contrôlée

| Étape | Primitive réutilisée | Condition de succès |
|---|---|---|
| 1 | `ne2k_dns_query` | La requête DNS A est construite et transmise via l’ARP du serveur DNS. |
| 2 | `ne2k_dns_poll_a` | Une réponse DNS A portant l’identifiant attendu est reçue dans le budget fourni. |
| 3 | `ne2k_arp_resolve` | L’IPv4 résolue possède une MAC dans le cache ARP. |
| 4 | `net_tcp_connection_open` | Une nouvelle connexion caller-owned est préparée en `SYN_SENT`. |
| 5 | `ne2k_tcp_syn` | Le SYN IPv4/TCP est émis vers la MAC de l’hôte résolu. |
| 6 | Publication | L’IPv4 et la connexion ne sont publiées qu’après toutes les étapes précédentes. |

## Contrat transactionnel et mémoire

| Propriété | Garantie |
|---|---|
| Allocation dynamique | Aucune. |
| Buffers | Requête ARP, réception ARP, frame Ethernet et IPv4 résolue sont caller-owned. |
| Budgets | Les tentatives DNS et ARP doivent être non nulles et sont transmises aux primitives bornées. |
| Publication de l’IPv4 | Écrite seulement après l’émission SYN réussie. |
| Publication de TCP | `net_tcp_connection_t` copié seulement après succès complet. |
| Échec | Tout statut négatif ou attente DNS laisse les sorties `remote_ip` et `connection` inchangées. |

Le chemin ne stocke aucun hostname, aucune adresse distante ni aucun cache propre à l’orchestrateur. Il emploie le cache ARP caller-owned déjà fourni au pilote. Cette propriété évite le partage d’état caché entre requêtes LLM successives.

## Tests et validation locale

Le test NE2000 couvre l’interdiction d’un budget DNS nul et l’absence de publication lorsque l’émission DNS est suivie d’une attente sans réponse. Les primitives DNS, ARP et SYN individuelles disposent déjà de leurs propres vecteurs de framing et de transition ; le nouveau test vérifie la propriété transactionnelle de leur composition.

| Vérification | Résultat |
|---|---|
| Suite noyau | **32/32** exécutables réussis. |
| Suite complète | **372/372** tests réussis. |
| Compilation freestanding i386 | Réussie. |
| Smoke QEMU `qemu-ai-provider` | Réussi. |
| Smoke QEMU `qemu-ne2k-status` | Réussi au second lancement ; le premier a subi une absence transitoire de détection NIC QEMU. |

## Limites connues

Le bootstrap ne réalise pas de DNS AAAA, CNAME, DNSSEC, cache DNS, renouvellement TTL, recherche de plusieurs adresses, DHCP, routage, reconnexion SYN, timeout avec backoff, ni acceptation automatique du SYN-ACK. Le DNS et l’ARP ne remplacent jamais l’authentification de serveur TLS. L’orchestrateur n’inclut pas le début du handshake TLS : l’appelant doit accepter le SYN-ACK puis lancer le client TLS existant.

## Références

[1] [RFC 1035 — Domain Names, implementation and specification](https://www.ietf.org/rfc/rfc1035.html)  
[2] [AOS-505 à AOS-512 — récupération transactionnelle TCP/TLS](aos505_512_tcp_tls_connection_retry.md)
