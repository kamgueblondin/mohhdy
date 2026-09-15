# AOS-409 à AOS-416 — durcissement temps constant X25519/bigint

Le chemin du ladder X25519 utilise désormais des primitives bigint à largeur fixe sur le nombre de limbs du module. Les additions, soustractions et multiplications modulaires parcourent systématiquement tous les limbs et tous les bits. L’ajout dans la multiplication est sélectionné au moyen d’un masque calculé depuis le bit, plutôt que par une branche conditionnelle.

| Élément | Durcissement appliqué |
|---|---|
| Échange Montgomery | Échange XOR masqué de tous les limbs, sans échange de longueur variable. |
| Sous-modulo | Soustraction et ajout conditionnel du module par masque de borrow. |
| Addition modulo | Soustraction du module puis restauration masquée selon carry/borrow. |
| Multiplication modulo | Boucles fixes `modulus->length × 32` ; ajout du multiplicande masqué. |
| Représentation X25519 | Les éléments de champ conservent une largeur de huit limbs pendant le ladder. |

Les primitives `bigint_mod_add_ct`, `bigint_mod_subtract_ct` et `bigint_mod_multiply_ct` exigent des operands canoniques, initialisés jusqu’à `modulus->length` et fournis par l’appelant. Elles sont employées dans le ladder X25519, notamment pour les opérations dont les operands dérivent du scalaire privé.

> Ce lot réduit les branches explicites de haut niveau liées aux bits secrets, mais ne constitue pas une preuve formelle de temps constant : le compilateur, le processeur, le cache, les multiplications 64 bits i386, les accès mémoire et les chemins RSA restent hors analyse instrumentée. Une revue de l’assembleur généré et des tests de fuite dédiés restent nécessaires avant toute promesse de résistance aux canaux auxiliaires.

Les vecteurs X25519 existants restent verts. Un test bigint compare les résultats classiques et à largeur fixe pour addition avec réduction, soustraction sous module et multiplication.
