# Incrément MOHHDY Foundation 20 — État de capacité des services

## Objectif

Cet incrément rend observable la capacité IPC introduite à l’incrément 19. Toute tâche Ring 3 peut demander un instantané borné pour un **nom de service publié vivant**. Le noyau retourne le PID propriétaire, le nombre total de messages actuellement en FIFO, la limite de deux messages clients et la capacité brute de quatre messages.

> Cet état est une observation locale instantanée. Il ne réserve aucun emplacement, ne garantit pas qu’un envoi ultérieur réussira et ne confère aucun droit sur le service.

| Élément | Valeur livrée |
|---|---:|
| Nouveau syscall | `SYS_SERVICE_STATUS` = 36 |
| ABI totale | 37 syscalls, numéros 0–36 |
| Structure de réponse | `os_service_status_t`, 16 octets |
| Profondeur exposée | nombre total de messages dans la FIFO du propriétaire |
| Capacité client | 2 messages pour un PID propriétaire d’au moins un service |
| Capacité brute | 4 messages par endpoint Ring 3 |

## Contrat ABI

L’appelant fournit un nom et un pointeur vers la structure de sortie.

```c
int sys_service_status(const char *name, os_service_status_t *out);
```

| Champ | Type | Sémantique |
|---|---|---|
| `owner_pid` | `int32_t` | propriétaire actuel du nom résolu |
| `queued_messages` | `uint32_t` | profondeur totale de sa FIFO au moment de la copie |
| `client_capacity` | `uint32_t` | limite appliquée aux envois clients vers un propriétaire publié : 2 |
| `endpoint_capacity` | `uint32_t` | capacité physique de FIFO : 4 |

Un nom absent ou dont le propriétaire n’est plus une tâche utilisateur vivante retourne `OS_SERVICE_NOT_FOUND`. Un appel hors Ring 3 ou sans structure de sortie retourne `OS_SERVICE_BAD_NAME`. La recherche réutilise le nettoyage lazy déjà présent pour les propriétaires terminés.

## Interface shell et vérification

La commande suivante rend l’état lisible sans envoyer de message :

```text
service-status demo
service-status ok demo pid 1 queued 2 client-capacity 2 endpoint-capacity 4
```

Le contrat QEMU de transfert vérifie trois observations : profondeur 0 après publication, profondeur 2 après deux messages clients acceptés, puis profondeur 0 après lecture des deux messages. Il exige ensuite le refus explicite du troisième message, puis valide toujours l’abonnement, le transfert de `demo` à `serviceclaim` et la purge après `kill`.

La suite conserve **201/201** tests Unity et robustesse et les six contrats de `make integration-qemu` réussissent. Le contrat VFS garde des relances bornées sur les lectures d’alias et le contrat de service cadence les frappes à 0,55 s afin de résister aux rebonds PS/2 observés sous QEMU TCG ; chaque relance conserve les mêmes marqueurs fonctionnels exigés.

## Limites honnêtes

L’instantané n’est ni atomique, ni horodaté, ni souscription, ni métrique historique. Il expose la profondeur totale, donc les notifications noyau best-effort y sont aussi visibles ; il ne sépare pas messages clients, événements ou réponses RPC. Aucune priorité, capacité, quota par nom, réservation, délai, blocage, identité vérifiée ou capability n’est fournie. Le backend VFS reste noyau et cet état ne transforme pas la boîte aux lettres locale en système de communication distribué.
