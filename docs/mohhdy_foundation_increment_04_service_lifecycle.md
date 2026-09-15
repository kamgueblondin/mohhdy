# MOHHDY Foundation — incrément 04 : cycle de vie des services

**Statut :** conception de la tranche suivante, fondée sur le registre nommé `vfs`.

**Portée MOHHDY :** renforcement réduit de US-003, US-012 et US-013 : éviter qu’un nom de service continue à désigner une tâche terminée.

**Non-objectif :** ce mécanisme ne crée pas de capabilities, de supervision de santé, de reprise automatique ni d’autorité de publication.

## Décision d’architecture

Le registre Foundation précédent nettoie une entrée stale lors d’une recherche ou d’une tentative de réinscription. Cette tranche rend le retrait immédiat et contrôlé par propriétaire. `SYS_SERVICE_UNREGISTER(name)` ne retire que le nom détenu par la tâche Ring 3 appelante. Les chemins `SYS_EXIT` et `SYS_KILL` appellent `service_registry_remove_pid(pid)` avant de marquer ou de délier la tâche : une recherche ultérieure ne peut donc pas pointer vers une tâche déjà retirée.

| Événement | Garantie |
|---|---|
| Inscription | Le nom est associé au PID de l’appelant, pas à une valeur fournie par l’utilisateur |
| Retrait explicite | Seul le propriétaire Ring 3 peut désinscrire son propre nom |
| `exit` | Toutes les inscriptions du processus courant sont retirées avant la terminaison |
| `kill` | Toutes les inscriptions de la cible sont retirées avant son retrait de la file de tâches |
| Recherche | La purge lazy précédente reste une défense supplémentaire contre une entrée incohérente |

## ABI

| Syscall | Numéro | Argument | Retour |
|---|---:|---|---|
| `SYS_SERVICE_UNREGISTER` | 27 | `const char *name` | `0` ou erreur négative |

`MAX_SYSCALLS` devient 28. Les numéros existants, l’IPC et le protocole VFS restent inchangés.

## Validation

Les tests Unity couvrent le retrait propriétaire, le refus d’un retrait par un autre PID et le nettoyage par PID. Le contrat QEMU lance `vfsserver`, vérifie la lecture nommée, termine le serveur avec `kill`, puis exige que `vfs-read hello.txt` annonce explicitement que le service est indisponible. Les suites AOS, IPC et VFS sont relancées sans régression.

## Limites et suite

Le registre reste volatile et toute tâche peut publier un nom libre. Un service arrêté volontairement ne notifie pas ses clients ; le client doit encore traiter `OS_SERVICE_NOT_FOUND`. Des capabilities de publication/découverte, des réponses corrélées et une supervision de disponibilité restent nécessaires avant de s’approcher d’un cycle de vie de service microkernel.
