# MOHHDY Foundation — Incrément 08 : politique VFS virtuelle Ring 3

## But

`vfsserver` ne se limite plus à transmettre toutes les lectures vers `SYS_READFILE`. Il fournit maintenant le chemin virtuel **`vfs-info`** depuis son propre espace Ring 3. La réponse `vfsserver ring3 policy` est construite dans le service, envoyée par IPC et corrélée au client sans appeler le backend fichiers noyau.

| Chemin demandé | Responsable effectif | Comportement |
|---|---|---|
| `vfs-info` | `vfsserver` Ring 3 | Réponse synthétique locale, sans `SYS_READFILE` |
| Tout autre chemin sûr | Backend noyau via `SYS_READFILE` | Compatibilité initrd et overlay existante |
| Chemin invalide | Validation du protocole VFS | Réponse structurée de refus |

## Validation

Le contrat QEMU VFS lance le serveur, conserve un message IPC concurrent, lit `hello.txt`, récupère ce message, lit ensuite `vfs-info` et exige à la fois la trace `vfsserver virtual vfs-info` et le contenu synthétique. Les 186 tests Unity et robustesse restent verts.

## Portée réelle

> Cette tranche externalise **une politique et une source virtuelle**, non pas le backend de stockage général.

Les sélections initrd/overlay, ATA PIO, la copie des données de fichier et `SYS_READFILE` restent dans le noyau. La source virtuelle n’est ni un point de montage général, ni un pilote, ni une isolation mémoire renforcée. L’étape suivante réaliste consiste à définir un protocole de backend distinct et une liste de chemins ou de montages détenue par le service, sans prétendre que le VFS noyau a déjà disparu.
