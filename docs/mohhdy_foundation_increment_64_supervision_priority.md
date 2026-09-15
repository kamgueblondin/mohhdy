# Foundation 64 — Supervision prioritaire locale

## Objet

Le lot Foundation 64 permet à un parent utilisateur de désigner **un enfant direct prioritaire**. Tant que la souscription aux notifications détaillées est active, les transitions de cet enfant sont proposées à l’IPC même si le filtre d’action courant ou la watchlist les exclurait. Le journal de supervision demeure complet et inchangé.

| Élément | Contrat livré |
|---|---|
| Sélection | `SYS_TASK_SUPERVISION_PRIORITY = 83` |
| État | `SYS_TASK_SUPERVISION_PRIORITY_STATUS = 84` |
| ABI | `MAX_SYSCALLS = 85` |
| Structure | `os_task_supervision_priority_status_t` |
| Shell | `task-priority-child <pid|off>`, `task-priority-child-status` |

La sélection est strictement locale au parent et ne concerne qu’un unique enfant direct actif. `off` efface le choix. La priorité est automatiquement effacée lorsqu’un enfant prioritaire sort ou est délégué vers un autre superviseur.

## Sémantique

La souscription IPC reste indispensable ; la priorité ne rend pas l’IPC bloquant ni fiable. Une fois la souscription active, un événement de l’enfant prioritaire contourne le masque de notification et la watchlist. Les compteurs de livraison comptabilisent toujours la tentative, la livraison ou la perte à saturation.

| Cas | Résultat |
|---|---|
| PID enfant direct utilisateur | PID sélectionné, priorité remplacée localement |
| `0` / `off` | Effacement, l’état renvoie `child_pid = -1` |
| PID absent | `OS_TASK_NOT_FOUND` |
| PID non direct, non utilisateur ou terminé | `OS_TASK_NOT_CHILD` |
| Sortie ou délégation de l’enfant prioritaire | Effacement automatique |

## Validation

La suite `make test-all` exécute **251/251** tests verts, y compris la sélection, l’instantané, le contournement d’un filtre vide et le nettoyage à la sortie. La reconstruction i386 est réussie. Le scénario QEMU `spawn` sélectionne l’enfant délégué comme prioritaire puis vérifie la notification détaillée et sa rediffusion sous le budget de 240 secondes.

## Limites explicites

Cette priorité est locale, volatile, non atomique et limitée à un enfant direct. Elle ne modifie ni l’ordonnancement CPU, ni l’ordre de la boîte IPC, ni l’ordre ou la capacité du journal. Elle ne fournit ni plusieurs niveaux de priorité, héritage, réservation de capacité, délai, quota, attente, retransmission, accusé, garantie de livraison, persistance, capability, ACL, identité vérifiée, chiffrement ni audit de sécurité.
