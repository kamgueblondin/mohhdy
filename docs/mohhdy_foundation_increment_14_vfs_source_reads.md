# MOHHDY Foundation — Incrément 14 : lectures VFS source-spécifiques

> **Statut : livré et vérifié localement.** L’incrément 14 fait correspondre chaque préfixe VFS déclaré à une primitive de lecture backend distincte, toutes deux réservées au propriétaire courant du service `vfs`.

## Motivation

Avant cet incrément, la lecture backend réservée appelait le chemin de lecture historique, qui cherchait l’overlay puis l’initrd. Cette recherche implicite était pratique mais ne permettait pas d’affirmer que `initrd/` et `overlay/` représentaient des sources réellement séparées. Une entrée `initrd/nom` pouvait donc, selon le contenu de l’overlay, ne pas refléter strictement la source attendue.

| Montage VFS | Droit | Primitive noyau réservée | Source réellement lue |
|---|---:|---|---|
| `initrd/` | `ro` | `SYS_VFS_INITRD_READ` (32) | archive initrd uniquement |
| `overlay/` | `rw` | `SYS_VFS_OVERLAY_READ` (33) | overlay ATA uniquement |

L’ABI couvre désormais les numéros **0 à 33** avec `MAX_SYSCALLS = 34`. Les deux primitives vérifient que l’appelant est une tâche utilisateur vivante et le PID publié sous le nom `vfs`. Elles refusent le shell et tout ancien propriétaire après transfert de service.

## Routage du médiateur

`vfsserver` déclare une table locale de montages de lecture composée d’un préfixe et d’un identifiant de source. Après validation du chemin, il retire le préfixe et transmet seulement le suffixe au syscall de la source correspondante. La voie générique `SYS_VFS_BACKEND_READ` reste présente pour la compatibilité et les démonstrateurs existants, mais le médiateur ne l’emploie plus pour servir une lecture montée.

Ainsi, la séquence suivante est un contrat observable :

```text
spawn vfsserver
vfs-write overlay/note.txt vfsok
vfs-read overlay/note.txt
```

La dernière commande reçoit le contenu depuis l’overlay via la réponse `OS_IPC_VFS_READ_REPLY` corrélée. Une lecture `vfs-read initrd/hello.txt` reste servie par l’initrd, sans rechercher de remplacement dans l’overlay.

## Tests et stabilité QEMU

Le contrat VFS QEMU utilise un disque overlay isolé. Il vérifie les refus de backend direct, le refus d’écriture sous `initrd/`, l’écriture médiée sous `overlay/`, puis la relecture de `overlay/note.txt` **par VFS**. Les tests conservent la corrélation IPC, la file différée, le transfert et la révocation de `vfs`.

Les contrats VFS et de transfert de service, ainsi que le smoke fournisseur IA, utilisent désormais un rythme clavier QEMU renforcé. Le changement ne relâche aucune assertion : il élimine seulement les doubles frappes PS/2 observées sous TCG.

```bash
make test-all
make integration-qemu
make ci
```

## Limites honnêtes

Les deux sources restent des composants noyau, sans capability, identité cryptographique, chiffrement, contrôle d’accès par chemin, montages dynamiques, transactions, verrouillage ou cache de pages. La table de montages est locale et statique dans `vfsserver`. L’IPC demeure non bloquant et la réponse de lecture reste limitée à 80 octets. La lecture générique historique et les opérations de fichiers directes ne sont pas supprimées par cet incrément.
