# MOHHDY Foundation — Incrément 96 : accumulation des values

**État :** implémenté et testé.

## Objectif

Le lot 96 complète l’étape située après les scores query-key bruts. `gpt2_gguf_kv_cache_accumulate_values` produit le vecteur d’attention en accumulant les values historiques avec les poids fournis par l’appelant :

\[
O_c = \sum_{p=0}^{N-1} w_p \times V_{p,c}
\]

Les poids peuvent être des probabilités issues d’un softmax futur ou tout autre vecteur normalisé préparé par le caller. La primitive ne réalise pas encore le softmax ni la mise à l’échelle des scores; elle reste donc composable avec les incréments suivants.

## Contrat caller-owned

| Élément | Contrat |
| --- | --- |
| `weights` | tableau fourni par l’appelant, de capacité au moins égale à `position_count` |
| `output` | tableau fourni par l’appelant, de capacité au moins égale à `cache->channels` |
| allocation | aucune allocation dynamique et aucun buffer caché |
| sortie | `output[0..channels-1]`, `out_count = channels` |
| état du cache | lecture seule; les clés et values historiques ne sont pas modifiées |

La sortie est remise à zéro avant l’accumulation, ce qui rend la fonction réutilisable pour chaque tête, couche ou position sans dépendre d’un contenu précédent du buffer.

## Validation des erreurs

La primitive suit les conventions du cache KV. Elle renvoie `-1` lorsqu’un pointeur requis manque, `-6` lorsqu’une capacité est insuffisante et `-9` lorsqu’une couche ou une fenêtre historique est hors bornes. Pour une fenêtre vide, elle retourne un succès et `out_count = 0`, sans lire les poids ni modifier la sortie.

## Tests

La fixture du cache contient les values `[5,6,7,8]`, `[19,6,7,8]` et `[5,6,7,8]`. Avec les poids `[0,2, 0,3, 0,5]`, la sortie attendue est `[9,2, 6, 7, 8]`. Les tests couvrent le résultat pondéré, le nombre de canaux produit, un tableau de poids trop court, une sortie trop courte et une fenêtre hors historique.

`make test-all` passe avec **265 tests réussis, 0 échec et 0 test ignoré**.

## Suite

L’étape suivante peut introduire la mise à l’échelle des scores par la dimension de tête, puis un softmax stable caller-owned avant d’appeler cette accumulation.
