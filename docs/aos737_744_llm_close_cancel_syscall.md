# AOS-737 à AOS-744 — Fermeture et annulation contrôlées de session LLM

**Auteur :** Manus AI

**Statut :** implémenté et validé localement

**Périmètre :** annulation explicite d’une session LLM noyau avec tentative de fermeture TCP, effacement des données de session et conservation du seul bail DHCP.

## Contrat

`SYS_LLM_CLOSE` et la commande `ai-close` ne reçoivent aucun argument depuis Ring 3. Une session `IDLE` est refusée sans modifier le statut. Pour une session active, le noyau tente un `FIN+ACK` seulement lorsque le transport est réellement `ESTABLISHED`. Le helper NE2000 déjà transactionnel ne publie `FIN_WAIT_1` qu’après émission de trame réussie ; quelle que soit cette tentative, l’annulation poursuit ensuite sa purge locale.

| Situation | Résultat de contrôle | État local final |
|---|---|---|
| Session `IDLE` | `OS_LLM_CLOSE_BAD_PHASE` | Inchangé. |
| TCP non établi | Succès d’annulation sans FIN. | Session `IDLE`, bail conservé. |
| TCP établi et FIN transmis | Succès d’annulation. | Session `IDLE`, bail conservé. |
| TCP établi et FIN non transmis | `OS_LLM_CLOSE_FIN_FAILED`. | Purge locale terminée, bail conservé. |

> L’échec d’un FIN n’empêche jamais l’effacement local. Cette priorité évite qu’une erreur de transport conserve des secrets éphémères, un transcript ou des fragments de prompt/réponse.

## Données effacées

La purge efface le contexte TCP et l’IPv4 distante, les matériaux RDRAND X25519, les secrets maître et de session TLS, le transcript, les records, les buffers RSA/X25519/PRF, les accumulations HTTP/SSE, les fragments de prompt/requête/réponse et le hostname. Le client TLS est ensuite réinitialisé avec ses buffers noyau statiques afin de pouvoir démarrer un futur bootstrap propre.

| Donnée | Traitement |
|---|---|
| `net_dhcp_lease_t` validé | Conservé. |
| Cache ARP | Réinitialisé. |
| TCP et IPv4 distante | Effacés. |
| Aléas et clé privée X25519 | Effacés. |
| Secrets AES-GCM, master secret, transcript | Effacés. |
| JSON, HTTP, plaintext, texte et SSE | Effacés. |
| Ancre X.509 compilée et capacité RDRAND | Conservées comme invariants de boot. |

Aucune allocation dynamique n’est ajoutée. Tous les buffers restent des objets statiques du noyau ; le syscall n’expose ni pointeur interne, ni clé, ni adresse, ni détail de bail.

## Interface shell et tests

`ai-close` restitue un succès lorsque le FIN est transmis ou qu’il n’était pas applicable. Si un FIN TCP établi n’a pas pu être émis, le shell annonce explicitement la purge locale terminée. Le smoke QEMU fournisseur vérifie le refus déterministe de `ai-close` depuis `IDLE`, suivi de la conservation de la phase et de l’absence de bail dans le scénario sans NE2000.

| Vérification | Résultat |
|---|---|
| Build i386 freestanding | Réussi. |
| Smoke QEMU fournisseur | Réussi. |
| Smoke QEMU NE2000 | Réussi. |
| Suite complète | **385/385** tests réussis. |

## Limites connues

Cette annulation ne conduit pas le handshake TCP complet `FIN_WAIT_1 → FIN_WAIT_2 → CLOSED`, car le contexte local est intentionnellement purgé immédiatement après la tentative best-effort. La fermeture TLS `close_notify`, la réception/polling du FIN pair, ECDSA, révocation, credentials OpenAI, timeout/retry, outils, multimodal et Unicode complet restent hors périmètre.

## Références

[1] [RFC 5246 — TLS 1.2, Alert Protocol et fermeture](https://datatracker.ietf.org/doc/html/rfc5246)

[2] [AOS-697 à AOS-704 — syscall de réarmement de session LLM](aos697_704_llm_session_reset_syscall.md)
