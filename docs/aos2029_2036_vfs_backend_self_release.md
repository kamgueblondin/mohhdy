# AOS-2029 à AOS-2036 — Libération autonome des capacités backend VFS

## Objet

Le registre de services permettait déjà au propriétaire de déléguer puis de révoquer une capacité backend VFS. Ce macro-lot ajoute la **libération autonome** : le bénéficiaire d’un droit peut y renoncer de lui-même, sans connaître ni fournir l’identité du propriétaire et sans pouvoir cibler le droit d’un autre processus.

| Élément | Contrat livré |
|---|---|
| Registre statique | `service_registry_backend_release(name, grantee_pid)` retire l’unique entrée dont le nom et le bénéficiaire correspondent, efface tous ses champs puis incrémente la génération du service. |
| Identité vérifiée | Le syscall reçoit uniquement le nom de service. Il dérive le PID bénéficiaire de `current_task->id`, après contrôle que l’appelant est une tâche Ring 3. |
| Isolation | Aucun PID cible n’est accepté par l’ABI de libération ; une tâche ne peut donc pas libérer la capacité d’un autre bénéficiaire. |
| Observabilité | La génération backend progresse après libération ; une observation utilisant l’ancienne génération devient obsolète selon le contrat existant. |
| Ring 3 | Le programme `vfsreleaseclaim` attend un accès VFS, confirme la lecture, appelle `SYS_SERVICE_BACKEND_RELEASE` pour `vfs`, puis signale la libération. |
| IPC | La réception `vfs-read` conserve les messages non corrélés et attend au plus huit tours coopératifs : cette borne couvre la réponse VFS après un message différé sans allocation ni attente bloquante. |

> La révocation propriétaire demeure inchangée. La libération autonome est une renonciation du bénéficiaire : elle réduit ses propres droits et ne constitue ni un transfert de propriété ni une élévation de privilège.

## ABI et chemin d’exécution

`SYS_SERVICE_BACKEND_RELEASE` est ajouté au numéro `115`. Son registre `EBX` contient seulement le nom NUL-terminé du service. Le noyau associe alors ce nom à l’identité de la tâche appelante, et transmet ce PID au registre de services ; le programme utilisateur n’envoie jamais son propre PID comme argument de confiance.

Le chemin est volontairement borné. Le registre parcourt au plus `SERVICE_REGISTRY_BACKEND_CAPACITY` entrées statiques, efface l’entrée correspondante et publie une nouvelle génération. Aucun tas dynamique, aucun jeton opaque et aucune copie non bornée ne sont introduits.

## Validation

| Niveau | Vérification | Résultat |
|---|---|---|
| Unity du registre | Délégation de lecture, libération par le bénéficiaire, refus de seconde libération, rejet de nom invalide, génération passée de 1 à 3. | Réussi. |
| QEMU VFS | `vfsreleaseclaim` est lancé, reçoit la capacité, lit via VFS, s’auto-libère et le propriétaire observe ensuite une capacité absente. | Réussi. |
| IPC différé | Le scénario conserve un message non VFS avant une lecture corrélée et valide que la réponse VFS est reçue. | Réussi. |
| Suite complète | `make -s test-all` reconstruit et exécute tous les tests Unity et robustesse. | 482/482 réussis. |
| Noyau i386 | `make -s kernel-only` compile le dispatch syscall et les modules freestanding. | Réussi. |
| Hygiène | `git diff --check` et recherche d’allocations dynamiques dans les fichiers concernés. | Réussis. |

## Portée et suites

Cette livraison ferme la partie **révocation indépendante et identité de demandeur vérifiée** de l’axe des capacités. Les capacités restent des entrées statiques localement valides tant que la tâche et le service existent. Le routage général des réponses discordantes ainsi que l’externalisation complète des backends de chemins restent des chantiers distincts ; ils ne sont pas revendiqués par ce lot.

## Références

[1] [Registre de services et capacités backend](../kernel/service_registry.c)

[2] [Contrat du registre](../kernel/service_registry.h)

[3] [ABI Ring 3 / noyau](../include/os_syscalls.h)

[4] [Relais des appels système](../kernel/syscall/syscall.c)

[5] [Client Ring 3 de libération](../userspace/vfs_backend_release_claim.c)

[6] [Test unitaire du registre](../tests/unit/kernel/test_service_registry.c)

[7] [Contrat QEMU VFS](../tests/integration/test_qemu_vfs_service.py)
