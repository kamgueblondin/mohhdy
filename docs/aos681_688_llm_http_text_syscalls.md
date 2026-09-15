# AOS-681 à AOS-688 — Syscalls HTTP LLM et polling de texte contrôlés

**Auteur :** Manus AI

**Statut :** implémenté et validé localement

**Périmètre :** émission POST LLM après TLS authentifié et restitution bornée du texte extrait

## Objectif

Ce macro-lot ajoute deux syscalls au plan de contrôle LLM. `SYS_LLM_REQUEST` prépare et émet un POST JSON chiffré uniquement lorsque la session noyau a atteint `TLS_COMPLETE`. `SYS_LLM_POLL_TEXT` avance le polling HTTP/TLS et copie vers Ring 3 seulement le texte fournisseur déjà extrait, accompagné du code HTTP. Les records TLS, le corps JSON complet, les buffers HTTP et les états TCP/TLS restent internes au noyau.

| Syscall | Entrée | Sortie | Phase exigée |
|---|---|---|---|
| `SYS_LLM_REQUEST` (`93`) | `os_llm_request_t*` | code de contrôle | `TLS_COMPLETE` |
| `SYS_LLM_POLL_TEXT` (`94`) | `os_llm_text_result_t*` | texte extrait et statut HTTP | `REQUEST_SENT` |

## ABI sans secret

La requête est une structure POD fixe. Elle comprend le fournisseur, un modèle, un chemin et un prompt borné. Aucun pointeur, bearer token, mot de passe, clé, adresse IP, buffer TLS ou identifiant fournisseur n’est accepté.

```c
typedef struct {
    uint8_t provider;
    char model[OS_LLM_MODEL_MAX];
    char path[OS_LLM_PATH_MAX];
    uint16_t prompt_length;
    uint8_t prompt[OS_LLM_PROMPT_MAX];
} os_llm_request_t;
```

Le résultat est également copié par valeur. Il ne contient qu’un texte de 512 octets au plus et le statut HTTP. Il ne conserve aucune vue vers un record chiffré, un plaintext noyau ou un accumulateur HTTP.

```c
typedef struct {
    uint16_t text_length;
    uint16_t status_code;
    uint8_t text[OS_LLM_TEXT_MAX];
} os_llm_text_result_t;
```

## Politique fournisseur

Ollama peut être demandé sans jeton, car son adaptateur HTTP ne requiert pas d’autorisation Bearer. OpenAI est volontairement refusé par `OS_LLM_REQUEST_UNCONFIGURED` tant qu’un canal de provisionnement interne de credential n’existe pas. Cette décision évite d’envoyer ou de stocker un bearer token dans le shell, l’ABI syscall ou l’image de boot.

> L’absence de provisionnement OpenAI est un refus de sécurité explicite, non une prise en charge incomplète silencieuse.

| Condition | Retour | Effet |
|---|---:|---|
| Requête POD invalide | `OS_LLM_REQUEST_BAD_REQUEST` | Aucune émission. |
| NIC absente ou TLS incomplet | `OS_LLM_REQUEST_BAD_PHASE` | Aucun état publié. |
| OpenAI sans credential noyau | `OS_LLM_REQUEST_UNCONFIGURED` | Aucun secret n’est demandé à Ring 3. |
| Échec HTTP/TLS/TCP | `OS_LLM_REQUEST_FAILED` | La façade réseau conserve son rollback transactionnel. |
| Polling avant POST | `OS_LLM_TEXT_BAD_PHASE` | Aucun buffer ou texte n’est publié. |
| Polling complet | `0` | Texte et statut HTTP copiés dans la sortie bornée. |

## Mémoire et confinement

Le noyau emploie des buffers fixes de 8 Kio pour JSON, requête HTTP, record TLS, réponse HTTP et plaintext, plus un buffer texte de 512 octets. L’accumulateur HTTP et la vue de réponse sont persistants côté noyau. Aucune allocation dynamique n’est introduite et l’ABI ne permet pas d’obtenir leurs adresses.

Les commandes shell sont `ai-request <ollama|openai> <modele> <chemin> <prompt>` et `ai-text-poll`. Elles ne manipulent que les structures publiques bornées et des diagnostics de phase. La simulation QEMU encode désormais `/` explicitement dans les commandes, afin de couvrir un chemin HTTP réaliste comme `/api/generate`.

## Validation locale

Le smoke QEMU fournisseur vérifie successivement le rejet d’un POST Ollama avant TLS authentifié, puis le rejet du polling avant émission. Il confirme ensuite que la phase demeure `IDLE` et que le bail DHCP reste absent. Le smoke NE2000 confirme la compatibilité avec une carte détectée sans acquisition DHCP.

| Vérification | Résultat |
|---|---|
| Compilation freestanding i386 | Réussie. |
| Smoke QEMU fournisseur | Réussi ; gardes POST/polling et état conservé. |
| Smoke QEMU NE2000 | Réussi ; carte prête, `IDLE`, bail absent. |
| Suite complète | Réussie : **384/384** tests. |
| `git diff --check` | Propre avant commit. |

## Limites connues

L’émission effective reste dépendante d’un handshake TLS réellement terminé. Le lot précédent bloque volontairement cette étape jusqu’au provisionnement noyau d’une entropie cryptographique et d’une ancre X.509 de production. OpenAI exige ensuite un mécanisme distinct de credential noyau ; aucun token ne sera ajouté au shell ou à l’image. Timeout/retry applicatif, fermeture, streaming SSE via syscall, outils, multimodal et Unicode complet restent hors périmètre.

## Références

[1] [AOS-673 à AOS-680 — syscall de polling TLS LLM contrôlé](aos673_680_llm_poll_tls_syscall.md)

[2] [AOS-577 à AOS-584 — réutilisation d’une session TLS LLM](aos577_584_llm_session_reuse.md)

[3] [AOS-545 à AOS-552 — progression TLS authentifiée dans la session LLM](aos545_552_llm_tls_session_progress.md)
