# AOS-1553 à AOS-1560 — préparation statique d’un contexte de génération GGUF

## Objectif

Ce lot prépare un modèle GPT-2 GGUF pour le futur chemin de génération locale sans interpréter de métadonnées de configuration non fiables ni réserver de mémoire. Il introduit `gpt2_gguf_generation_prepare`, qui produit un contexte de génération composé de descripteurs de tenseurs et d’une table de couches détenue par l’appelant.

## Contrat préparé

| Élément | Résolution | Invariant vérifié |
|---|---|---|
| Embeddings de tokens | `token_embd.weight` | tenseur dense F32/F16 `[C, V]` |
| Embeddings de position | `position_embd.weight` | tenseur dense F32/F16 `[C, T]` |
| Normalisation finale | `output_norm.weight` et `output_norm.bias` | deux vecteurs denses de longueur `C` |
| Tête de sortie | `output.weight` | matrice Q3_K/Q4_K/Q6_K `[C, V]` |
| Couches | `blk.0` à `blk.N` | dix rôles complets, contigus et valides |

Les dimensions **C** (canaux), **V** (vocabulaire) et **T** (contexte maximal) sont déduites des formes des embeddings, puis réconciliées avec la tête de sortie et les vecteurs de normalisation. Chaque axe est contrôlé avant conversion vers 32 bits.

> Les buffers de noms et de couches demeurent caller-owned. Le contexte préparé ne possède ni heap, ni copie de modèle, ni état global persistant.

## Validation

La suite `test_gpt2_gguf` construit maintenant un index synthétique complet de quinze tenseurs : cinq rôles globaux et les dix rôles de `blk.0`. Elle confirme la préparation d’un modèle de 256 canaux, quatre tokens et deux positions, ainsi que le rejet déterministe d’un format invalide pour un tenseur dense global.

La règle de build de cette suite lie désormais le loader réel, FAT16 et les kernels quantifiés : la couverture s’exécute donc contre l’implémentation de production du contrat de préparation.

## Limites restantes

La préparation résout et valide tous les descripteurs nécessaires, mais elle ne lit pas encore les embeddings et paramètres denses depuis FAT16 pour exécuter une séquence de tokens. Le prochain lot raccordera l’état préparé au chemin `embeddings → blocs transformeur → normalisation finale → logits → échantillonnage`.
