# AOS-1537 à AOS-1544 — premier chemin d’exécution GGUF : données denses et logits quantifiés

## Périmètre

Les modules GGUF savaient déjà indexer un modèle GPT-2, relire des lignes Q3_K/Q4_K/Q6_K depuis FAT16 et calculer des produits scalaires. Le chaînon manquant était l’interface entre les tenseurs denses du modèle — embeddings, normalisations et biais — et la projection quantifiée de la tête de sortie.

Ce lot livre ce premier chemin exécutable. Il fournit un décodeur de ligne dense F32/F16 borné et une primitive de calcul des logits d’une tête de sortie Q3_K/Q4_K/Q6_K depuis un état caché fourni par l’appelant.

## Contrats ajoutés

| Primitive | Entrées | Sortie | Garanties |
|---|---|---|---|
| `gpt2_gguf_read_dense_row_fat16` | tenseur F32/F16, ligne, scratch | vecteur `float` | lecture FAT16 bornée, décodage little-endian, aucun heap |
| `gpt2_gguf_forward_output_logits_fat16` | état caché, matrice `[C,V]` Q3_K/Q4_K/Q6_K | `V` logits | une lecture de ligne par logit, contrôle des dimensions et capacités |

Les deux fonctions exigent des buffers caller-owned. Les capacités de sortie sont exprimées en octets, comme les primitives de projection existantes. Les tailles `largeur × sizeof(float)` sont contrôlées avant toute multiplication afin d’empêcher un contournement par débordement sur des métadonnées GGUF non fiables.

## Chemin de données

```text
FAT16 -> index GGUF borné -> tenseur dense F32/F16 -> état caché caller-owned
                                              \-> matrice Q3_K/Q4_K/Q6_K -> logits caller-owned
```

La matrice de sortie respecte la forme GGUF `[channels, vocabulary]`. L’implémentation délègue le calcul de chaque ligne au kernel Q3_K, Q4_K ou Q6_K existant ; aucun tenseur complet n’est chargé dans le heap ou copié dans une zone dynamique.

## Validation

La fixture FAT16/GGUF existante couvre désormais les situations suivantes :

| Cas | Résultat attendu |
|---|---|
| Ligne F32 de largeur 1 | décodage réussi depuis le fichier GGUF |
| Ligne F16 de largeur 1 | conversion freestanding réussie |
| Tête Q4_K de forme `[256, 2]` avec activation nulle | deux logits nuls calculés depuis FAT16 |
| Scratch ou sortie insuffisante | rejet contrôlé |
| Dimensions ou type incompatibles | rejet contrôlé |

Le chemin n’est pas encore une génération GPT-2 quantifiée complète : il reste à chaîner l’extraction des embeddings et positions, les blocs attention/MLP pour toutes les couches, la normalisation finale, la sélection de token et le raccordement shell. Les briques déjà disponibles et ce lot permettent désormais d’assembler ce travail sans allocation dynamique.
