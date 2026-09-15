# AOS-633 à AOS-640 — Bootstrap LLM DNS/TCP routé depuis DHCP

**Auteur :** Manus AI  
**Statut :** implémenté et validé localement  
**Périmètre :** bail DHCP caller-owned, DNS, ARP, TCP SYN, état de session LLM

## Objectif

Ce macro-lot transforme les informations DHCP déjà obtenues en un bootstrap réseau LLM routé. La nouvelle façade sélectionne d’abord le prochain saut du DNS depuis le masque et le routeur du bail, résout le nom du fournisseur, sélectionne ensuite le prochain saut de l’IPv4 obtenue, puis envoie un SYN TCP vers cette destination via la MAC de passerelle appropriée.

> DNS et TCP conservent leur destination IPv4 logique ; ARP ne résout que le prochain saut Ethernet nécessaire au chemin local.

## API ajoutées

| API | Rôle |
|---|---|
| `ne2k_tcp_syn_via` | Émet un SYN vers l’hôte IPv4 cible, en résolvant la MAC du prochain saut avec un nombre d’essais ARP borné. |
| `ne2k_llm_dns_syn_bootstrap_dhcp` | Orchestration transactionnelle `bail DHCP → prochain saut DNS → DNS A → prochain saut hôte → SYN`. |
| `ne2k_llm_connection_start_dhcp` | Façade de session qui publie `SYN_SENT` uniquement après le bootstrap complet. |

## Flux transactionnel

| Étape | Donnée utilisée | Effet publié uniquement au succès |
|---|---|---|
| Sélection de route DNS | `lease->dns_ipv4`, masque, routeur | Aucun état de session. |
| Requête DNS routée | DNS destination + prochain saut | Aucune IPv4 LLM publiée. |
| Polling DNS A | Réponse DNS authentifiée par ID | IPv4 conservée localement. |
| Sélection de route hôte | IPv4 LLM résolue + bail | Connexion inchangée. |
| SYN routé | IP hôte + MAC prochain saut | Connexion TCP temporaire. |
| Publication | `remote_ip`, `connection`, phase | `SYN_SENT` seulement à la fin. |

Les erreurs des étapes DHCP, DNS, ARP ou SYN n’écrivent ni l’IPv4 distante caller-owned, ni la connexion TCP caller-owned, ni la phase LLM. Les budgets de polling DNS et ARP restent fournis explicitement par l’appelant.

## Séparation Ethernet et IPv4

`ne2k_tcp_syn_via` suit le même contrat que l’UDP routé : la MAC Ethernet est cherchée pour `next_hop_ip`, mais le paquet IPv4 généré par `net_tcp_build_syn_ipv4` porte toujours `remote_ip`. La couche TLS ultérieure reçoit donc l’IPv4 du fournisseur, non celle de la passerelle.

## Tests et validation locale

Les tests NE2000 ajoutent un SYN vers `1.1.1.1` routé via une passerelle `10.0.2.2` en cache ARP. Ils vérifient la MAC Ethernet de la passerelle et les quatre octets de destination IPv4 distante. Un test de garde de `ne2k_llm_connection_start_dhcp` confirme que le pointeur de bail invalide ne modifie ni la phase `IDLE`, ni l’IPv4 de session, ni la séquence TCP.

| Vérification | Résultat |
|---|---|
| Test NE2000 ciblé | **19/19** réussis. |
| Suite complète | **382/382** tests réussis. |
| Compilation freestanding i386 | Réussie. |
| Smoke QEMU fournisseur | Réussi. |
| Smoke QEMU NE2000 | Réussi. |

## Limites connues

Cette livraison requiert un bail DHCP déjà obtenu ; elle ne compose pas encore l’acquisition DHCP et le démarrage LLM dans un unique appel. Le provisioning de l’interface noyau, les syscalls de démarrage/polling LLM, la configuration d’endpoint et d’identifiants hors image, TLS live, timeout/retry, fermeture de session, outils, multimodal et Unicode complet restent hors périmètre.

## Références

[1] [AOS-625 à AOS-632 — UDP et DNS routés par prochain saut](aos625_632_routed_dns_next_hop.md)  
[2] [AOS-617 à AOS-624 — masque DHCP et prochain saut réseau](aos617_624_dhcp_next_hop.md)  
[3] [AOS-537 à AOS-544 — orchestrateur LLM DNS vers ClientHello](aos537_544_llm_connection_orchestrator.md)
