# MOHHDY Foundation — Incrément 101 : forward d’attention multi-têtes

**État :** implémenté et testé.

## Objectif

Le lot 101 ajoute `gpt2_gguf_kv_cache_attention_multi_head`. Cette primitive exécute successivement l’attention complète de chaque tête du cache KV, réutilise les buffers de scratch de clé et de scores, écrit les sorties intermédiaires dans un tableau caller-owned, puis concatène les têtes dans le vecteur final.

## Contrat mémoire

| Buffer | Usage |
| --- | --- |
| `query` | vecteur complet de `channels`, lu sans modification |
| `head_outputs` | sorties intermédiaires contiguës, capacité d’au moins `channels` floats |
| `key_scratch` | scratch d’une seule tête, réutilisé entre les têtes |
| `scores` | scores historiques, réutilisés entre les têtes |
| `output` | vecteur concaténé final, capacité d’au moins `channels` floats |

Aucune allocation dynamique n’est réalisée. Le cache reste en lecture seule. La fonction rejette les nombres de têtes qui ne divisent pas `channels`, les capacités insuffisantes et les intervalles invalides propagés par la primitive par tête.

## Chaîne exécutée

Pour chaque tête, la primitive réalise les produits query-key, le scaling par `1/sqrt(head_size)`, le softmax stable, puis l’accumulation des values historiques. Après la boucle, elle appelle la concaténation multi-têtes et retourne `out_count = channels`.

## Validation

Le test d’intégration réutilise le cache à deux têtes du lot 98, vérifie les sorties des deux tranches et contrôle les erreurs pour trois têtes non compatibles ainsi qu’un buffer intermédiaire trop court. `make test-all` passe avec **265 tests réussis, 0 échec et 0 test ignoré**.

## Suite

Le prochain groupe peut connecter cette primitive à la projection `attn_output.weight`, puis ajouter la connexion résiduelle et la normalisation de sortie du bloc d’attention.
