# Incrément MOHHDY Foundation 19 — Capacité IPC des services publiés

## Objectif

Cet incrément rend explicite la pression de file appliquée aux tâches qui possèdent au moins un nom dans le registre de services. Il ne remplace pas la FIFO IPC par tâche : il ajoute une politique bornée devant son entrée pour protéger un serveur publié d’une accumulation de messages clients.

| Périmètre | Capacité |
|---|---:|
| Endpoint IPC brut de toute tâche Ring 3 | 4 messages |
| Messages clients vers un propriétaire de service publié | 2 messages en attente |
| Notifications noyau de service | capacité brute existante, livraison best-effort |

> Cette limite est attachée au **PID propriétaire d’au moins un service**, non à un nom isolé ni à une file séparée par service. Un processus qui publie plusieurs noms partage donc la même limite de deux messages clients.

## Contrat

`sys_ipc_send` vérifie que la cible est une tâche utilisateur vivante, puis consulte le registre. Si le PID cible possède au moins un nom de service et que sa file contient déjà deux messages, l’envoi retourne `OS_IPC_SERVICE_FULL` (`-44`). Le payload n’est pas copié et l’ordre des messages déjà admis reste inchangé.

| Situation | Retour |
|---|---:|
| Cible ordinaire, file inférieure à 4 | `0` |
| Cible ordinaire, quatre messages | `OS_IPC_FULL` (`-41`) |
| Propriétaire de service, file de 0 ou 1 message | `0` |
| Propriétaire de service, deux messages | `OS_IPC_SERVICE_FULL` (`-44`) |
| Cible invalide, terminée ou non utilisateur | `OS_IPC_BAD_TARGET` (`-42`) |

Les notifications de publication, transfert, retrait et purge empruntent toujours le chemin noyau best-effort existant. Elles ne passent pas par ce filtre de clients et peuvent donc occuper les deux emplacements restants de la FIFO brute ; aucune garantie de livraison n’est ajoutée.

## Interface et vérification

Le shell traduit le nouveau retour en `ipc-send: capacite du service atteinte`, distinct de `ipc-send: boite aux lettres pleine` pour une tâche non publiée.

La suite porte à **201/201** les tests Unity et robustesse. Le prédicat de registre qui identifie un propriétaire est vérifié après publication, transfert et purge. Le contrat QEMU de transfert publie `demo`, envoie deux messages au shell propriétaire, exige le refus du troisième, vide les deux messages admis, puis rejoue intégralement la surveillance, le transfert à `serviceclaim` et la purge après terminaison.

## Limites honnêtes

La politique ne connaît ni priorité, ni poids, ni budget par nom, ni débit, ni délai, ni blocage, ni accusé de réception, ni réservation pour les réponses RPC. Elle ne protège pas un service contre les appels directs aux syscalls de stockage, ne fournit aucune capability ni identité vérifiée, et ne transforme pas la FIFO en file de messages durable. La limite est volatile, globale au PID propriétaire et réévaluée seulement lors de l’envoi ; elle n’est pas une isolation forte par service.
