# AOS-1817…1824 — Réconciliation DHCP du contexte LLM

## Objet

Ce macro-lot ajoute `ne2k_llm_network_context_reconcile_lease()`. Cette primitive compare le bail entrant aux paramètres IPv4 actifs du contexte. Si l’adresse, le masque, le serveur DHCP, la passerelle, le DNS ou leurs indicateurs de validité changent, elle publie le nouveau bail et réinitialise la session LLM ainsi que la connexion TCP devenues obsolètes.

| Cas | Bail | Session / TCP | Checkpoint SSE |
|---|---|---|---|
| Acquisition seulement renouvelée | Mis à jour | Conservés | Conservé |
| Paramètres IPv4 inchangés | Mis à jour | Conservés | Conservé |
| Route, DNS ou adresse modifiés | Mis à jour | Réinitialisés | Conservé |
| Entrée nulle | Inchangé | Inchangés | Inchangé |

## Transactionnalité

La fonction travaille sur une copie complète du contexte. La comparaison porte sur les indicateurs de validité et tous les octets IPv4 des paramètres réseau. Lorsqu’une rupture de continuité est détectée, la session devient `IDLE` et la connexion TCP est vidée. Le checkpoint SSE n’est pas effacé : il reste disponible pour une reprise logique après une nouvelle connexion sécurisée.

> Aucun buffer, secret, bearer ou modèle n’est stocké ni modifié. La réconciliation ne purge que l’état de transport lié à l’ancien adressage.

## Tests

Le vecteur NE2000 couvre un bail dont seul le tick d’acquisition évolue, puis un changement de passerelle. Il vérifie la conservation de la session dans le premier cas, sa réinitialisation dans le second et la restauration du checkpoint `Last-Event-ID` dans les deux cas.

| Vérification | Résultat |
|---|---|
| Bail réseau identique | Session conservée |
| Changement de passerelle | Session/TCP réinitialisés |
| Checkpoint SSE | Conservé et restaurable |
| Contexte nul | Rejet |
| Test NE2000 ciblé | **42/42 réussis** |

## Références

[1] [AOS-609 à AOS-616 — bail DHCP, route et DNS](aos609_616_dhcp_route_dns.md)  
[2] [AOS-1809 à AOS-1816 — renouvellement DHCP de contexte](aos1809_1816_dhcp_context_renewal.md)  
[3] [RFC 2131 — Dynamic Host Configuration Protocol](https://www.rfc-editor.org/rfc/rfc2131)
