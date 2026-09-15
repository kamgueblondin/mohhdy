# AOS-1473 à AOS-1480 — Maintenance DHCP différée dans le noyau

## Objectif

Ce lot introduit une maintenance DHCP interne à l’orchestrateur LLM. Après une acquisition DHCP/DNS/ARP/SYN réussie, le noyau conserve une copie bornée de la requête d’acquisition, sans pointeur ni secret. Cette configuration permet d’évaluer les échéances du bail et de lancer le renouvellement depuis un contexte noyau sûr.

> Les E/S de renouvellement DHCP ne sont jamais exécutées depuis l’interruption d’horloge. L’IRQ0 maintient uniquement le temps ; le renouvellement est évalué lors d’une entrée syscall, hors contexte d’interruption.

## Conception

| Élément | Garantie |
|---|---|
| `kernel_llm_dhcp_maintenance_t` | Contexte statique contenant un bit d’armement et la requête POD d’acquisition. |
| Armement | Publication seulement après succès complet de DHCP/DNS/ARP/SYN. |
| `kernel_llm_dhcp_maintenance(now)` | Appelle `ne2k_dhcp_renew_if_due` avec les buffers noyau caller-owned existants. |
| Déclenchement | Entrée syscall, après réactivation contrôlée des interruptions, jamais depuis IRQ0. |
| Renouvellement | Réutilise le REQUEST avec `ciaddr`, l’ACK borné et la publication transactionnelle du bail. |

## Invariants

La maintenance n’alloue aucune mémoire et n’expose aucun nouveau pointeur utilisateur. Un renouvellement non dû retourne sans transmission. Les échecs de transmission ou d’ACK ne publient pas de bail intermédiaire, car `ne2k_dhcp_renew_if_due` travaille sur une copie locale et ne remplace le bail qu’après ACK valide et marquage d’acquisition.

Le résultat de maintenance est volontairement séparé du résultat du syscall utilisateur qui l’a déclenchée. Cette séparation évite de modifier les ABI existantes tout en empêchant une opération réseau longue d’être exécutée dans l’ISR du timer.

## Validation

| Contrôle | Résultat |
|---|---|
| `make all` | Réussi |
| Tests noyau | 36/36 verts |
| `make test-all` | 444/444 verts |
| `git diff --check` | Propre |
| Allocations dynamiques dans les chemins modifiés | Aucune occurrence |

## Limites restantes

La maintenance renouvelle désormais automatiquement lorsqu’un syscall intervient après l’échéance. La réacquisition complète après expiration du bail reste le prochain incrément : elle devra fermer la session existante, relancer le bootstrap à partir de la requête statique et préserver les règles de nettoyage des secrets et du slot socket. Des délais/backoff explicites entre tentatives restent également à introduire.

## Références

[1]: aos1365_1372_dhcp_live_renewal.md "Renouvellement DHCP live caller-owned"
[2]: aos1461_1472_kernel_socket_orchestrator.md "Orchestrateur noyau LLM sur session socket"

[1] [2]
