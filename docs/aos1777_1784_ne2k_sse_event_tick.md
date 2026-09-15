# AOS-1777…1784 — Tick SSE NE2000 et reprise automatique

## Objet

Ce macro-lot ajoute une façade de tick à la session SSE NE2000. `ne2k_llm_connection_sse_event_tick()` compose le polling/résumé déjà existant avec le traitement automatique des résultats terminaux. Elle évite à l’appelant de recoder la classification d’erreur, le basculement vers `TLS_COMPLETE` et la programmation du délai de reconnexion.

| Situation observée | Action du tick |
|---|---|
| Progression SSE | Conserve la phase courante et remonte la progression |
| Flux terminé | Délègue la clôture vers `RESPONSE_READY` |
| Erreur transport ou statut retryable | Programme le retry borné et repasse en `TLS_COMPLETE` |
| Retry non échu | Ne modifie aucun état et ne réémet rien |
| Retry échu | Délègue le GET de reprise `Last-Event-ID` existant |

## Garanties

Le tick exige un état SSE, un scheduler et des sorties de polling valides. Il rejette un délai de base nul ou un maximum inférieur au délai de base. La phase `TLS_COMPLETE` est reconnue comme une attente : un tick avant échéance retourne sans transformer cette absence d’événement en terminaison de flux.

La planification effective reste portée par `ne2k_llm_connection_schedule_sse_retry()`, qui emploie des copies locales avant publication. Le tick ne conserve aucun buffer, secret, endpoint ni état TLS ; tous ces éléments restent caller-owned.

## Tests

Le test NE2000 force un échec de transport dans une session SSE active. Le tick programme alors une échéance à `now + base_delay`, publie `TLS_COMPLETE` et, à l’appel suivant avant échéance, confirme l’absence de mutation. Il vérifie également le rejet d’un délai de base nul.

| Vérification | Résultat |
|---|---|
| Échec transport → retry automatique | Validé |
| Transition vers `TLS_COMPLETE` | Validée |
| Tick avant échéance | Sans mutation |
| Délai de base nul | Rejeté |
| Test NE2000 ciblé | **37/37 réussis** |

## Limites

Le tick est une façade non bloquante et doit être appelée par l’ordonnanceur réseau ou la boucle matérielle du système. Il n’introduit ni attente active ni stockage durable ; la persistance inter-session demeure détenue par le contexte SSE caller-owned livré dans AOS-1769…1776.

## Références

[1] [AOS-921 à AOS-928 — scheduler de reprise SSE NE2000](aos921_928_ne2k_sse_retry_scheduler.md)  
[2] [AOS-1769 à AOS-1776 — contexte SSE persistant inter-session](aos1769_1776_sse_session_context.md)  
[3] [WHATWG — Server-Sent Events](https://html.spec.whatwg.org/multipage/server-sent-events.html)
