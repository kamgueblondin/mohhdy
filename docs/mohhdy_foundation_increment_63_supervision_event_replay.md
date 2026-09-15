# Foundation 63 — Rediffusion locale d’événements de supervision

## Objet

Le lot Foundation 63 permet à un parent utilisateur de **rediffuser par séquence** une transition de supervision encore retenue dans son journal local. Cette opération répare explicitement une notification détaillée perdue à saturation, ou permet une nouvelle lecture après consommation, sans modifier l’événement source ni la configuration de souscription.

> La rediffusion consulte l’événement retenu dans `task-events`, puis le dépose à nouveau comme `OS_IPC_TASK_SUPERVISION_EVENT`. Elle ne recrée pas une transition noyau et ne reconstitue jamais une entrée déjà oubliée ou écrasée.

| Élément | Contrat livré |
|---|---|
| Syscall | `SYS_TASK_SUPERVISION_EVENT_REPLAY = 82` |
| Plage ABI | `MAX_SYSCALLS = 83` |
| Shell | `task-event-replay <séquence>` |
| Source | Événement de supervision retenu par séquence locale non nulle |
| Saturation | Retour `OS_IPC_FULL` ; la tentative reste visible dans les statistiques de livraison |

## Sémantique

Le parent appelant fournit une séquence non nulle. Le noyau recherche l’événement dans sa propre fenêtre `task-events`, encode une copie du message détaillé et tente directement l’envoi vers son endpoint IPC. La rediffusion ne repasse ni par l’état de souscription, ni par le masque d’action, ni par la watchlist : ces politiques ont déjà déterminé la première tentative, tandis que cette primitive constitue une récupération explicite.

| Cas | Retour |
|---|---|
| Séquence retenue et endpoint disponible | `0`, une copie détaillée est ajoutée à l’IPC |
| Séquence nulle, absente, acquittée, oubliée ou écrasée | `OS_TASK_NO_SUPERVISION_EVENT` |
| Parent non utilisateur ou disparu | `OS_TASK_NOT_FOUND` |
| Endpoint IPC plein | `OS_IPC_FULL` ; `attempted` et `dropped` progressent |

La rediffusion conserve le journal, sa génération, les compteurs enfant et les données de supervision. Elle incrémente les compteurs de livraison détaillée : une copie acceptée incrémente `attempted` et `delivered`, tandis qu’une saturation incrémente `attempted` et `dropped`.

## Usage shell

```text
task-event-replay 4
ipc-recv
```

Le shell affiche `task-event-replay ok 4` après une tentative acceptée. Si la boîte IPC est pleine, il affiche une erreur explicite ; l’utilisateur doit la vider puis réessayer tant que la séquence reste retenue.

## Validation

| Niveau | Vérification |
|---|---|
| Tâches Unity | Rediffusion sans souscription, séquence absente, contenu, conservation du journal, saturation et compteurs |
| Syscalls Unity | ABI 82, transition journalisée, rediffusion puis réception IPC et séquence absente |
| Suite complète | `make test-all` : **249/249** tests réussis |
| Image i386 | `make clean && make all` réussi |
| QEMU spawn | Réception de `delegate-out`, `task-event-replay 4` et seconde réception identique ; statistiques et rétention restent couvertes par Unity |

## Limites explicites

La rediffusion est locale, volatile, non atomique et strictement bornée à la fenêtre circulaire de quatre événements retenus. Elle ne fournit ni persistance, restauration d’événement perdu, recherche globale, sélection par PID/action hors séquence, délai, priorité, quota, réservation, attente, retransmission automatique, accusé, garantie de livraison, ordre global, lecteur multiple, souscription distante, capability, ACL, identité vérifiée, chiffrement ou audit de sécurité. Elle ne rediffuse que la copie détaillée de supervision et ne modifie ni l’événement historique de sortie, ni les résultats enfants, ni la filiation.
