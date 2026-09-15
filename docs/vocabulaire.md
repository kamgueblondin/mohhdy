# Vocabulaire Mohhdy

**Role :** termes a utiliser dans la documentation vivante. **Mohhdy** est **un seul produit** : le SE agentique autonome. L'ancien nom produit `AI-OS` / `mohhos` n'est plus utilise. Ce n'est pas une distribution, pas un clone Unix et pas un noyau Linux.

En cas de doute sur le **guest i386**, [ETAT_REEL.md](ETAT_REEL.md) decrit le comportement reel. Ce lexique ne cree aucune fonction.

## Produit (unitaire)

| Dire | Eviter comme architecture produit |
|---|---|
| SE Mohhdy, OS agentique autonome, instance Mohhdy | Trois couches produit, piste parallele, track Agent Support comme produit distinct |
| Navigateur-OS du SE | Widget SaaS a cote du noyau, vision boltee optionnelle |
| Docker / PC / hyperviseur = deploiement du SE (machine vierge) | Docker = sidecar Python, produit de support a cote |
| `agent/` = scaffold userspace actuel (bootstrap) | `agent/` = produit Agent Support, SaaS tiers |
| Prototype guest (tranche noyau verifiee sous QEMU) | "Le SE autonome complet tourne deja dans le guest i386" |
| Capacites OS (`ASSIST-xxx`) | Piste parallele, 9e phase, produit distinct |

Deux **niveaux de maturite runtime** (ingenierie, pas deux produits) : le prototype guest i386 mesure dans ETAT_REEL, et l'instance OS autonome (Docker / PC / hyperviseur / metal nu) dont `agent/` est le bootstrap userspace.

## Identite du guest i386

| Dire (guest) | Eviter (confusion Linux) |
|---|---|
| Prototype guest i386 32-bit, Multiboot, QEMU | Distribution Linux, " Linux embarque ", GNU/Linux |
| Noyau freestanding Ring 0, shell ELF Ring 3 | Userland GNU, glibc, systemd, apt |
| Invité QEMU (guest) / hôte de compilation | " MOHHDY tourne sous Linux " (seul l'émulateur tourne sur l'hôte) |
| Syscalls `int 0x80`, ABI propre | POSIX comme objectif, " compatible Linux " |
| Tâche, PID local, filiation directe | Processus Unix, `waitpid`, signaux, groupes de processus, zombies |
| Médiateur de chemins `vfsserver` (nom historique **VFS**) | VFS Linux, inodes, droits Unix, points de montage hiérarchiques |

Le nom **VFS** reste celui du code (`vfsserver`, `SYS_VFS_*`). Il désigne un médiateur Ring 3 de chemins et de sources, pas le VFS d'un noyau Unix.

## Stockage

| Dire | Éviter |
|---|---|
| Archive **TAR** (ustar) en lecture seule dans l'initrd | " système de fichiers POSIX ", " FS Linux " |
| Overlay **AIOV** V2 : snapshot ATA PIO, 64 nœuds, 384 octets | ext2, inode, journal |
| Volume **FAT16** lecture seule (LBA 64) | ext2, ext4, " FS Unix ", écriture FAT déjà livrée |
| Secteur, cluster, table d'allocation, entrée de répertoire | inode, superbloc Unix, dentry Linux |
| Disque IDE émulé, ATA PIO LBA28 | `/dev/sda`, block layer Linux |

FAT est un format de **volume** (table d'allocation + répertoires). Il n'est pas choisi pour ressembler à Linux : il est plus simple à poser sur ATA PIO qu'un système à inodes, et il se prépare avec des outils d'image hors de l'OS.

Détail de conception : [aos_fat_volume.md](aos_fat_volume.md).

## Inférence et réseau

| Dire | Éviter |
|---|---|
| GPT-2 local dans le noyau, initrd, sans réseau au boot | Service hôte, Ollama, " petite distro qui lance Ollama " |
| Sonde GGUF v3, kernels Q3_K/Q4_K/Q6_K, génération shell encore FP32 | " n'importe quel modèle ", inférence GGUF déjà branchée, intégration Ollama |
| Stub OpenAI / `net-status json` | Client HTTP déjà présent, " la NIC QEMU suffit " |
| Pilote NE2000 ISA + codecs caller-owned | Pile TCP/IP live, DHCP automatique, TLS/HTTPS |

Ollama et les services d'inférence d'un OS hôte ne font pas partie de MOHHDY. La seule voie retenue est un moteur porté dans ce noyau.

## Hôte de travail

Linux, macOS ou Windows n'apparaissent que comme **machine de compilation et d'émulation** (`make deps`, QEMU). Ils ne définissent ni le modèle de fichiers, ni l'ABI, ni l'identité de MOHHDY.
