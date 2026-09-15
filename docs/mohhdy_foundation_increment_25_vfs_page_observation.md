# Incrément Foundation 25 — Observation cohérente des pages VFS

## Objet

L’incrément 25 ajoute une observation de génération au listage VFS paginé. La commande `vfs-list-observe <repertoire/> <depart> <generation>` peut demander une page en indiquant la génération déjà observée. Le serveur retourne soit la page et la génération courante, soit un refus `obsolete` avec la génération qui a remplacé celle attendue.

| Élément | Contrat |
|---|---|
| Commande | `vfs-list-observe <repertoire/> <depart> <generation>` |
| Requête | `OS_IPC_VFS_LIST_OBSERVE` (`0x56465314`) |
| Réponse | `OS_IPC_VFS_LIST_OBSERVE_REPLY` (`0x56465315`) |
| Requête | Chemin 48 octets, index logique 32 bits, génération attendue 32 bits |
| Réponse | Statut, compte, index suivant, génération et 64 octets de noms, soit 80 octets |
| Statut d’obsolescence | `OS_VFS_STATUS_STALE` (`-64`) |

## Sémantique

La génération démarre à `1` à chaque lancement de `vfsserver`. Elle est incrémentée après une écriture, une suppression, un renommage ou une modification de table de montages effectivement réussie. Une requête dont la génération attendue vaut `0` accepte la génération courante et sert à prendre un premier instantané. Une valeur non nulle différente de la génération courante retourne `obsolete generation <n>` sans contenu de page.

Le routage reste local au médiateur Ring 3. Il retire le préfixe de montage et contacte exclusivement le backend initrd ou overlay déclaré. Cette génération est une indication de cohérence : elle ne verrouille pas l’overlay, ne crée pas de snapshot, ne persiste pas et ne protège pas contre un dépassement de compteur.

## Vérification et limites

La suite Unity contient un aller-retour corrélé OBSERVE couvrant le chemin, l’index, la génération attendue, la réponse fixe et le statut d’obsolescence. Elle totalise **208 tests**. Le contrat QEMU VFS vérifie l’observation initiale `initrd/`, sa page partielle, le curseur `4` et la génération `1`, puis poursuit les montages, mutations, statistiques, transfert et révocation existants.

Le protocole ne fournit ni transaction de répertoire, ni atomicité entre deux pages, ni tri, ni métadonnées par entrée, ni capability. Une mutation entre l’observation et une page suivante est détectée seulement si elle a traversé ce serveur VFS et incrémenté sa génération volatile.
