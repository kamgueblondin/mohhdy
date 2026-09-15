# AOS-369 à AOS-376 — adaptateurs de réponse Ollama/OpenAI

La couche HTTP/TLS fournit deux adaptateurs caller-owned au-dessus de `net_json_extract_string`. `net_llm_ollama_response_extract` récupère le champ string `response` d’une réponse Ollama non-streaming. `net_llm_openai_response_extract` récupère le premier champ string `content` rencontré dans une réponse OpenAI compatible chat/completions déjà reçue en entier.

| Adaptateur | Champ extrait | Cas couvert |
|---|---|---|
| Ollama | `response` | Réponse JSON non-streaming contenant le texte de génération. |
| OpenAI compatible | `content` | Réponse JSON complète avec un contenu assistant déjà matérialisé. |

Les adaptateurs ne conservent aucun état, token ou buffer caché. Ils utilisent le buffer de sortie fourni par l’appelant et propagent les rejets de clé absente, d’échappement non pris en charge ou de capacité insuffisante.

Le test couvre `bonjour` dans une réponse Ollama et `salut` dans une réponse OpenAI compatible, ainsi que les rejets croisés lorsqu’un champ fournisseur attendu est absent.

> L’adaptateur OpenAI n’effectue pas de validation structurelle de `choices[0].message.content` ; il est volontairement limité au premier champ `content` string. Streaming SSE, tool calls, audio/images, Unicode, réponses multiples et variation des schémas de fournisseur restent hors périmètre.
