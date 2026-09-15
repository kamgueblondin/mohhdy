# MOHHDY Foundation — Incrément 99 : concaténation multi-têtes

**État :** implémenté et testé.

## Objectif

Le lot 99 ajoute `gpt2_gguf_attention_concat_heads`, qui rassemble les sorties calculées indépendamment pour chaque tête dans le vecteur d’attention attendu par la projection de sortie. Les données sont supposées contiguës dans l’ordre `[tête 0][tête 1]...`, et l’ordre des canaux est conservé sans transformation.

## Contrat caller-owned

| Élément | Contrat |
| --- | --- |
| `head_outputs` | sorties contiguës des têtes, en lecture seule |
| `head_count` | nombre strictement positif de têtes |
| `head_size` | nombre strictement positif de canaux par tête |
| `output` | vecteur de destination fourni par l’appelant |
| allocation | aucune allocation dynamique ni stockage interne |
| résultat | `out_count = head_count × head_size` |

La primitive contrôle l’overflow de la multiplication, la capacité de sortie et les dimensions nulles. Une entrée invalide renvoie `-1`, une dimension ou un produit impossible renvoie `-9`, et une destination trop courte renvoie `-6`.

## Rôle dans le forward

La chaîne Foundation est maintenant structurée comme suit: projection QKV quantifiée, écriture du cache KV, attention complète pour chaque tête, concaténation des sorties, puis future projection `attn_output.weight`. Le cache et les buffers intermédiaires restent caller-owned; aucune allocation n’est ajoutée au chemin critique.

## Tests

La fixture concatène deux têtes de taille deux contenant `[1,2]` et `[3,4]`, puis vérifie la sortie `[1,2,3,4]`, le compteur de quatre canaux, une capacité insuffisante et un nombre de têtes nul.

`make test-all` passe avec **265 tests réussis, 0 échec et 0 test ignoré**.

## Suite

Le prochain lot peut ajouter une projection matricielle quantifiée générique pour appliquer `attn_output.weight` au vecteur concaténé, avant de poursuivre vers le bloc MLP.
