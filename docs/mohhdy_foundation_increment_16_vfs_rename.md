# MOHHDY Foundation — Incrément 16 : renommage VFS médié

> **Statut : livré et vérifié localement.** L’incrément 16 ajoute un renommage VFS corrélé, réservé au propriétaire vivant de `vfs` et limité à deux chemins appartenant au montage `overlay/`.

## Objectif et contrat

Après les opérations VFS de lecture, écriture et suppression, le renommage complète le cycle de vie minimal d’un fichier overlay sans exposer directement le backend noyau au shell. La commande publique est :

```text
vfs-rename overlay/note.txt overlay/moved.txt
```

| Élément | Contrat |
|---|---|
| ABI noyau | `SYS_VFS_OVERLAY_RENAME = 35` ; l’ABI couvre 0–35 et `MAX_SYSCALLS = 36`. |
| Autorisation | Le noyau accepte uniquement le PID utilisateur vivant publié sous le nom `vfs`. |
| Requête | `OS_IPC_VFS_RENAME` contient deux chemins de 48 octets, soit exactement 96 octets de charge IPC, avec un `request_id`. |
| Réponse | `OS_IPC_VFS_RENAME_REPLY` contient un statut de quatre octets et le même identifiant de corrélation. |
| Politique | La source **et** la destination doivent correspondre au même montage écrivable `overlay/`. |

Le médiateur retire les deux préfixes `overlay/` et appelle `overlay_rename` uniquement avec leurs suffixes relatifs. Une source ou destination sous `initrd/`, une racine de montage, un chemin vide ou un chemin qui contient `..` produit un refus de politique avant l’accès backend.

## Surface utilisateur et vérification

Le shell expose `vfs-rename <source> <destination>`. La sonde `vfs-backend-rename-probe <source> <destination>` doit retourner `denied` depuis le shell, démontrant que le backend réservé ne peut pas être contourné. Le contrat QEMU vérifie successivement le refus direct, le refus inter-montage, l’écriture, la lecture, le renommage, l’absence de l’ancien nom, la lecture du nouveau nom, la suppression et l’absence finale.

Deux tests Unity valident l’encodage et le décodage de la requête de 96 octets, les chemins invalides, les réponses discordantes et le statut corrélé. La suite complète compte **198 tests**.

Le test de transfert de service effectue désormais une cession explicite (`yield`) après le grant avant d’exiger la réaction de `serviceclaim`. Cette cession est nécessaire au contrat d’ordonnancement coopératif de l’OS et n’assouplit pas les assertions de notification, de revendication ou de purge.

```bash
make test-all          # 198/198
make integration-qemu  # six contrats QEMU
make ci
```

## Limites honnêtes

Le renommage ne fournit ni transaction, ni atomicité multipath au-dessus de l’overlay existant, ni capability, identité vérifiée, contrôle d’accès par chemin, verrouillage, journalisation durable, rollback, renommage inter-montage, montages dynamiques ou externalisation du backend ATA. Les opérations de fichiers historiques restent disponibles hors de la politique VFS et l’initrd demeure en lecture seule.
