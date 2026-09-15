# AOS-1497 à AOS-1504 — Conservation fournisseur lors de reprise automatique

## Objectif

La réacquisition DHCP automatique devait pouvoir restaurer la session réseau sans obliger l’utilisateur à reconfigurer le fournisseur après chaque expiration de bail. Ce lot distingue donc la fermeture explicite, qui efface le Bearer OpenAI, de la fermeture interne de reprise, qui préserve ce seul élément de configuration dans le noyau.

> Une fermeture demandée explicitement par l’utilisateur efface toujours le Bearer. Seule la fermeture transitoire déclenchée par une expiration DHCP peut conserver la configuration fournisseur jusqu’au bootstrap suivant.

## API interne

| Chemin | `preserve_provider` | Effet sur le Bearer |
|---|---:|---|
| `kernel_llm_close()` | Non | Effacement borné du buffer et remise à zéro du drapeau de disponibilité. |
| Réacquisition DHCP automatique | Oui | Bearer et drapeau conservés pendant la fermeture socket et le nouveau bootstrap. |

La routine de nettoyage conserve toujours les règles existantes pour les matériaux TLS, les buffers HTTP/SSE, la session socket, les transcriptions et les espaces de travail cryptographiques. La conservation porte exclusivement sur le Bearer fournisseur déjà stocké dans la zone noyau non accessible aux syscalls.

## Garanties

La reprise automatique exécute une fermeture interne puis un bootstrap DHCP/DNS/ARP/SYN hors IRQ0. Le slot TCP reste libéré avant toute tentative suivante. L’utilisateur peut toujours invalider immédiatement la configuration sensible par une fermeture explicite.

Aucune copie vers l’espace utilisateur, allocation dynamique ou exposition d’un secret dans le diagnostic réseau n’est ajoutée.

## Validation

| Contrôle | Résultat |
|---|---|
| `make all` | Réussi |
| Tests noyau | 36/36 verts |
| `make test-all` | 444/444 tests verts |
| `git diff --check` | Propre |
| Allocations dynamiques | Aucune occurrence dans les chemins modifiés |

## Limites restantes

La session transport est désormais récupérable avec sa configuration fournisseur. La reprise d’une requête HTTP ou d’un flux SSE interrompu, avec conservation du prompt, du modèle et des identifiants d’événement, reste à concevoir. Le client OpenAI effectif nécessite encore une campagne d’intégration live distincte.

## Références

[1]: aos1489_1496_dhcp_reacquire_backoff.md "Backoff borné de réacquisition DHCP"
[2]: aos1461_1472_kernel_socket_orchestrator.md "Orchestrateur noyau LLM sur session socket"

[1] [2]
