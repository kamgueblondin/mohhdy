# AOS-1561 à AOS-1568 — pas de génération GPT-2 GGUF, token vers logits

## Objectif

Ce lot raccorde le contexte GPT-2 GGUF précédemment préparé à un pas local complet sur FAT16. La nouvelle primitive `gpt2_gguf_generation_token_fat16` reçoit un token, sa position, le cache KV et un workspace intégralement fourni par l’appelant. Elle lit les embeddings, traverse les blocs transformeur préparés, applique la normalisation finale et produit les logits de la tête quantifiée.

## Exécution bornée

| Étape | Données GGUF | Destination caller-owned |
|---|---|---|
| Entrée | embeddings token et position F32/F16 | état caché et vecteur position |
| Bloc | normes, biais QKV, matrices attention/MLP Qx_K | workspace de bloc et cache KV |
| Sortie | norme finale dense et matrice de sortie Q3_K/Q4_K/Q6_K | logits |

> Aucune étape ne réserve, ne copie durablement ou ne possède la mémoire du modèle. Les buffers sont contrôlés avant lecture et les transitions de cache exigent que la position demandée corresponde au prochain emplacement libre.

Le lot rend aussi explicite un point de contrat important : les capacités publiques du workspace sont exprimées en éléments `float`, alors que la primitive de lecture dense attend une capacité en octets. La conversion est désormais faite localement et contrôlée avant chaque lecture.

## Validation ciblée

La fixture FAT16/GGUF contient cinq tenseurs globaux et les dix tenseurs d’une couche GPT-2 à 256 canaux avec un MLP `4C`. Elle exécute deux positions successives, vérifie la progression du cache KV, les logits produits et le rejet d’un token hors vocabulaire.

## Limites restantes

Le runtime exécute désormais un pas token-vers-logits, mais il n’est pas encore raccordé au shell `ai`, au tokenizer GGUF ni à l’échantillonneur top-k local. Le prochain lot devra fournir cet adaptateur utilisateur et mesurer la latence sur les modèles compatibles.
