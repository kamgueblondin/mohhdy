# MOHHDY Foundation — Incrément 98 : forward d’attention par tête

**État :** implémenté et testé.

## Objectif

Le lot 98 compose les primitives livrées précédemment dans `gpt2_gguf_kv_cache_attention_head`. Pour une tête donnée, la primitive exécute la chaîne autoregressive complète sur une fenêtre historique : produit query-key, mise à l’échelle par `1/sqrt(head_size)`, softmax stable, puis accumulation des values.

La disposition du cache reste `[couche][position][K puis V]`. La fonction sélectionne la tranche correspondant à `head_index`, avec `head_size = channels / head_count`, et n’examine pas les canaux des autres têtes.

## Contrat caller-owned

| Buffer | Utilisation | Capacité minimale |
| --- | --- | --- |
| `query` | query de la tête courante | `head_size` floats |
| `key_scratch` | clé d’une position, réutilisée à chaque itération | `head_size` floats |
| `scores` | scores puis probabilités historiques | `position_count` floats |
| `output` | résultat de la tête | `head_size` floats |

Aucune allocation dynamique, aucun état global et aucune mutation du cache ne sont introduits. La sortie est remise à zéro avant l’accumulation. `out_count` vaut `head_size` en cas de succès; une fenêtre vide retourne un succès avec `out_count = 0`.

## Validations

La primitive rejette une couche, une tête ou une fenêtre hors bornes avec `-9`, les buffers trop courts avec `-6` et les pointeurs requis absents avec `-1`. Elle exige que `channels` soit divisible par `head_count` et que `head_index` soit inférieur au nombre de têtes.

## Tests

La fixture utilise quatre canaux et deux têtes. La tête 0 reçoit un historique de clés qui favorise la deuxième position et produit une sortie dominée par la value `[19,6]`. La tête 1 lit une tranche uniforme et restitue `[7,8]` à la précision de l’approximation freestanding. Les tests couvrent également une tête inexistante et un scratch trop court.

`make test-all` passe avec **265 tests réussis, 0 échec et 0 test ignoré**.

## Suite

Le chemin d’attention d’une tête est désormais intégré. Le prochain incrément peut boucler sur toutes les têtes, concaténer les sorties dans le vecteur de canaux et préparer la projection d’attention quantifiée.
