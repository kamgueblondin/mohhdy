# MOHHDY Foundation — incrément 03 : registre de services nommé

**Statut :** conception de la tranche suivante, fondée sur l’IPC et le médiateur VFS fusionnés.

**Portée MOHHDY :** jalon réduit de US-001, US-012 et US-013 : découverte d’un endpoint par un nom stable plutôt que par un PID codé en dur.

**Non-objectif :** ce registre n’est pas un espace de capabilities, un annuaire distribué ni une politique d’autorisation complète.

## Décision d’architecture

`vfsserver` est actuellement joignable seulement lorsque le client connaît son PID. Le registre introduit deux syscalls : `SYS_SERVICE_REGISTER` lie un nom au **PID de la tâche appelante**, et `SYS_SERVICE_LOOKUP` retourne le PID d’un service vivant portant ce nom. Le client VFS peut ainsi appeler `vfs-read hello.txt` : il résout d’abord `vfs`, puis utilise l’IPC déjà livré.

Dans les microkernels à capabilities, l’invocation vise une capability et l’identité de l’émetteur peut être portée par le mécanisme d’endpoint ; seL4 propose aussi un chemin d’appel/réponse spécifique [1]. MOHHDY ne possède encore ni capability dérivée, ni badge, ni `Call`/`Reply` bloquant. Le registre est donc une **découverte de nom de convenance**, explicitement moins sûre, qui ne délivre aucun droit d’accès en soi.

| Élément | Contrat de l’incrément |
|---|---|
| Capacité | 8 services nommés au plus |
| Nom | Chaîne NUL-terminée de 15 octets utiles au plus, alphanumérique, `_` ou `-` |
| Inscription | `SYS_SERVICE_REGISTER(name)` associe le nom au PID de `current_task` ; le PID n’est jamais fourni par l’utilisateur |
| Recherche | `SYS_SERVICE_LOOKUP(name)` retourne le PID utilisateur vivant ou une erreur explicite |
| Conflit | Un nom actif détenu par une autre tâche est refusé ; la réinscription du même propriétaire est idempotente |
| Nettoyage | Une entrée dont le PID est terminé ou absent est supprimée lors de recherche ou de tentative d’inscription |
| Première intégration | `vfsserver` enregistre `vfs`; `vfs-read <chemin>` résout le nom avant l’envoi IPC |

## ABI

| Syscall | Numéro | Arguments | Retour |
|---|---:|---|---|
| `SYS_SERVICE_REGISTER` | 25 | `const char *name` | `0` ou erreur négative |
| `SYS_SERVICE_LOOKUP` | 26 | `const char *name` | PID positif ou erreur négative |

Les erreurs dédiées sont `OS_SERVICE_BAD_NAME`, `OS_SERVICE_FULL`, `OS_SERVICE_TAKEN` et `OS_SERVICE_NOT_FOUND`. `MAX_SYSCALLS` devient 27. Le registre ne modifie pas le protocole IPC ni les numéros AOS existants.

## Démonstration et validation

Le contrat QEMU lance `vfsserver`, attend son inscription `vfs`, exécute `vfs-read hello.txt` sans PID et vérifie la réponse. Les tests Unity couvrent validation du nom, conflit actif, idempotence, capacité et réutilisation d’une entrée supprimée. `make integration-qemu` conserve les contrats AOS, IPC et VFS antérieurs.

## Limites et suite

Tout processus utilisateur peut encore tenter de réserver un nom libre : il n’existe pas de capability de publication, de manifeste signé, de contrôle d’identité ni de supervision de santé. Le registre n’est pas persistant et n’offre ni version ni plusieurs instances. Le prochain incrément devra introduire des droits d’enregistrement/découverte, la corrélation requête-réponse et le nettoyage à la fin d’une tâche, avant toute prétention de microkernel sécurisé.

## Références

[1] [seL4 API Reference — endpoints, sender badge et `Call`/`Reply`](https://docs.sel4.systems/projects/sel4/api-doc.html)
