# AOS-1529 à AOS-1536 — workspace GPT-2 statique borné

## Objectif

Le moteur GPT-2 FP32 utilisait encore des appels `kmalloc` pour ses activations, ses logits et son cache clé/valeur. Cette dépendance violait la contrainte d’architecture de l’OS : le chemin d’inférence doit rester entièrement freestanding et sans allocation dynamique.

Ce lot remplace ces allocations à l’exécution par un workspace global statique. Il constitue également le socle mémoire réutilisable pour raccorder les primitives GGUF quantifiées déjà présentes à un forward complet.

## Capacités fixes

| Ressource | Borne compilée | Justification |
|---|---:|---|
| Contexte | 64 tokens | limite historique du runtime bare-metal |
| Canaux | 768 | GPT-2 124M |
| Couches | 12 | GPT-2 124M |
| Vocabulaire paddé | 50 304 | GPT-2 124M |
| Cache KV | `12 × 64 × 2 × 768` floats | clés et valeurs des 12 couches |
| MLP temporaire | `4 × 768` floats | expansion GPT-2 standard |

Les buffers résident dans la BSS du noyau. Leur existence ne dépend donc ni du heap ni de la réussite d’une réservation au moment du premier token.

## Invariants

La fonction d’initialisation vérifie les canaux, le nombre de couches et le vocabulaire avant de publier les pointeurs du workspace. Une configuration qui dépasse les bornes échoue avec le statut `GPT-2: configuration hors capacite statique` sans parcourir les poids, écrire les logits ou toucher le cache KV.

Le changement de configuration à l’intérieur d’une session prête reste explicitement refusé. Cet invariant préserve la cohérence du cache KV et évite de réutiliser un buffer avec une interprétation de forme différente.

> Aucun `kmalloc`, `malloc`, `calloc` ou `realloc` ne subsiste dans `gpt2_infer.c` après ce lot.

## Validation

Le nouveau binaire unitaire `test_gpt2_infer` construit un modèle GPT-2 synthétique caller-owned de forme minimale. Il exécute le forward sur le workspace statique, puis force une configuration à 769 canaux pour confirmer le rejet contrôlé. Les validations globales seront exécutées avant publication.

## Suite

Les kernels de lecture et de projection Q3_K, Q4_K et Q6_K sont déjà disponibles dans `gpt2_gguf_loader`. Le prochain lot pourra les connecter à un runtime de génération complet en s’appuyant sur le même modèle de buffers statiques et de capacité déclarée.
