# MOHHDY Foundation — Incrément 06 : transfert limité de publication

## But

Le sixième incrément Foundation ajoute un droit **borné et nominatif** de transfert de publication. Le propriétaire actuel d’un nom de service peut le transférer à une tâche utilisateur Ring 3 vivante. Le bénéficiaire devient alors l’unique propriétaire pour les opérations suivantes : publication idempotente, retrait explicite et nettoyage automatique à sa terminaison.

> Il s’agit d’un **transfert unique de propriété de nom**, et non d’une capability générale ni d’une délégation révocable.

## Contrat ABI

| Élément | Contrat livré |
|---|---|
| Syscall | `SYS_SERVICE_GRANT = 28`, avec `MAX_SYSCALLS = 29` |
| Arguments | `EBX` pointe vers le nom, `ECX` contient le PID bénéficiaire |
| Autorisation | Seul le PID actuellement associé au nom peut le transférer |
| Bénéficiaire | Le noyau accepte uniquement une tâche Ring 3 existante et non terminée |
| Effet | Le registre remplace atomiquement le PID associé, sans créer une seconde entrée |
| Erreurs | `OS_SERVICE_NOT_OWNER` et `OS_SERVICE_BAD_GRANTEE` distinguent les refus |
| Exécution | Un handoff coopératif suit le transfert réussi lorsqu’une autre tâche utilisateur est prête |

Le handoff est nécessaire dans ce prototype : le shell retourne normalement vers `SYS_GETS`, qui reste en Ring 0 durant son attente clavier et ne peut pas être préempté par le quantum IRQ0 réservé aux cadres Ring 3. Il permet donc au bénéficiaire de constater immédiatement la propriété reçue.

## Parcours QEMU vérifié

Le shell publie d’abord `demo`, puis lance `serviceclaim`. Ce client constate que le nom est déjà détenu et cède le CPU. Le shell transfère ensuite `demo` au PID du client, qui publie `serviceclaim claimed demo`. La commande `service-find demo` résout ce PID. Enfin, `kill <pid>` retire automatiquement le nom et `service-find demo` annonce le service indisponible.

Les tests Unity couvrent le transfert autorisé, le refus par un non-propriétaire, le refus d’un bénéficiaire invalide et le nettoyage du bénéficiaire. Le sixième contrat QEMU couvre le parcours Ring 3 complet.

## Commandes shell

| Commande | Rôle |
|---|---|
| `service-publish <nom>` | Publie un nom au nom du shell appelant |
| `service-grant <nom> <pid>` | Transfère un nom détenu vers une tâche utilisateur vivante |
| `service-find <nom>` | Affiche le PID actuellement publié ou une indisponibilité |

## Limites et suite

| Limite | Conséquence |
|---|---|
| Transfert irrévocable par l’ancien propriétaire | Une fois transféré, seul le nouveau propriétaire peut retirer ou retransférer le nom |
| Nom unique et volatile | Aucun droit générique, hiérarchie, persistance ou journal d’audit n’est fourni |
| Découverte publique | Tout processus utilisateur peut toujours rechercher un nom et publier un nom libre |
| Pas de secret ni de capability | Un PID et un nom ne prouvent pas une identité de confiance |
| Handoff coopératif | Le comportement reste lié aux tâches utilisateur prêtes et ne crée pas d’IPC bloquant |
| Backend VFS noyau | Les pilotes, initrd, overlay et ATA ne sont pas déplacés hors du noyau |

Les étapes suivantes réalistes sont une politique de noms réservés, une révocation contrôlée ou des tokens intransférables. Elles nécessitent une réflexion séparée sur la protection de mémoire, l’identité des processus et l’audit ; ce prototype ne les simule pas.

## Fichiers concernés

| Fichier | Rôle |
|---|---|
| `include/os_syscalls.h` | Numéro de syscall et erreurs de transfert |
| `kernel/service_registry.[ch]` | Transfert atomique du propriétaire du nom |
| `kernel/syscall/syscall.[ch]` | Validation du bénéficiaire et handoff coopératif |
| `userspace/shell.c` | Commandes de publication, transfert et recherche |
| `userspace/service_claim.c` | Démonstrateur Ring 3 du bénéficiaire |
| `userspace/Makefile`, `Makefile` | Compilation et empaquetage initrd du démonstrateur |
| `tests/unit/kernel/test_service_registry.c` | Tests de propriété et nettoyage |
| `tests/integration/test_qemu_service_grant.py` | Contrat QEMU de transfert complet |
