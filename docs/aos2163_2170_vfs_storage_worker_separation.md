# AOS-2163...2170 - Separation storage-worker des I/O de montages proteges

**Statut : livre localement (increment Tranche 4).** Ce macro-lot pousse la
migration microkernel deja entamee : les lectures, `stat`, listages, pages et
observations des quatre montages proteges (`initrd/`, `overlay/`, `fat16/`,
`fat32/`) passent desormais par le worker Ring 3 `vfsvirtual`, sous la meme
capacite temporaire droit-source-prefixe que les alias dynamiques.

> Les pilotes ATA / FAT / overlay restent en Ring 0. Le mediateur `vfsserver`
> n'appelle plus lui-meme les syscalls backend pour ces I/O lorsqu'un worker
> de confiance est publie. Ce n'est pas "microkernel termine" ni US-001 complet.

## Critere de sortie (PLAN_SUITE Tranche 4)

- Le chemin ATA / FAT n'est plus le backend noyau unique et opaque du mediateur
  pour les I/O de lecture des montages proteges : un premier pas mesurable est
  livre avec contrat QEMU.
- Capacites droit-source-prefixe conservees. Pas de rejeu. Diagnostic public
  toujours sans prefixe.
- `make qemu-vfs-service` et `make qemu-ipc-foundation` restent verts.

## Architecture

| Element | Role | Limite |
|---|---|---|
| Shell | Envoie `OS_IPC_VFS_READ` / `STAT` / `LIST*` a `vfs` | Aucun acces backend |
| `vfsserver` | Politique, montages, grant temporaire, reponse publique | Miroir ; pas d'I/O locale si worker OK |
| `vfsvirtual` | Autorite table boot+alias ; syscall backend sous grant | Uniquement depuis PID `vfs` |
| Noyau | IPC, ACL prefixe, ATA/FAT/overlay | Pilotes encore Ring 0 |

Traces attendues pour un montage protege :

```text
vfsserver delegated storage read
vfsvirtual storage read initrd/hello.txt
```

Les alias dynamiques conservent les traces `delegated alias ...`.

## Preuves

```text
make -s -C userspace all
make -s test-all
make -s qemu-vfs-service
make -s qemu-ipc-foundation
```

Le contrat QEMU exige la delegation storage sur `initrd/`, `overlay/` et
`fat16/` (list/read/stat) en plus des cycles alias et FAT deja presentes.

## Correctif connexe

Le worker utilisait `SYS_FAT16_LIST_PATH` / `SYS_FAT32_LIST_PATH` sans placer
le buffer `os_dirent_t*` dans ECX (capacite seule). Les `stat`/listes de
sous-repertoires proteges echouaient alors en `NOT_MOUNTED`. ABI alignee sur
`vfsserver`.

## Limites explicites

- Aucune externalisation du pilote ATA lui-meme (secteurs, PIO, DMA).
- Les mutations fixes etaient deja deleguees ; ce lot aligne les I/O de lecture.
- Pas de shared memory, pas de multi-requetes worker, pas US-010 / US-016.
- Sans worker de confiance, le repli local historique `*_mounted_backend` reste.

## References

- [PLAN_SUITE Tranche 4](PLAN_SUITE_IMPLEMENTATION.md)
- [ETAT_REEL](ETAT_REEL.md)
- [Worker Ring 3](../userspace/vfs_virtual_worker.c)
- [Mediateur VFS](../userspace/vfs_server.c)
- [Contrat QEMU VFS](../tests/integration/test_qemu_vfs_service.py)
