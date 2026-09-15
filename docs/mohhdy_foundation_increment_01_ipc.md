# MOHHDY Foundation — incrément 01 : IPC local borné

**Statut :** première tranche livrée, compatible avec MOHHDY.

**Portée MOHHDY :** prérequis réduit de US-001 (microkernel/IPC), US-003 (contrôle d’accès) et US-013 (communication inter-services).

**Non-objectif :** cette livraison ne transforme pas encore MOHHDY en microkernel.

## Décision d’architecture

La migration directe des services de fichiers, réseau et pilotes vers des processus séparés détruirait l’ABI et le chemin de boot actuellement testés. La première étape introduit donc le mécanisme qui manque au prototype : une **boîte aux lettres FIFO par tâche utilisateur**, gérée par le noyau et accessible par deux syscalls. Les services restent temporairement dans le noyau ; le transport est séparé et peut ensuite être utilisé lors de leur externalisation.

Cette approche reprend uniquement les propriétés minimales utiles du modèle d’endpoint : messages courts, files d’attente bornées et identité de l’émetteur attribuée par le noyau. Les microkernels L4/seL4 utilisent des endpoints pour des transmissions IPC bornées et synchrones ; leur modèle distingue le mécanisme noyau de la politique portée par les services [1] [2]. MOHHDY démarre avec une variante **asynchrone non bloquante**, plus facile à intégrer à son ordonnanceur actuel.

| Élément | Contrat de l’incrément |
|---|---|
| Destinataire | Tâche utilisateur existante et non terminée, sélectionnée par PID |
| Identité de l’émetteur | PID de `current_task`, jamais fourni par l’appelant |
| File | FIFO fixe de 4 messages par tâche |
| Charge utile | 96 octets au maximum, copiés par valeur dans le noyau |
| Envoi quand la file est pleine | Échec explicite, sans écrasement de message |
| Après un envoi réussi | Handoff coopératif vers une autre tâche utilisateur prête, pour livrer avant le prochain `SYS_GETS` du shell |
| Réception quand la file est vide | Échec explicite non bloquant |
| Accès noyau | Interdit via l’API syscall : seules les tâches Ring 3 sont des endpoints |
| Partage mémoire/capabilities | Hors périmètre de cette tranche |

## ABI proposée

Deux numéros sont ajoutés après `SYS_GPT2_GENERATE` sans modifier les numéros existants.

| Syscall | Registres | Retour |
|---|---|---|
| `SYS_IPC_SEND` (23) | `EBX = pid cible`, `ECX = os_ipc_payload_t*` | `0` ou erreur IPC négative |
| `SYS_IPC_RECV` (24) | `EBX = os_ipc_message_t*` | `0` ou `OS_IPC_EMPTY` |

`os_ipc_payload_t` contient seulement le type et les octets transmis. `os_ipc_message_t`, reçu par le destinataire, ajoute le PID d’émetteur construit par le noyau. Cette séparation interdit à un client de revendiquer l’identité d’une autre tâche.

| Code | Sens |
|---|---|
| `OS_IPC_EMPTY` | Aucune donnée à lire ; le destinataire reste exécutable |
| `OS_IPC_FULL` | File du destinataire saturée ; aucun message n’est perdu |
| `OS_IPC_BAD_TARGET` | PID absent, tâche noyau ou tâche terminée |
| `OS_IPC_BAD_MESSAGE` | Pointeur nul ou taille supérieure à 96 octets |

## Démonstration et non-régression

Le shell ajoute `ipc-send <pid> <texte>` et `ipc-recv`. Le programme initrd `ipcserver` appelle `ipc-recv` dans une boucle, affiche le PID et le contenu reçus, puis cède le processeur. Le contrat QEMU lance ce serveur, envoie un message depuis le shell et vérifie la réception. Les tests C vérifient l’ordre FIFO, la conservation de l’émetteur, la saturation et le rejet de cible invalide.

Les systèmes de fichiers, GPT-2, overlay, préemption IRQ0 et syscalls existants restent inchangés et sont revalidés par `make test-all` et `make integration-qemu`.

## Limites et prochain incrément

Une boîte aux lettres fixe n’apporte pas l’isolation d’un microkernel : le VFS, l’ATA, le GPT-2 et les pilotes s’exécutent toujours en Ring 0. Le handoff est nécessaire parce que le shell attend les caractères dans `SYS_GETS`, cadre Ring 0 que le quantum IRQ0 ne préempte volontairement pas. L’incrément suivant pourra construire un **service VFS Ring 3** autour de l’IPC, avec une politique d’autorisation explicite et un modèle d’attente repensé. Un IPC bloquant, des réponses corrélées et des capabilities déléguées sont également reportés.

## Références

[1] [seL4 FAQ — définition d’un microkernel et séparation mécanisme/politique](https://sel4.systems/About/FAQ.html)

[2] [seL4 IPC tutorial — endpoints, files de threads et messages bornés](https://docs.sel4.systems/Tutorials/ipc.html)
