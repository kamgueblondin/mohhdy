# AOS-641 à AOS-648 — Façade unifiée DHCP vers session LLM

**Auteur :** Manus AI  
**Statut :** implémenté et validé localement  
**Périmètre :** DHCP, route, DNS, TCP SYN, contexte LLM caller-owned

## Objectif

Ce macro-lot fournit un point d’entrée unique pour établir une préconnexion LLM réseau sans allocation dynamique. Il compose l’acquisition du bail DHCP, l’identification du routeur et du DNS, la résolution DNS routée, la sélection du prochain saut de l’hôte LLM, puis l’émission du SYN TCP.

> La façade ne publie le bail, l’IPv4 distante, la connexion TCP et la phase `SYN_SENT` qu’après la réussite complète de toutes les étapes logiques.

## API ajoutée

```c
int ne2k_llm_connection_acquire_start_dhcp(...,
                                           net_dhcp_lease_t* lease,
                                           ne2k_llm_connection_state_t* state,
                                           net_tcp_connection_t* connection);
```

Les paramètres sont tous caller-owned : buffers DHCP TX/RX, buffers ARP RX/TX, trame de transport, bail DHCP, cache ARP, contexte LLM et connexion TCP. Aucun buffer n’est alloué ou retenu par le pilote.

## Déroulement transactionnel

| Étape | Primitive | État externe publié |
|---|---|---|
| Vérification de phase | `IDLE` obligatoire | Aucun. |
| Acquisition IPv4 | `ne2k_dhcp_acquire` sur un bail local | Aucun. |
| Bootstrap routé | `ne2k_llm_connection_start_dhcp` sur copies locales | Aucun. |
| Publication finale | Affectations atomiques | Bail, connexion, IPv4 et `SYN_SENT`. |

Un échec DHCP retourne `-3`; un échec de bootstrap retourne `-4`. Dans les deux cas, le bail fourni, l’état LLM et la connexion TCP fournis par l’appelant restent inchangés. Une phase distincte de `IDLE` est rejetée avant toute opération DHCP.

## Garanties et limites

Le trafic effectivement envoyé (par exemple un DHCP DISCOVER) ne peut pas être annulé à l’échelle matérielle, mais aucune donnée logique caller-owned n’est publiée tant que le flux ne s’achève pas. Les budgets DHCP, DNS et ARP sont explicites et bornés. Aucun secret fournisseur ni endpoint n’est ajouté à l’image.

## Tests et validation locale

Le test NE2000 ajoute une garde du flux unifié avec session `SYN_SENT`. Il vérifie que le refus intervient avant DHCP et conserve intégralement le bail sentinelle, la phase et la séquence TCP. Les tests précédents garantissent les trames DNS/SYN routées et les rollbacks DHCP.

| Vérification | Résultat |
|---|---|
| Test NE2000 ciblé | **20/20** réussis. |
| Suite complète | **383/383** tests réussis. |
| Compilation freestanding i386 | Réussie. |
| Smoke QEMU fournisseur | Réussi. |
| Smoke QEMU NE2000 | Réussi. |

## Limites connues

La façade attend encore que l’appelant fournisse l’endpoint LLM, le hostname, les ports et les buffers TLS. L’exposition de cette opération par un syscall noyau contrôlé, le provisionnement d’endpoint et d’identifiants hors image, TLS live, timeout/retry, fermeture de session, outils, multimodal et Unicode complet restent hors périmètre.

## Références

[1] [AOS-633 à AOS-640 — bootstrap LLM DNS/TCP routé depuis DHCP](aos633_640_llm_dhcp_routed_bootstrap.md)  
[2] [AOS-625 à AOS-632 — UDP et DNS routés par prochain saut](aos625_632_routed_dns_next_hop.md)  
[3] [AOS-601 à AOS-608 — acquisition DHCP transactionnelle NE2000](aos601_608_dhcp_transactional_acquire.md)
