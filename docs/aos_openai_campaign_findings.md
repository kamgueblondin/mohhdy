# Cadrage de la campagne OpenAI

## Constat de compatibilité

Le client bare-metal utilise déjà le chemin historique **Chat Completions** : un `POST` JSON vers un chemin fourni par l’appelant, avec `Authorization: Bearer`, et le booléen `stream:true`. Son extracteur traite le champ `choices[0].delta.content` des chunks SSE. Cette forme reste documentée pour les événements Chat Completions, tandis que la documentation OpenAI actuelle recommande l’API Responses pour les nouveaux développements.

La campagne doit donc distinguer deux objectifs : valider exhaustivement le contrat Chat Completions réellement implanté, puis préparer une migration explicite vers Responses sans faire passer l’ancien parser pour un parser universel des événements sémantiques Responses.

## Probe externe non génératif

Le 20 août 2026, un `GET https://api.openai.com/v1/models` utilisant la variable de test disponible dans le sandbox a reçu **HTTP 401** avec le code `invalid_api_key`. Aucun token n’est copié dans cette note, les logs ou le dépôt. Cette réponse ne permet pas de conclure à une panne du client OS ; elle bloque seulement la campagne d’intégration réelle jusqu’à la fourniture d’une clé API OpenAI valide et autorisée.

## Invariants de sécurité

| Élément | Décision |
|---|---|
| Secret API | ne jamais l’écrire dans le dépôt, les fixtures, la sortie QEMU ou la documentation publiée |
| Test réel | exécuté seulement avec une variable d’environnement explicitement fournie hors CI publique |
| Test CI | fixtures déterministes reproduisant les réponses 2xx, 401, 429 et 5xx sans appel externe |
| Streaming | validation séparée des chunks Chat Completions, de la fin `[DONE]` et des erreurs de protocole |
| Mémoire | buffers fixes ou caller-owned ; aucune allocation dynamique |

## Références

1. [OpenAI API Overview](https://developers.openai.com/api/reference/overview) : bearer API key, informations de requête et compatibilité API v1.
2. [OpenAI Streaming Responses](https://developers.openai.com/api/docs/guides/streaming-responses) : différence entre l’API Responses moderne et les flux SSE.
3. [Chat Completions streaming events](https://developers.openai.com/api/reference/resources/chat/subresources/completions/streaming-events) : chunks avec `choices[].delta.content`.
