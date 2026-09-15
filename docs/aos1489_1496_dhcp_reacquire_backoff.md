# AOS-1489 à AOS-1496 — Backoff borné de réacquisition DHCP

## Objectif

Ce macro-lot empêche une réacquisition DHCP échouée de relancer DHCP/DNS/ARP/SYN à chaque entrée syscall. Le contexte de maintenance porte désormais un compteur de tentatives, un plafond et une échéance de prochaine tentative. Ces informations restent entièrement statiques et caller-owned.

## Politique de délai

| Paramètre | Valeur | Rôle |
|---|---:|---|
| Délai initial | 100 ticks | Première attente après un échec de réacquisition. |
| Délai maximal | 10 000 ticks | Plafond qui évite tout dépassement ou attente non bornée. |
| Tentatives maximales | 5 | Stoppe les nouvelles émissions après échecs répétés. |
| Réinitialisation | Bootstrap réussi | Efface compteur et échéance lors de la publication d’un nouveau bail. |

La maintenance retourne immédiatement si l’horloge n’a pas atteint `next_retry_tick`. Lorsqu’une tentative est autorisée, elle utilise un XID dérivé de la requête mémorisée et du nombre de tentatives déjà consommées. En cas d’échec, le délai est doublé jusqu’au plafond ; en cas de réussite, `kernel_llm_acquire_start` réarme la maintenance avec un compteur nul.

> Le backoff ne dort jamais et ne bloque jamais. Il est évalué à l’entrée syscall, hors IRQ0, exactement comme le renouvellement et la réacquisition DHCP.

## Invariants

Aucun slot socket n’est conservé entre un bail expiré et une tentative différée : la session est fermée avant la première réacquisition. Les erreurs de bootstrap restent transactionnelles et ne publient ni bail, ni socket, ni phase LLM intermédiaire. Lorsque le plafond est atteint, les appels ultérieurs ne transmettent plus de trames DHCP.

## Validation

| Contrôle | Résultat |
|---|---|
| `make all` | Réussi |
| Tests noyau | 36/36 verts |
| `make test-all` | 444/444 tests verts |
| `git diff --check` | Propre |
| Allocations dynamiques | Aucune occurrence dans les chemins modifiés |

## Limites restantes

Le backoff couvre désormais les réacquisitions de bail. La conservation sécurisée des identifiants OpenAI lors d’une fermeture automatique, une politique de reprise TLS/HTTP/SSE plus complète et des tests QEMU avec serveur DHCP réel restent à effectuer.

## Références

[1]: aos1481_1488_dhcp_reacquisition.md "Réacquisition DHCP après expiration"
[2]: aos1473_1480_dhcp_deferred_maintenance.md "Maintenance DHCP différée dans le noyau"

[1] [2]
