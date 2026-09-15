# AOS-849 à AOS-856 — SSE multi-ligne et UTF-8 fragmenté

## Objectif

Ce macro-lot durcit le streaming LLM SSE sans allocation dynamique. L’accumulateur accepte désormais un événement composé de plusieurs lignes `data:` valides et concatène leurs charges utiles dans le buffer caller-owned avant d’appeler l’extracteur JSON fournisseur.

Le même chemin accepte une séquence UTF-8 dont les octets sont répartis entre deux appels `net_llm_sse_accumulator_feed`. L’extraction reste différée jusqu’à la fin de l’événement SSE ; aucun caractère partiel n’est publié.

## Contrat

| Élément | Comportement |
|---|---|
| Ligne SSE | Chaque ligne doit commencer exactement par `data:` |
| Événement multi-ligne | Les charges utiles sont concaténées avant extraction JSON |
| UTF-8 fragmenté | Les fragments incomplets restent en attente dans le buffer |
| `[DONE]` | Conserve la terminaison existante |
| Mémoire | Buffer caller-owned borné, sans `kmalloc` |
| Erreur de capacité ou framing | Rejet transactionnel sans sortie partielle |

Les commentaires SSE, champs `id`, `retry` et événements autres que `data:` restent refusés volontairement afin de ne pas ignorer silencieusement un framing non supporté.

## Validation

Le scénario ajouté vérifie une réponse Ollama dont l’emoji UTF-8 est coupé entre deux appels `feed`, puis un événement `data:` multi-ligne reconstruit en `bonjour`. La suite HTTP/TLS atteint **17/17 tests verts** ; la suite globale et les smokes QEMU doivent confirmer la non-régression du reste du noyau i386.

## Limites restantes

Ce lot ne fournit pas encore de pagination applicative, de file de sortie multi-appels ou de reconnexion SSE. La capacité des buffers reste fixe et les réponses qui la dépassent sont rejetées plutôt que tronquées.
