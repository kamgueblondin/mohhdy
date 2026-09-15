# AOS-809 à AOS-832 — JSON UTF-8/Unicode pour les échanges LLM

**Statut : implémenté et validé.** Ce macro-lot lève la limitation ASCII des requêtes et réponses JSON LLM tout en conservant le modèle freestanding, borné et sans allocation dynamique.

## Contrat Unicode

| Entrée | Comportement |
|---|---|
| UTF-8 1 à 4 octets | Accepté si la séquence est complète, canonique et dans `U+0000..U+10FFFF`. |
| Surrogates UTF-8 directs | Rejetés comme séquences invalides. |
| `\uXXXX` BMP | Décodé vers UTF-8 caller-owned. |
| Paires `\uD800..\uDBFF` + `\uDC00..\uDFFF` | Combinées en code point non-BMP puis encodées en quatre octets UTF-8. |
| Surrogate isolé ou hexadécimal invalide | Rejeté sans publier une sortie partielle. |
| Octet UTF-8 tronqué ou surlong | Rejeté. |
| Contrôles JSON `< 0x20` | Rejetés dans les valeurs non échappées. |

L’extracteur `net_json_extract_string` conserve les échappements JSON classiques (`\"`, `\\`, `\/`, `\b`, `\f`, `\n`, `\r`, `\t`) et ajoute `\uXXXX`. L’écriture vers le buffer appelant est vérifiée à chaque code point ; une capacité insuffisante retourne une erreur bornée.

## Builders LLM

`net_llm_build_ollama_generate_json`, `net_llm_build_openai_chat_json` et leurs variantes streaming acceptent désormais les modèles et prompts UTF-8 valides. Les caractères non contrôlés sont recopiés en UTF-8, tandis que guillemets, antislashs et contrôles sont échappés comme l’exige JSON. Une séquence invalide est rejetée avant qu’une requête utilisable ne soit publiée.

Le chemin SSE réutilise le même extracteur pour les champs `response` Ollama et `content` OpenAI. Le changement est donc commun aux réponses non-streaming et aux deltas streaming, sans duplication de codec ni allocation cachée.

## Tests et sécurité

Le test Unity vérifie le décodage de `caf\\u00e9`, d’une paire emoji `\\ud83d\\ude00`, la conservation de l’UTF-8 brut dans un body Ollama et le rejet d’un prompt tronqué. Les assertions historiques sont adaptées pour confirmer que `\\u0041` produit bien `A`; tous les tests ASCII et SSE existants restent inchangés dans leur résultat.

Le décodeur utilise uniquement des variables locales de taille fixe et les buffers détenus par l’appelant. Il n’emploie ni `kmalloc`, ni table dynamique, ni dépendance libc Unicode. Le noyau i386 reste compilé en mode freestanding.

## Validation

| Vérification | Résultat |
|---|---|
| `test_net_http_tls` ciblé | **14/14** scénarios passés. |
| `make test-build` | Image i386 compilée avec succès. |
| `make test-all` | **396/396** tests passés. |
| `make qemu-ai-provider` | Smoke fournisseur IA réussi. |
| `make qemu-ne2k-status` | Smoke NE2000 réussi. |

Le build signale uniquement les avertissements historiques de style d’indentation du fichier compacté ; aucune erreur ni régression fonctionnelle n’est présente.

## Limites explicites

Le codec accepte et produit l’UTF-8 JSON dans les buffers du noyau, mais ne prétend pas normaliser les formes Unicode, translittérer les textes ou appliquer une politique linguistique. Les tailles restent limitées par les capacités caller-owned des builders, extracteurs, HTTP et SSE. La prochaine fonctionnalité logique est la propagation Unicode jusqu’au service LLM interactif complet et la vérification de réponses JSON volumineuses en fragments.
