# AOS-1481 à AOS-1488 — Réacquisition DHCP après expiration

## Objectif

Ce macro-lot complète la maintenance DHCP différée par une réacquisition automatique. Lorsque la primitive de renouvellement constate qu’un bail n’est plus valide à l’instant courant, le noyau ferme la session LLM socket, efface le bail, incrémente l’identifiant de transaction DHCP et relance le bootstrap DHCP/DNS/ARP/SYN à partir de la requête statique validée lors de l’acquisition initiale.

> La réacquisition est exécutée depuis l’entrée syscall, jamais depuis IRQ0. Le gestionnaire d’horloge conserve ainsi ses garanties de faible latence et ne réalise aucune E/S réseau.

## Séquence

| Étape | Action | Publication |
|---|---|---|
| 1 | `ne2k_dhcp_renew_if_due` détecte un bail expiré. | Aucune mutation partielle du bail. |
| 2 | `kernel_llm_close` tente FIN+ACK, puis libère le slot. | La session devient `IDLE`. |
| 3 | Le bail est effacé et le XID DHCP est incrémenté. | L’ancien bail ne peut plus être réutilisé. |
| 4 | `kernel_llm_acquire_start` rejoue DHCP/DNS/ARP/SYN. | La nouvelle session est publiée seulement après bootstrap complet. |

## Invariants

La requête réutilisée est une structure POD statique, bornée, sans pointeur et sans secret. Le bootstrap conserve ses propres transactions : un échec DHCP, DNS, ARP ou SYN ne publie pas de socket incomplet. Les buffers DHCP, TLS et HTTP sont toujours ceux préalloués du noyau.

La fermeture retourne son diagnostic historique mais la réacquisition libère le slot avant de relancer le bootstrap. Elle ne conserve donc pas de capacité TCP réservée si le bail a expiré. Aucun chemin modifié ne recourt à une allocation dynamique.

## Validation

| Contrôle | Résultat |
|---|---|
| `make all` | Réussi |
| Tests noyau | 36/36 verts |
| `make test-all` | 444/444 tests verts |
| `git diff --check` | Propre |
| Allocations dynamiques | Aucune occurrence dans les chemins modifiés |

## Limites restantes

La reprise est immédiate à la première entrée syscall post-expiration. Le prochain axe introduira un délai exponentiel borné entre échecs de réacquisition, une politique de conservation/configuration des identifiants OpenAI après une fermeture automatique, et une validation QEMU contre un serveur DHCP réel.

## Références

[1]: aos1473_1480_dhcp_deferred_maintenance.md "Maintenance DHCP différée dans le noyau"
[2]: aos1449_1460_socket_bootstrap.md "Bootstrap DHCP/DNS/ARP/SYN vers session LLM socket"

[1] [2]
