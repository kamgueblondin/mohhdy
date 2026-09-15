# AOS-1505 à AOS-1512 — Reprise applicative HTTP/SSE après récupération réseau

## Objectif

Ce macro-lot complète la récupération DHCP/TLS par la reprise applicative. Après une émission LLM réussie, le noyau mémorise la requête HTTP ou SSE dans un contexte statique. Si le réseau doit être réacquis, la requête reste disponible pendant la fermeture interne. Dès que le nouveau handshake TLS est terminé, elle est réémise sur la session socket authentifiée.

> La requête n’est mémorisée qu’après son émission réussie. Une requête rejetée, mal formée ou non transmise ne peut pas être rejouée automatiquement.

## Cycle de reprise

| Phase | Garantie |
|---|---|
| Émission LLM | La requête validée est copiée dans le contexte static caller-owned après succès. |
| Réacquisition réseau | La fermeture automatique conserve la requête, mais libère le slot, les clés TLS et les buffers de transport. |
| Nouveau TLS | Le polling ne réarme pas l’application avant l’état `TLS_COMPLETE`. |
| Réémission | `kernel_llm_request` reconstruit un nouveau JSON, HTTP et record TLS avec la requête mémorisée. |
| Fermeture explicite | La requête mémorisée est effacée avec les identifiants fournisseur. |

## Invariants

Le contexte de reprise ne contient que la structure publique de requête, déjà bornée et validée. Les secrets fournisseur restent confinés au buffer noyau et suivent la politique du lot AOS-1497 à AOS-1504. Les buffers HTTP, SSE, TLS et TCP sont toujours effacés lors du nettoyage ; aucun fragment de transport antérieur n’est rejoué.

La réémission est déclenchée uniquement depuis le polling TLS, donc hors IRQ0. Elle maintient les mêmes contrôles de fournisseur, de Bearer OpenAI, de modèle, de chemin et de taille de prompt que l’émission initiale.

## Validation

| Contrôle | Résultat |
|---|---|
| `make all` | Réussi |
| Tests noyau | 36/36 verts |
| `make test-all` | 444/444 tests verts |
| `git diff --check` | Propre |
| Allocations dynamiques | Aucune occurrence dans les chemins modifiés |

## Limites restantes

La reprise réémet une requête complète et protège les flux SSE via la même requête de streaming. La reprise fine SSE avec `Last-Event-ID`, la conservation d’un prompt partiellement consommé et des tests QEMU contre des coupures réelles restent les prochaines améliorations.

## Références

[1]: aos1497_1504_provider_recovery.md "Conservation fournisseur lors de reprise automatique"
[2]: aos1425_1436_socket_llm_orchestrator.md "Orchestrateur actif LLM sur socket"

[1] [2]
