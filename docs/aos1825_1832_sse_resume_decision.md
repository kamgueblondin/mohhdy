# AOS-1825…1832 — Décision de reprise SSE dans le contexte LLM

## Objet

`ne2k_llm_network_context_sse_resume_decide()` rend explicite la décision à prendre lorsque le transport TLS est prêt : construire un GET de reprise lorsque `Last-Event-ID` est valide, ou démarrer un flux neuf lorsqu’il est absent, notamment après une rotation de fournisseur.

| État | `Last-Event-ID` | Résultat |
|---|---:|---|
| `TLS_COMPLETE` | présent | `1` — reprise SSE |
| `TLS_COMPLETE` | absent | `0` — flux neuf |
| autre phase | indifférent | `-2` — rejet |
| pointeur invalide | indifférent | `-1` — rejet |

La décision est pure : elle ne modifie ni contexte, ni scheduler, ni checkpoint SSE, ni buffer. Le test NE2000 couvre les trois décisions et leurs gardes, avec **43/43 tests ciblés réussis**.

## Références

[1] [AOS-1793 à AOS-1800 — rotation de fournisseur](aos1793_1800_sse_provider_rotation_context.md)  
[2] [AOS-1769 à AOS-1776 — persistance SSE](aos1769_1776_sse_session_context.md)
