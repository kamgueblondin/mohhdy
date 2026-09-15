# MOHHDY Foundation — Incrément 17 : statistiques VFS volatiles

> **Statut : livré et vérifié localement.** L’incrément 17 rend l’activité du médiateur VFS observable depuis Ring 3, sans nouveau syscall, nouveau droit backend ni changement du protocole IPC.

## Contrat

La commande suivante interroge une source virtuelle exposée par le service publié `vfs` :

```text
vfs-stats
```

Elle réutilise exactement le chemin corrélé de `vfs-read vfs-stats`. Le serveur répond avec un texte borné :

```text
reads=<n>
writes=<n>
removes=<n>
renames=<n>
```

| Compteur | Incrémenté lorsque | Sémantique |
|---|---|---|
| `reads` | Une requête `OS_IPC_VFS_READ` reconnue arrive au serveur | Inclut les lectures réussies, les sources virtuelles et les chemins refusés. La lecture de `vfs-stats` elle-même est incluse. |
| `writes` | Une requête `OS_IPC_VFS_WRITE` reconnue arrive au serveur | Inclut les écritures autorisées et les refus de politique de montage. |
| `removes` | Une requête `OS_IPC_VFS_REMOVE` reconnue arrive au serveur | Inclut les suppressions autorisées, absentes ou refusées. |
| `renames` | Une requête `OS_IPC_VFS_RENAME` reconnue arrive au serveur | Inclut les renommages autorisés, inter-montage, invalides ou refusés. |

Les compteurs sont des entiers non signés de 32 bits statiques au processus `vfsserver`. Ils sont remis à zéro à chaque démarrage du serveur et sont incrémentés **avant** l’analyse ou la politique de chemin : les refus restent donc visibles, sans qu’un client Ring 3 puisse accéder au backend réservé.

## Vérifications

Le contrat QEMU VFS vérifie d’abord l’état frais (`reads=1`, autres compteurs à zéro, puisque l’interrogation se compte elle-même). Après les lectures, opérations mutables et refus déjà couverts, il vérifie `reads=10`, `writes=2`, `removes=2`, `renames=2`. Ces valeurs démontrent que les refus `initrd/` participent à l’observabilité, tout comme les opérations `overlay/` autorisées.

```bash
make test-all          # 198/198 tests Unity et robustesse
make integration-qemu  # six contrats QEMU, dont VFS avec vfs-stats
make ci                # build, suite C et smokes QEMU
```

La cadence de saisie du contrat VFS est passée à 0,55 s, celle du smoke QEMU cœur à 0,65 s et celle du smoke extras à 0,80 s afin de réduire les doubles frappes PS/2 observées sous QEMU TCG. Les assertions fonctionnelles elles-mêmes ne sont pas relâchées.

## Limites honnêtes

Ces statistiques ne sont ni persistantes, ni atomiques vis-à-vis de plusieurs clients, ni horodatées. Elles peuvent déborder après `2^32 - 1` opérations et n’exposent ni latence, volume de données, code d’erreur, identité de client, métrique de backend, agrégation inter-serveurs, export machine, historique ou alerte. Elles ne constituent pas une capability, un audit de sécurité ni une garantie de disponibilité.
