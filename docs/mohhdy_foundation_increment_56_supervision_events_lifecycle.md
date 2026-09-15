# Lot Foundation 56 — Consommation post-mortem du journal de supervision

## Objet

Ce lot complète le journal local de supervision du lot 55 par une lecture conditionnelle à une génération donnée et un acquittement explicite. Il permet à un parent de distinguer un instantané encore courant d’un historique déjà modifié, puis de vider volontairement sa fenêtre locale.

| Élément | Contrat livré |
|---|---|
| ABI ajoutée | `SYS_TASK_SUPERVISION_EVENTS_ACK = 70`, `SYS_TASK_SUPERVISION_EVENTS_OBSERVE = 71` |
| Plage ABI | syscalls 0–71 ; `MAX_SYSCALLS = 72` |
| Commandes Ring 3 | `task-events-observe <génération>` et `task-events-clear` |
| Erreur de concurrence | `OS_TASK_HISTORY_STALE` (`-70`), déjà définie par l’ABI d’historique enfant |
| Portée | journal borné, local au parent appelant, volatile et non atomique |

## Contrat d’observation

`SYS_TASK_SUPERVISION_EVENTS_OBSERVE` reçoit la génération attendue en `EBX` et un pointeur vers `os_task_supervision_events_observation_t` en `ECX`. La structure de sortie porte systématiquement la génération courante. Si la valeur attendue n’est plus courante, le syscall renvoie `OS_TASK_HISTORY_STALE` sans remettre un instantané d’événements ; l’appelant peut alors décider de relire le journal.

| Situation | Retour | Sortie |
|---|---:|---|
| Génération identique | `0` | génération et instantané circulaire ordonné |
| Génération différente | `OS_TASK_HISTORY_STALE` | génération courante uniquement |
| Parent absent ou pointeur nul | `OS_TASK_NOT_FOUND` | aucune garantie utile |

Cette règle évite qu’un consommateur traite silencieusement une fenêtre remplacée ou modifiée entre deux lectures. Elle ne fournit toutefois ni verrou, ni réservation de génération, ni lecture atomique inter-processus.

## Contrat d’acquittement

`SYS_TASK_SUPERVISION_EVENTS_ACK` ne prend pas d’argument. Il vide la fenêtre circulaire du parent courant, conserve sa séquence monotone pour les événements futurs et incrémente la génération. La nouvelle génération est retournée dans `EAX` sous forme d’entier positif.

Après l’acquittement, une lecture simple retourne un journal de longueur zéro à la nouvelle génération. Toute observation fondée sur la génération précédente est périmée. Le prochain événement reprend la séquence locale suivante et crée une nouvelle génération.

> L’acquittement ne modifie ni les résultats enfants, ni l’historique de sorties, ni le compteur cumulatif de départs, ni la filiation, ni les notifications IPC déjà émises.

## Interface Ring 3

| Commande | Sortie réussie | Cas périmé |
|---|---|---|
| `task-events-observe 3` | `task-events-observe ok 3 <nombre>` suivi des entrées | `task-events-observe stale <génération_courante>` |
| `task-events-clear` | `task-events-clear ok <nouvelle_génération>` | erreur syscall explicite |

## Vérification

| Preuve | Résultat |
|---|---|
| `make all` | noyau i386, Ring 3 et initrd construits avec succès |
| `make test-all` | **235/235** tests Unity et robustesse réussis |
| Test de tâche | observation fraîche et stale, génération exposée, acquittement et fenêtre vide |
| Test ABI | syscalls 70–71, lecture conditionnelle, acquittement et nouvelle génération |
| Contrat QEMU `spawn` | observation stale/fraîche, acquittement, fenêtre vide puis nouvelle délégation |
| `make qemu-smoke` | core, extras, persistance, spawn et exec réussis ; budget spawn porté à 210 s pour couvrir les preuves additionnelles sans assouplir leurs assertions |

## Limites

Cette consommation est **locale** et ne rend pas le journal durable. Elle ne crée ni audit sécurisé, ni identité, capability, ACL, chiffrement, signature, intégrité, verrou, transaction, accusé de lecture, réservation, filtre, recherche, souscription, export, synchronisation ou rétention configurable. La fenêtre contient au plus quatre événements et son contenu peut être remplacé avant ou après toute observation. La génération et la séquence sont sur 32 bits et peuvent déborder.

L’acquittement est global au journal du parent courant : il ne sait pas effacer une seule entrée, cibler un enfant, fusionner des journaux, récupérer un événement évincé ou gérer plusieurs lecteurs. Il ne modifie pas les autres mécanismes de supervision.

## Fichiers concernés

| Zone | Évolution |
|---|---|
| `include/os_syscalls.h` | ABI 70–71 et structure d’observation publique |
| `kernel/task/task.[ch]` | primitives d’acquittement et d’observation conditionnelle |
| `kernel/syscall/syscall.[ch]` | adaptateurs et dispatch ABI 70–71 |
| `userspace/shell.c` | wrappers et commandes `task-events-observe` / `task-events-clear` |
| `tests/framework/kernel_mocks.c` | miroir des primitives et du dispatch |
| `tests/unit/kernel/test_task.c` | cycle de vie local du journal |
| `tests/unit/kernel/test_syscall.c` | preuves ABI 70–71 |
| `tests/scripts/ci_qemu_spawn.py` | contrat Ring 3 de lecture/acquittement |
| `tests/scripts/ci_qemu_smoke.sh` | budget spawn de 210 secondes |

## Références

[1] [Journal borné des transitions de supervision — lot Foundation 55](mohhdy_foundation_increment_55_supervision_events.md)

[2] [État réel de MOHHDY](ETAT_REEL.md)
