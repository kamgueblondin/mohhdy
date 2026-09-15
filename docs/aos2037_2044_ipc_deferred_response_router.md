# AOS-2037 à AOS-2044 — Routeur général de réponses IPC différées

## Objet

Les commandes VFS Ring 3 attendaient chacune leur réponse IPC avec une boucle locale. Ces boucles dupliquées reconnaissaient le type et l’identifiant de requête, mais extrayaient d’abord une correspondance sans vérifier systématiquement l’expéditeur. Ce macro-lot introduit un **routeur commun de réponses corrélées** et migre les seize chemins VFS concernés.

| Élément | Contrat livré |
|---|---|
| Corrélation complète | Une réponse est identifiée par le triplet **PID expéditeur, type, identifiant de requête**. |
| File différée | Une réponse homonyme mais issue d’un autre service est conservée, au lieu d’être consommée puis perdue. |
| Attente bornée | Le routeur effectue au plus huit tours `yield`/réception ; il ne bloque pas et n’alloue pas de mémoire. |
| Migration VFS | Les délégations backend, lecture, écriture, suppression, renommage, montages, répertoires, listes, pagination, observation et statut utilisent le même routeur. |
| Compatibilité | Les parseurs ABI spécialisés restent inchangés : ils reçoivent le message uniquement après la corrélation commune. |

> La conservation locale ne transforme pas l’IPC en file non bornée : `OS_IPC_DEFERRED_CAPACITY` demeure égale à quatre messages, soit la capacité de l’endpoint noyau correspondant.

## Conception

La primitive `os_ipc_deferred_take_matching_from()` recherche une entrée par expéditeur, type et identifiant, puis retire uniquement cette entrée en préservant l’ordre des autres messages. Le shell utilise ensuite `wait_ipc_reply()` pour chercher d’abord dans cette file, recevoir coopérativement si nécessaire, et remettre chaque message discordant dans la file statique.

Cette centralisation évite les divergences entre commandes. Un message de même type et de même identifiant provenant d’un mauvais émetteur reste disponible pour son destinataire ; il ne peut plus être assimilé à tort à une réponse VFS valide ni être perdu lors de l’attente suivante.

## Validation

| Niveau | Vérification | Résultat |
|---|---|---|
| Unité IPC | Deux messages de même type et identifiant, avec deux PID différents, sont extraits séparément sans perte. | Réussi. |
| Shell | La compilation Ring 3 vérifie les seize migrations vers le routeur commun. | Réussi. |
| QEMU VFS | Un message `deferred` est injecté, puis une lecture et un listage VFS corrélés s’exécutent avant `ipc-recv`, qui récupère toujours le message d’origine. | Réussi. |
| Suite complète | `make -s test-all` reconstruit et exécute les tests Unity et robustesse. | 483/483 réussis. |
| Noyau i386 | `make -s kernel-only` est compilé après les changements. | Réussi. |
| Hygiène | `git diff --check` et recherche d’allocations dynamiques dans les fichiers modifiés. | Réussis. |

## Portée et suites

Ce lot ferme l’élément **routage général des réponses discordantes** de l’axe des capacités. La suite cohérente est l’externalisation d’un backend de chemins, puis les axes plus larges de migration microkernel, de quantification de latence GGUF et de réseau utilisateur effectif.

## Références

[1] [File IPC différée et corrélation par expéditeur](../include/os_ipc_deferred.h)

[2] [Routeur Ring 3 et commandes VFS migrées](../userspace/shell.c)

[3] [Tests unitaires de la file différée](../tests/unit/kernel/test_ipc_deferred.c)

[4] [Contrat QEMU VFS](../tests/integration/test_qemu_vfs_service.py)
