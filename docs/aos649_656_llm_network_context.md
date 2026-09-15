# AOS-649 à AOS-656 — Contexte réseau LLM caller-owned

**Auteur :** Manus AI  
**Statut :** implémenté et validé localement  
**Périmètre :** persistance de bail DHCP, session LLM et connexion TCP sans allocation dynamique

## Objectif

Ce macro-lot rassemble les trois états qui doivent survivre entre le bootstrap réseau et les tours LLM : le bail DHCP, le contexte de phase LLM et la connexion TCP. Le nouvel agrégat est strictement détenu par l’appelant ; il ne contient ni buffer de trame, ni hostname, ni endpoint, ni clé TLS, ni identifiant fournisseur.

```c
typedef struct {
    net_dhcp_lease_t lease;
    ne2k_llm_connection_state_t session;
    net_tcp_connection_t connection;
} ne2k_llm_network_context_t;
```

## Cycle de vie

| API | Précondition | Effet |
|---|---|---|
| `ne2k_llm_network_context_init` | Contexte non nul | Efface le bail, initialise la session à `IDLE` et remet la connexion à zéro. |
| `ne2k_llm_network_context_acquire_start_dhcp` | `IDLE`, buffers fournis par l’appelant | Délègue au flux DHCP→DNS→SYN transactionnel sur les champs agrégés. |
| `ne2k_llm_network_context_reset_for_request` | `RESPONSE_READY` | Délègue au réarmement TLS et repasse à `TLS_COMPLETE` sans toucher au bail ou au TCP. |

Le contexte centralise ainsi les états longs, alors que les buffers DHCP, ARP, DNS, TLS, HTTP et LLM restent éphémères et explicitement fournis à chaque appel.

## Garanties de sûreté

L’initialisation ne réalise aucune allocation dynamique. La façade de démarrage délègue à `ne2k_llm_connection_acquire_start_dhcp`, qui utilise des copies locales avant publication. Un échec ne modifie donc ni le bail, ni la phase, ni la connexion du contexte. Le réarmement de requête conserve explicitement le bail et la séquence TCP pour permettre un tour suivant sur la session TLS existante.

## Tests et validation locale

Le test de cycle de vie initialise un contexte, crée une connexion TCP, place la session en `RESPONSE_READY`, puis la réarme. Il vérifie le passage à `TLS_COMPLETE`, la préservation de l’IPv4 de bail et de la séquence TCP, ainsi que le rejet d’un contexte nul.

| Vérification | Résultat |
|---|---|
| Test NE2000 ciblé | **21/21** réussis. |
| Suite complète | **384/384** tests réussis. |
| Compilation freestanding i386 | Réussie. |
| Smoke QEMU fournisseur | Réussi. |
| Smoke QEMU NE2000 | Réussi. |

## Limites connues

Le contexte est une primitive noyau caller-owned et n’est pas encore retenu par un service noyau ni exposé par un syscall de démarrage contrôlé. Le provisionnement d’endpoint et d’identifiants, TLS live, timeout/retry, fermeture de session, outils, multimodal et Unicode complet restent hors périmètre.

## Références

[1] [AOS-641 à AOS-648 — façade unifiée DHCP vers session LLM](aos641_648_llm_dhcp_unified_start.md)  
[2] [AOS-633 à AOS-640 — bootstrap LLM DNS/TCP routé depuis DHCP](aos633_640_llm_dhcp_routed_bootstrap.md)  
[3] [AOS-577 à AOS-584 — réutilisation d’une session TLS LLM](aos577_584_llm_session_reuse.md)
