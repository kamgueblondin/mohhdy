# AOS-1513 à AOS-1520 — reprise SSE fine avec `Last-Event-ID`

## Objectif

Ce lot raccorde au noyau la reprise fine d’un flux SSE interrompu par une récupération réseau. Lorsqu’une session LLM streaming possède un identifiant SSE valide et qu’un bail DHCP expire, l’orchestrateur conserve cet identifiant dans un état statique borné avant de fermer le socket. Après réacquisition DHCP, bootstrap DNS/ARP/SYN et handshake TLS authentifié complet, il émet un `GET` HTTP/1.1 de reprise avec l’en-tête `Last-Event-ID` au lieu de rejouer le `POST` LLM initial.

## État et propriété mémoire

`kernel_llm_application_recovery_t` contient désormais le drapeau `is_sse_resume`, une longueur et un tableau de `NET_LLM_SSE_EVENT_ID_MAX` octets. Cet état appartient au noyau et ne contient aucun bearer, secret TLS ni pointeur utilisateur. L’identifiant est copié uniquement si la requête mémorisée est streaming et si l’accumulateur SSE fournit un identifiant valide de taille bornée.

| Élément | Propriété | Garantie |
|---|---|---|
| `event_id` | tableau fixe noyau | au plus 32 octets, aucune allocation |
| `is_sse_resume` | drapeau explicite | différencie GET de reprise et POST initial |
| `boot_llm_sse_response` | buffers HTTP/SSE fixes | réinitialisé puis réhydraté après TLS |
| bearer fournisseur | stockage noyau existant | conservé par la récupération automatique, jamais sérialisé dans la reprise SSE |

La fermeture explicite continue d’effacer entièrement le contexte applicatif, y compris l’identifiant. La fermeture automatique de récupération conserve uniquement le contexte fournisseur et l’état applicatif nécessaire à la reprise, conformément au contrat introduit par AOS-1505 à AOS-1512.

## Chemin d’émission

Le module `net_llm_socket` expose `net_llm_socket_build_sse_resume`. Il appelle le builder HTTP borné `net_llm_sse_build_resume_get`, puis chiffre le plaintext avec `net_socket_send_tls`. Le pilote NE2000 ajoute deux façades : `ne2k_socket_llm_resume_sse` prend un snapshot TCP et TLS avant l’envoi matériel, tandis que `ne2k_llm_socket_session_resume_sse` ne publie `REQUEST_SENT` qu’après succès intégral.

> Toute erreur de construction HTTP, de chiffrement, de construction TCP ou de transmission NE2000 restaure la connexion TCP et la session TLS précédentes. La phase de session reste alors `TLS_COMPLETE`.

À l’achèvement TLS, `kernel_llm_poll_tls` choisit donc deux voies exclusives. Sans identifiant SSE mémorisé, il appelle `kernel_llm_request` et conserve la réémission POST existante. Avec un identifiant valide, il réinitialise les accumulateurs dans leurs buffers déjà réservés, restaure cet identifiant, puis appelle la façade de reprise SSE. Le fournisseur et le mode streaming sont publiés seulement après succès.

## Validation

Un test Unity ajouté à `test_net_llm_socket` vérifie la construction de `GET /v1/chat`, l’en-tête `Last-Event-ID: evt-42`, l’encapsulation TLS/TCP et le rejet sans identifiant. La compilation i386 et les suites ciblées `test_net_llm_socket` et `test_ne2k` sont exécutées avant la validation complète.

## Limites assumées

Cette reprise dépend de la sémantique SSE du serveur distant : un endpoint LLM qui ne propose pas de reprise `Last-Event-ID` peut répondre comme à un GET HTTP générique. Le lot ne modifie ni le protocole OpenAI ni le contrat Ollama distant. Une campagne d’intégration contre des endpoints réels, puis un client OpenAI effectif et la négociation explicite des capacités de reprise, restent les prochains axes.

Aucune allocation dynamique, aucun `kmalloc` et aucun travail réseau dans IRQ0 ne sont introduits.
