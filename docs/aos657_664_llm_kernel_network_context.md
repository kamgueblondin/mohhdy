# AOS-657 à AOS-664 — Plan de contrôle noyau avec contexte réseau LLM agrégé

**Auteur :** Manus AI

**Statut :** implémenté et validé localement

**Périmètre :** adoption persistante du contexte réseau LLM par le noyau et diagnostic non sensible dans le shell

## Objectif

Ce macro-lot raccorde le contexte réseau LLM agrégé au plan de contrôle du noyau. Au démarrage, le noyau initialise un unique `ne2k_llm_network_context_t` persistant, qui rassemble le bail DHCP, la session LLM et la connexion TCP. Cette consolidation prépare les prochains syscalls de démarrage, de polling TLS et d’émission de requête, tout en gardant les détails de transport et les secrets hors de l’interface utilisateur.

```c
typedef struct {
    net_dhcp_lease_t lease;
    ne2k_llm_connection_state_t session;
    net_tcp_connection_t connection;
} ne2k_llm_network_context_t;
```

## Contrat du statut noyau

`kernel_llm_session_status()` publie un mot de contrôle minimal. Il ne contient ni IPv4, ni masque, ni routeur, ni DNS, ni endpoint, ni clé TLS, ni identifiant fournisseur.

| Bits | Signification | Origine |
|---|---|---|
| `0` | NE2000 prêt | `boot_ne2k_present` |
| `1` | Bail DHCP présent | `boot_llm_network.lease.valid` |
| `8..15` | Phase de session LLM | `boot_llm_network.session.phase` |
| autres | Réservés | `0` |

> La présence du bail est un indicateur logique uniquement. Les paramètres de configuration réseau restent confinés au contexte noyau.

## Intégration au démarrage

`ne2k_boot_probe()` initialise `boot_llm_network` avant toute tentative de sonde NE2000. Le statut reste donc déterministe lorsque la carte est absente : phase `IDLE`, bit NE2000 absent et bit DHCP absent. Lorsque la carte est détectée mais qu’aucune acquisition DHCP n’a été demandée, la phase reste également `IDLE` et le bit DHCP reste absent.

Le shell `ai-runtime` affiche désormais une ligne distincte, `Bail DHCP noyau`, à partir du bit 1. Cette restitution facilite le diagnostic sans fournir de données directement exploitables hors du noyau.

## Garanties de sûreté

| Propriété | Garantie |
|---|---|
| Allocation dynamique | Aucune allocation n’est ajoutée ; le contexte est un objet statique du noyau. |
| Propriété des buffers | Les buffers de DHCP, ARP, DNS, TCP, TLS, HTTP et LLM restent fournis par l’appelant lors des futures façades. |
| Secrets | Aucun identifiant, token, hostname, clé TLS ou adresse réseau n’est publié par le statut. |
| État initial | `ne2k_llm_network_context_init` remet le bail à invalide et la session à `IDLE`. |
| Compatibilité | Le bit historique NE2000 et l’encodage de phase restent inchangés. |

## Tests et validation locale

Les deux smokes QEMU ont été enrichis pour contrôler la nouvelle ligne de diagnostic dans les scénarios sans acquisition DHCP : sans NE2000, puis avec un NE2000 ISA détecté. Ils confirment que le contexte noyau ne prétend pas posséder de bail avant une transaction DHCP effective.

| Vérification | Résultat |
|---|---|
| Compilation freestanding i386 | Réussie. |
| Smoke QEMU fournisseur | Réussi ; `IDLE`, NE2000 absent, bail DHCP absent. |
| Smoke QEMU NE2000 | Réussi ; `IDLE`, NE2000 prêt, bail DHCP absent. |
| Suite unitaire complète | Réussie : **384/384** tests. |
| `git diff --check` | Propre. |

## Limites connues

Ce lot ne déclenche ni DHCP, ni DNS, ni TCP, ni TLS depuis le noyau. Les prochains lots doivent fournir un syscall contrôlé de démarrage DHCP→LLM, suivi des syscalls de polling TLS, d’émission HTTP et de lecture de réponse. Le provisionnement d’endpoint et d’identifiants, TLS live, timeout/retry, fermeture, outils, multimodal et Unicode complet demeurent hors périmètre.

## Références

[1] [AOS-649 à AOS-656 — contexte réseau LLM caller-owned](aos649_656_llm_network_context.md)

[2] [AOS-641 à AOS-648 — façade unifiée DHCP vers session LLM](aos641_648_llm_dhcp_unified_start.md)

[3] [AOS-617 à AOS-624 — routage DHCP par prochain saut](aos617_624_dhcp_next_hop.md)
