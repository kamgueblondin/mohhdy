# MOHHDY Foundation — Incrément 97 : scaling et softmax de l’attention

**État :** implémenté et testé.

## Objectif

Le lot 97 relie les scores query-key bruts du lot 95 aux poids nécessaires au lot 96. Il fournit deux primitives indépendantes et sans allocation :

| Primitive | Rôle |
| --- | --- |
| `gpt2_gguf_attention_scale_scores` | multiplie chaque score par `1/sqrt(head_size)` |
| `gpt2_gguf_attention_softmax` | convertit les scores en probabilités par soustraction du maximum puis exponentiation et normalisation |

La mise à l’échelle respecte le calcul d’attention par tête. Le softmax est stable vis-à-vis des grands scores positifs, car le maximum est retiré avant l’exponentiation. L’approximation d’exponentielle déjà utilisée par le moteur freestanding est bornée à `[-80, 80]`; elle évite de dépendre de la bibliothèque mathématique ou d’une allocation dynamique.

## Contrat caller-owned

Les deux primitives modifient le tableau `scores` fourni par l’appelant. Elles ne créent aucun scratch interne et ne dépendent d’aucun état global. Le softmax écrit dans le même tableau et retourne `out_count = score_count`. Une entrée vide est acceptée comme succès avec `out_count = 0`.

La mise à l’échelle refuse un pointeur nul ou une dimension de tête nulle. Le softmax refuse un pointeur nul ou l’absence du compteur de sortie; une somme exponentielle dégénérée retourne `-7` plutôt que de produire des NaN ou une division invalide.

## Validation

Le test vérifie qu’un vecteur `[4,0]` mis à l’échelle pour une tête de taille 4 devient environ `[2,0]`, dans la précision de l’inverse de racine carrée rapide. Il vérifie ensuite que le softmax de `[0,1,2]` est strictement croissant et que la somme des probabilités vaut 1 à la précision de test. Les cas vides et les pointeurs invalides sont également couverts.

`make test-all` passe avec **265 tests réussis, 0 échec et 0 test ignoré**.

## Suite

La chaîne caller-owned est désormais disponible : projection QKV, écriture du cache, scores query-key, scaling, softmax puis accumulation des values. Le prochain lot peut intégrer ces primitives dans un forward autoregressif GGUF par tête et par couche.
