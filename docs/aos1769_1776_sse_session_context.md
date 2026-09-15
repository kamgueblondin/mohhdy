# AOS-1769…1776 — Contexte SSE persistant inter-session NE2000

## Objet

Ce macro-lot raccorde l’état minimal de reprise SSE au contexte réseau LLM NE2000. Le contexte caller-owned peut désormais conserver un checkpoint de reprise comprenant le fournisseur actif, le nombre de tentatives déjà consommées et le `Last-Event-ID` SSE. Aucun bearer, nom d’hôte, chemin, modèle, buffer HTTP/SSE ni état TLS n’est enregistré.

| Élément | Stocké | Exclu |
|---|---|---|
| Reprise SSE | fournisseur, compteur de retries, `Last-Event-ID`, checksum | payload, texte généré |
| Contexte LLM | bail DHCP, phase, connexion TCP, checkpoint | secrets, endpoint, buffers |
| Mémoire | structure fixe caller-owned | allocation dynamique |

## Contrat

`ne2k_llm_network_context_sse_checkpoint()` prépare une copie locale puis publie le checkpoint seulement après la validation du sérialiseur SSE. `ne2k_llm_network_context_sse_resume_load()` utilise une réponse, un fournisseur et un compteur temporaires : en cas de contexte vide, checksum invalide ou format invalide, les sorties de l’appelant restent inchangées.

`ne2k_llm_network_context_sse_resume_clear()` invalide explicitement la reprise, sans modifier le bail DHCP, l’état TCP ni la phase LLM. Le réarmement usuel de requête conserve le checkpoint afin qu’une reconnexion SSE puisse réutiliser `Last-Event-ID` après une transition `RESPONSE_READY` vers `TLS_COMPLETE`.

## Tests

Le vecteur NE2000 couvre l’initialisation, la sauvegarde d’un identifiant `evt`, le réarmement de session, la restauration du fournisseur OpenAI et de deux retries, le rejet non-mutant d’un checksum corrompu, puis l’effacement explicite.

| Vérification | Résultat |
|---|---|
| Checkpoint SSE intègre | Validé |
| Conservation après réarmement | Validée |
| Restauration de fournisseur, budget et identifiant | Validée |
| Corruption de checksum sans mutation des sorties | Validée |
| Effacement explicite | Validé |
| Test NE2000 ciblé | **36/36 réussis** |

## Limites

Le checkpoint réside dans le contexte caller-owned : sa durabilité physique reste du ressort du support qui possède ce contexte. L’intégration automatique au stockage disque et au polling matériel à chaque tick constitue un axe distinct.

## Références

[1] [AOS-953 à AOS-960 — rotation multi-fournisseur](aos953_960_provider_rotation.md)  
[2] [AOS-921 à AOS-928 — scheduler de reprise SSE NE2000](aos921_928_ne2k_sse_retry_scheduler.md)  
[3] [WHATWG — Server-Sent Events](https://html.spec.whatwg.org/multipage/server-sent-events.html)
