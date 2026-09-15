# AOS-513 à AOS-520 — Streaming SSE LLM sécurisé sur HTTPS

**Auteur :** Manus AI  
**Statut :** implémenté et validé localement  
**Périmètre :** HTTP/1.1 chunked, SSE, AES-GCM TLS 1.2, NE2000, Ollama/OpenAI compatibles

## Objectif

Ce macro-lot ajoute un chemin de réponse LLM incrémental au-dessus du canal HTTPS déjà authentifié. Il construit les bodies JSON `stream:true`, décode des réponses HTTP `Transfer-Encoding: chunked`, accumule les événements SSE fragmentés et expose les deltas textuels dans un buffer fourni par l’appelant.

> La spécification SSE décrit des champs `data:` regroupés en événements par une ligne vide ; les lignes peuvent être terminées par LF ou CRLF [1]. L’implémentation applique ce framing à un sous-ensemble strictement borné, adapté aux réponses JSON de fournisseurs LLM.

## API ajoutée

| API | Rôle |
|---|---|
| `net_llm_build_ollama_generate_stream_json` | Construit un body Ollama avec `"stream":true`. |
| `net_llm_build_openai_chat_stream_json` | Construit un body OpenAI compatible avec `"stream":true`. |
| `net_llm_sse_accumulator_*` | Accumule et décode les événements `data:` fragmentés. |
| `net_llm_sse_response_*` | Compose le décodage HTTP chunked puis SSE. |
| `net_http_tls_open_sse_stream` | Ouvre transactionnellement un record TLS applicatif et l’alimente au décodeur SSE. |
| `ne2k_https_llm_stream_request` | Chiffre et émet une requête LLM streaming depuis NE2000. |
| `ne2k_https_llm_poll_sse` | Poll, déchiffre, décode et acquitte un fragment streaming. |

## Flux de données

| Étape | Traitement | Propriété de mémoire |
|---|---|---|
| 1 | Le builder produit le JSON `stream:true`. | Buffer JSON caller-owned. |
| 2 | Le wrapper NE2000 chiffre le POST dans un record AES-128-GCM. | Request et record caller-owned. |
| 3 | Le polling reçoit et authentifie le record TLS applicatif. | Plaintext caller-owned. |
| 4 | L’accumulateur chunked retire les en-têtes et tailles HTTP. | Buffer HTTP caller-owned. |
| 5 | L’accumulateur SSE attend un événement complet `data: …` terminé par ligne vide. | Buffer SSE caller-owned. |
| 6 | Le delta JSON est extrait sous la clé `response` (Ollama) ou `content` (OpenAI compatible). | Buffer texte caller-owned. |

Chaque appel retourne `1` lorsqu’aucun événement complet ne fournit encore de delta, `0` après progression ou terminaison SSE, et une valeur négative en cas de framing, capacité, provider ou authentification invalide.

## Contrat de sécurité et de robustesse

Aucune allocation dynamique n’est introduite. Les contextes SSE conservent seulement pointeur, capacité, longueur et drapeau de terminaison ; les buffers restent entièrement possédés par l’appelant. Un événement dépassant la capacité est rejeté avant écriture hors limite. La fin explicite `[DONE]` est consommée et fixe le drapeau `done`.

Le wrapper TLS réalise un rollback des états TCP, session AEAD et contexte SSE si l’ouverture du record ou le parsing échoue. Le wrapper NE2000 applique ensuite son rollback de client et connexion si l’ACK de transport échoue. Les anciennes APIs non-streaming et les builders `stream:false` sont conservés sans modification de contrat.

## Sous-ensemble SSE couvert

| Élément | Comportement |
|---|---|
| Champ accepté | Une unique ligne `data:` par événement. |
| Séparateur d’événement | `\n\n` ou `\r\n\r\n`. |
| Espacement | Un espace optionnel après `data:` est ignoré. |
| Terminaison fournisseur | `[DONE]` est reconnue. |
| JSON Ollama | Extraction de `response`. |
| JSON OpenAI compatible | Extraction de `content`; un chunk sans `content` est accepté sans delta. |
| HTTP | Réponse `2xx` chunked obligatoire sur ce chemin. |

## Tests et validation locale

Les vecteurs Unity couvrent la construction JSON streaming, un événement Ollama découpé entre deux fragments, la terminaison `[DONE]`, une réponse OpenAI chunked complète, un événement SSE invalide et le dépassement de capacité du buffer SSE.

| Vérification | Résultat |
|---|---|
| Suite noyau | **32/32** exécutables réussis. |
| Suite complète | **371/371** tests réussis. |
| Compilation freestanding i386 | Réussie. |
| Smoke QEMU `qemu-ai-provider` | Réussi. |
| Smoke QEMU `qemu-ne2k-status` | Réussi. |

## Limites connues

Ce lot ne met pas en œuvre le modèle SSE générique complet : commentaires, champs `event`, `id` et `retry`, événements multi-lignes `data`, terminaison CR seule, UTF-8 multi-octet validé, `Last-Event-ID`, reconnexion SSE, backoff, annulation par utilisateur et multiplexage de flux restent hors périmètre. Les réponses non chunked et les fournisseurs dont le format streaming n’est pas compatible avec `data:` + JSON `response`/`content` doivent continuer à utiliser le chemin non-streaming ou un adaptateur dédié.

## Références

[1] [WHATWG HTML Living Standard, section 9.2 — Server-sent events](https://html.spec.whatwg.org/multipage/server-sent-events.html)  
[2] [AOS-385 à AOS-392 — requête LLM NE2000 unifiée](aos385_392_ne2k_https_llm_request.md)
