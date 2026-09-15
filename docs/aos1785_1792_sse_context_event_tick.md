# AOS-1785…1792 — Tick SSE actif lié au contexte réseau LLM

## Objet

Ce macro-lot raccorde le tick SSE NE2000 au contexte réseau LLM caller-owned. La nouvelle façade `ne2k_llm_network_context_sse_event_tick()` utilise directement la session et la connexion TCP persistées dans le contexte, puis sauvegarde un checkpoint de reprise SSE après chaque tick ayant abouti.

| Élément | Avant | Après |
|---|---|---|
| Tick SSE | Opère sur une session et une connexion passées séparément | Opère sur `context.session` et `context.connection` |
| Reprise persistante | Checkpoint explicite par l’appelant | Checkpoint automatique après tick réussi |
| Publication | États gérés séparément | Contexte, réponse, scheduler et longueurs publiés ensemble |
| Échec | À traiter par l’intégrateur | Pas de mutation du contexte ni des sorties |

## Contrat transactionnel

La façade crée des copies locales du contexte, de la réponse SSE, du scheduler et des longueurs de sortie. Elle exécute ensuite `ne2k_llm_connection_sse_event_tick()` sur ces copies. Sur un état terminal retryable, la phase devient `TLS_COMPLETE`, le scheduler est armé et le checkpoint conserve le fournisseur, le budget consommé et `Last-Event-ID`.

Si le tick ou la sauvegarde de checkpoint échoue, aucun des objets appelants n’est publié. Un tick avant l’échéance de reconnexion est un succès sans émission et maintient le checkpoint précédemment validé. Les délais incohérents restent rejetés par la couche de tick sous-jacente.

> Aucun endpoint, bearer, modèle, buffer HTTP/SSE, secret ou état TLS n’est copié dans le contexte persistant. Le contexte conserve seulement la session, la connexion, le bail DHCP et le checkpoint SSE compact.

## Tests

Le vecteur NE2000 initialise un contexte LLM, simule une erreur transport pendant un flux SSE et vérifie que le tick arme le retry, change la phase, persiste le checkpoint et restaure le fournisseur OpenAI, le budget de retry et l’identifiant `evt`. Il vérifie ensuite l’attente avant échéance et l’absence de mutation du contexte si le délai de base est nul.

| Vérification | Résultat |
|---|---|
| Erreur transport → retry dans le contexte | Validé |
| `TLS_COMPLETE` et échéance à `now + base_delay` | Validés |
| Checkpoint automatique du fournisseur, retry et `Last-Event-ID` | Validé |
| Tick avant échéance | Sans mutation de phase ni d’échéance |
| Délai nul | Rejet transactionnel |
| Test NE2000 ciblé | **38/38 réussis** |

## Références

[1] [AOS-1769 à AOS-1776 — contexte SSE persistant inter-session](aos1769_1776_sse_session_context.md)  
[2] [AOS-1777 à AOS-1784 — tick SSE NE2000](aos1777_1784_ne2k_sse_event_tick.md)  
[3] [WHATWG — Server-Sent Events](https://html.spec.whatwg.org/multipage/server-sent-events.html)
