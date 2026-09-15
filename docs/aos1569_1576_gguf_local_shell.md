# AOS-1569…1576 — Exécution locale GPT-2 GGUF depuis FAT16

**Statut : livré et validé.** Ce macro-lot rend le profil `gpt2.gguf` exécutable depuis la commande `ai`, sans recopier les poids en mémoire et sans introduire d’allocation dynamique. Le modèle réel Q3_K est conservé sur un volume FAT16 et le noyau ne réside que le catalogue GGUF, le workspace de calcul et le cache KV borné.

> **Principe d’exécution.** Le disque de déploiement expose le modèle réel sous l’alias FAT16 8.3 `GPT2.GGU`. Le noyau indexe son catalogue dans une fenêtre statique de 2 MiB, puis lit les poids par fenêtres au fur et à mesure du forward.

## Périmètre livré

| Axe | Livraison | Garantie principale |
|---|---|---|
| Catalogue GGUF | Indexage à deux bornes : fenêtre d’en-tête chargée et taille logique complète du fichier | Aucun poids n’est copié avec le catalogue ; les offsets sont validés contre la taille FAT16 réelle. |
| Formats quantifiés | Déquantification caller-owned Q3_K, Q4_K et Q6_K pour les embeddings | Le modèle Q3_K réel, dont `token_embd.weight` est Q3_K et la tête de sortie Q6_K, est supporté. |
| Bloc GPT-2 | Mapping des douze rôles par couche, y compris les biais `ffn_up` et `ffn_down` | Le forward applique les biais MLP réels au lieu de les neutraliser. |
| Runtime local | État statique GPT-2 12 × 768, cache KV de 64 positions et top-k déterministe | Zéro `kmalloc`, `malloc`, `calloc` ou `realloc`. |
| E/S FAT16 | Curseur, saut par chaîne FAT, projections QKV/MLP/logits séquentielles | Une projection ne reparcourt plus la chaîne de clusters pour chaque ligne. |
| Shell et ABI | `SYS_GPT2_GGUF_GENERATE` (109), sélection par `ai-model use gpt2.gguf` | Le syscall FP32 historique reste inchangé. |
| Déploiement | `make gguf-disk` et image FAT16 contenant `GPT2.GGU` et `FATOK.TXT` | Le modèle reste hors initrd ; l’image est compatible avec les smokes existants. |

## Architecture mémoire et limites

Le catalogue GGUF occupe au plus **2 MiB**. Le modèle Q3_K de référence reste sur le disque FAT16 ; il n’est jamais chargé dans le heap ni copié intégralement. Le profil statique cible GPT-2 124M avec 12 couches, 768 canaux, 12 têtes, un vocabulaire maximal de 50 257 tokens et un cache de contexte de 64 positions.

| Ressource résidente | Bornage | Rôle |
|---|---:|---|
| Catalogue GGUF | 2 MiB | Métadonnées, descripteurs et offsets validés. |
| Cache KV | 12 × 64 × 2 × 768 floats | K/V persistants par couche pour le préfixe courant. |
| Workspace de bloc | 768 et 3 072 floats selon le buffer | Normalisations, QKV, attention, MLP et résidu. |
| Logits | 50 257 floats | Échantillonnage top-k déterministe. |
| Buffer de ligne | 630 octets | Plus grande ligne quantifiée Q6_K de 768 canaux. |

Le profil GGUF produit volontairement **un token par appel shell**. Cette borne évite de monopoliser le noyau pendant une génération longue sur un périphérique bare-metal à E/S FAT16. Le cache KV conserve le préfixe déjà calculé ; les appels suivants réutilisent donc les positions compatibles.

## Flux d’exécution

```text
ai-model use gpt2.gguf
        │
        ▼
SYS_GPT2_GGUF_GENERATE (109)
        │
        ├── Tokenizer BPE initrd
        ├── Vérification du profil GGUF FAT16 prêt
        ├── Réutilisation / reconstruction du préfixe KV
        ├── Embedding Q3_K ou dense par ligne
        ├── 12 blocs : QKV → attention KV → MLP avec biais
        ├── Normalisation finale
        ├── Tête Q6_K lue séquentiellement sur FAT16
        └── Top-k déterministe → détokenisation shell
```

Les projections QKV, attention, MLP et logits ouvrent un curseur FAT16, se positionnent sur le début du tenseur via la chaîne FAT, puis consomment les lignes dans l’ordre. Cette stratégie retire le coût pathologique d’un `fat16_read_file_range` depuis le début du fichier pour chaque ligne quantifiée.

## Construction et essais

La commande suivante construit un disque de déploiement d’environ 102 MiB à partir du modèle Q3_K local. Elle installe les poids sous `GPT2.GGU` et ajoute `FATOK.TXT` afin de rester compatible avec les diagnostics FAT16 standard.

```sh
make gguf-disk
```

Le test de bout en bout dédié reconstruit l’image, démarre QEMU, sélectionne le profil et attend un retour local du shell.

```sh
make qemu-gguf-smoke
```

Le smoke QEMU standard peut aussi démarrer sur ce disque sans le réécrire en définissant explicitement la conservation FAT16 :

```sh
MOHHDY_PRESERVE_FAT16=1 \
OVERLAY_DISK="$PWD/build/gpt2_gguf_fat16.img" \
make qemu-smoke DISK_IMAGE=build/gpt2_gguf_fat16.img
```

## Validation réalisée

| Vérification | Résultat |
|---|---|
| Compilation freestanding i386 | Réussie. |
| Unity complet | **452/452** tests verts, zéro test ignoré. |
| Indexage du modèle Q3_K réel | Catalogue de 149 tenseurs dans une fenêtre de 2 MiB. |
| Smoke QEMU standard sur disque de déploiement | Core, extras shell, persistance, spawn/yield/wait et exec validés. |
| Smoke `qemu-gguf-smoke` | Boot GGUF, sélection `gpt2.gguf` et retour local validés. |
| Analyse statique de la voie GGUF | Aucun appel à un allocateur dynamique. |

## Limites et suite

La livraison vise le profil GPT-2 124M GGUF quantifié, le format de fichier FAT16 8.3 et un contexte local de 64 tokens. Elle ne rend pas encore n’importe quel modèle GGUF ni n’importe quel tokenizer GGUF interchangeable : le tokenizer BPE GPT-2 fourni par l’initrd demeure le contrat de génération. La suite du backlog pourra généraliser les profils, exposer une politique de contexte plus riche et réduire davantage la latence par cache de pages de poids strictement statique.
