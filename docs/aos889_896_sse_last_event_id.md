# AOS-889 à AOS-896 — reprise SSE avec Last-Event-ID

## Objectif

Ce macro-lot mémorise le champ SSE `id:` dans un identifiant ASCII borné à 32 octets. Les lignes `id:` sont acceptées avec les lignes `data:` déjà supportées ; leur valeur est validée contre les caractères de contrôle et conservée dans l’accumulateur caller-owned. Les lignes `data:` restent concaténées avant extraction JSON, y compris lorsqu’un événement contient plusieurs lignes.

Le builder `net_http_build_sse_resume_get` construit un GET HTTP/1.1 avec `Last-Event-ID`, sans transmettre de caractères de contrôle ni dépasser la capacité caller-owned. `net_llm_sse_build_resume_get` expose la même opération depuis une réponse SSE. Le reset utilisé avant reconnexion remet à zéro les buffers HTTP/SSE mais conserve le dernier identifiant, de sorte que l’appelant puisse construire une reprise cohérente.

| Élément | Garantie |
|---|---|
| Identifiant | ASCII imprimable, 0 à 32 octets |
| Framing | `id:` et `data:` stricts, inconnus rejetés |
| Reprise | En-tête HTTP `Last-Event-ID` borné |
| Reset | Buffers vidés, identifiant conservé |
| Sécurité | Contrôles rejetés et aucune fuite hors buffers caller-owned |
| Mémoire | Aucun état global et aucun `kmalloc` |

## Validation

Le test injecte `id: 42`, extrait le delta JSON `ok`, vérifie la requête GET complète, réinitialise la réponse SSE et reconstruit le même GET depuis l’identifiant conservé. La suite HTTP/TLS ciblée passe à **22/22 tests**. La suite globale doit confirmer la non-régression i386.

## Limites restantes

L’identifiant est conservé dans la session applicative mais n’est pas encore injecté automatiquement dans le poller NE2000/TLS ; l’appelant doit construire et réémettre explicitement le GET lorsqu’une deadline de reconnexion est atteinte. La persistance multi-session, la rotation de fournisseur, le jitter et la reprise exacte au milieu d’un événement restent hors périmètre.
