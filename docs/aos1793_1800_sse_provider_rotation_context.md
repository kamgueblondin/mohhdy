# AOS-1793…1800 — Rotation SSE de fournisseur dans le contexte LLM

## Objet

Ce macro-lot raccorde l’épuisement du budget de reconnexion SSE à la rotation déjà définie entre Ollama et OpenAI. La nouvelle primitive `ne2k_llm_network_context_sse_rotate_provider()` ne bascule que si le scheduler n’est plus armé et si son nombre de tentatives atteint sa limite.

| Situation | Comportement |
|---|---|
| Retry encore planifié | Aucune rotation ni mutation |
| Budget non atteint | Aucune rotation ni mutation |
| Budget atteint | Bascule Ollama ↔ OpenAI, reset du scheduler et checkpoint persistant |
| Fournisseur invalide | Rejet transactionnel |

## Sécurité de reprise

Un `Last-Event-ID` est propre au flux et à son fournisseur. Une rotation l’invalide donc explicitement : le checkpoint reste présent pour conserver le fournisseur choisi et le budget remis à zéro, mais est restauré comme **non-reprenable** (`event_id_valid = 0`). Le format persistant SSE supporte désormais ce checkpoint vide, tout en conservant la validation de bornes et de checksum.

> La rotation ne stocke ni endpoint, ni bearer, ni modèle, ni buffer HTTP/SSE. L’intégrateur choisit les paramètres du nouveau fournisseur avant d’émettre une requête fraîche.

## Transactionnalité

La rotation travaille sur des copies du contexte, du scheduler, de la réponse SSE et du fournisseur. Le checkpoint est écrit avant toute publication. Une entrée invalide ou une erreur de persistance laisse donc tous les objets appelants inchangés.

| Propriété | Garantie |
|---|---|
| Bascule | Uniquement après saturation réelle du budget |
| Scheduler | `retries_used`, `next_tick` et `scheduled` remis à zéro |
| Reprise | `Last-Event-ID` effacé entre fournisseurs |
| Persistance | Nouveau fournisseur et budget zéro enregistrés avec checksum |
| Allocation dynamique | Aucune |

## Tests

Le vecteur NE2000 couvre une saturation de budget Ollama, la bascule vers OpenAI, le reset du scheduler, la restauration du checkpoint sans identifiant et les gardes sur un scheduler encore actif ou un fournisseur invalide.

| Vérification | Résultat |
|---|---|
| Budget atteint → Ollama vers OpenAI | Validé |
| Scheduler réarmé à zéro | Validé |
| Checkpoint OpenAI sans `Last-Event-ID` | Validé |
| Scheduler armé | Sans mutation |
| Fournisseur invalide | Rejet transactionnel |
| Test NE2000 ciblé | **39/39 réussis** |

## Références

[1] [AOS-953 à AOS-960 — rotation de fournisseur](aos953_960_provider_rotation.md)  
[2] [AOS-1769 à AOS-1776 — contexte SSE persistant](aos1769_1776_sse_session_context.md)  
[3] [AOS-1785 à AOS-1792 — tick SSE actif de contexte](aos1785_1792_sse_context_event_tick.md)  
[4] [WHATWG — Server-Sent Events](https://html.spec.whatwg.org/multipage/server-sent-events.html)
