# MOHHDY Foundation — incrément 02 : médiateur VFS Ring 3

**Statut :** conception de la tranche suivante, fondée sur l’IPC Foundation livré.

**Portée MOHHDY :** premier jalon de l’extraction progressive du VFS prévue par US-001, avec un protocole minimal de communication inter-services de US-013.

**Non-objectif :** le backend initrd/overlay/ATA ne quitte pas encore Ring 0.

## Décision d’architecture

Le premier service externalisé ne doit pas réimplémenter un système de fichiers : il doit exercer une frontière client–serveur observable. `vfsserver`, processus Ring 3, reçoit une demande de lecture via son endpoint IPC, appelle l’ABI de lecture déjà stable, puis renvoie une réponse structurée au PID émetteur. Le shell devient le client et n’appelle plus directement `SYS_READFILE` dans ce scénario.

Cette forme client–serveur s’aligne sur le motif d’échange requête/réponse des microkernels, où un endpoint transporte de petites données entre un client et un serveur [1]. MOHHDY ne possède pas encore les reply capabilities, l’attente bloquante, le registre de services ou l’isolation d’un espace VFS : le client passe donc explicitement le PID du serveur et lit sa réponse dans sa propre boîte aux lettres.

| Élément | Contrat de l’incrément |
|---|---|
| Serveur | Programme Ring 3 `vfsserver`, lancé par `spawn vfsserver` |
| Client | Commande shell `vfs-read <pid> <chemin>` |
| Requête | Type `OS_IPC_VFS_READ`, chemin NUL-terminé de 47 octets utiles au plus |
| Réponse | Type `OS_IPC_VFS_READ_REPLY`, statut signé, longueur et au plus 80 octets lus |
| Routage | Le serveur répond au `sender_pid` fourni par le noyau, jamais à une identité issue de la charge utile |
| Sécurité initiale | Rejet des chemins vides, tronqués ou contenant `..` ; aucune écriture dans cette tranche |
| Capacité | Une réponse tient dans les 96 octets d’un message IPC ; les fichiers plus longs sont tronqués à 80 octets |

## ABI de protocole

Le protocole est défini dans `include/os_vfs_service.h`, partagé entre le shell et le serveur. Il ne modifie pas les numéros de syscall : le service s’appuie uniquement sur `SYS_IPC_SEND`, `SYS_IPC_RECV` et `SYS_READFILE`.

| Type IPC | Charge |
|---|---|
| `OS_IPC_VFS_READ` | `os_vfs_read_request_t { char path[48]; }` |
| `OS_IPC_VFS_READ_REPLY` | `os_vfs_read_reply_t { int32_t status; uint32_t size; uint8_t data[80]; }` |

Le serveur renvoie une réponse même lorsque la lecture échoue, à condition que la requête soit structurellement valide. Une réponse n’est pas corrélée par identifiant de requête : cette version cible une interaction shell–serveur à la fois. La prochaine version devra ajouter une corrélation, une file de réponses et un registre d’endpoints.

## Démonstration et tests

Le contrat QEMU lance `vfsserver`, demande `hello.txt` avec `vfs-read`, exige une ligne `vfs-read ok` contenant la donnée attendue, puis contrôle le retour du shell. Les tests C du protocole valident l’absence de dépassement, la forme des messages et les rejets de chemins non sûrs.

Les validations globales restent `make test-all` et `make integration-qemu`.

## Limites et suite

`vfsserver` est un **médiateur de politique**, non un vrai VFS isolé : le syscall de lecture continue d’accéder aux implémentations noyau initrd et overlay. Un processus malveillant conserve aussi la possibilité d’appeler `SYS_READFILE` directement. L’étape suivante devra introduire des droits par endpoint, retirer progressivement l’ABI de fichiers directe des clients ordinaires et déléguer un backend sûr au service.

## Références

[1] [seL4 IPC tutorial — communication client–serveur sur endpoint](https://docs.sel4.systems/Tutorials/ipc.html)
