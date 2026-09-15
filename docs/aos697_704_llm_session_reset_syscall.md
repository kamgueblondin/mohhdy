# AOS-697 à AOS-704 — Syscall de réarmement de session LLM

**Auteur :** Manus AI

**Statut :** implémenté et validé localement

**Périmètre :** réutilisation contrôlée d’une session TLS LLM terminée, sans réacquisition DHCP et sans allocation dynamique

## Objectif

`SYS_LLM_RESET_FOR_REQUEST` permet au shell de préparer un nouveau tour seulement après l’achèvement complet d’une réponse texte ou SSE. Il délègue à `ne2k_llm_network_context_reset_for_request`, qui applique la transition transactionnelle `RESPONSE_READY → TLS_COMPLETE`.

La commande associée est `ai-next`. Elle ne prend aucun argument et ne transporte donc ni endpoint, ni credential, ni adresse réseau, ni clé, ni buffer.

| Élément | Contrat |
|---|---|
| Numéro ABI | `SYS_LLM_RESET_FOR_REQUEST = 96` |
| Argument | Aucun. |
| Phase requise | `RESPONSE_READY`. |
| Succès | `TLS_COMPLETE`, prête pour une nouvelle émission HTTP. |
| Phase incorrecte | `OS_LLM_RESET_BAD_PHASE`, sans mutation. |
| Échec de façade | `OS_LLM_RESET_FAILED`, sans publication partielle. |

## Conservation et nettoyage

Le réarmement conserve le bail DHCP, l’adresse distante associée au contexte LLM, la connexion TCP existante, le client TLS authentifié, le hostname et les matériaux TLS privés au noyau. Il évite donc DHCP, DNS, ARP, SYN et handshake supplémentaires entre deux requêtes d’une même session valide.

Les données applicatives transitoires sont en revanche réinitialisées : le fournisseur HTTP, le marqueur streaming, les longueurs et codes de l’accumulateur HTTP, les vues de réponse, les compteurs SSE ainsi que les buffers de texte, réponse HTTP et SSE sont purgés. Les buffers de records et les clés TLS ne sont pas exposés et restent attachés au client noyau.

> La séparation entre transport conservé et résultats applicatifs purgés empêche qu’un texte ou un fragment SSE du tour précédent soit relu durant le suivant.

| Donnée | Après `ai-next` réussi |
|---|---|
| Bail DHCP et route | Conservés. |
| Connexion TCP et session TLS | Conservées. |
| Phase LLM | `TLS_COMPLETE`. |
| Mode HTTP/SSE | Réinitialisé. |
| Texte, réponse HTTP, buffers SSE | Purge locale noyau. |
| Token, clé, buffer ou IPv4 vers Ring 3 | Jamais publiés. |

## Validation locale

Le smoke QEMU fournisseur invoque `ai-next` au boot, où la session est `IDLE`. Il exige le rejet explicite et vérifie ensuite que phase `IDLE` et bail DHCP absent sont préservés. Le smoke NE2000 confirme l’initialisation normale d’une carte présente.

| Vérification | Résultat |
|---|---|
| Compilation freestanding i386 | Réussie. |
| Smoke QEMU fournisseur | Réussi ; garde `IDLE` et contexte inchangé. |
| Smoke QEMU NE2000 | Réussi ; carte prête, `IDLE`, bail absent. |
| Suite complète | Réussie : **384/384** tests. |
| `git diff --check` | Propre avant commit. |

## Limites connues

Le réarmement ne ferme pas TCP/TLS, ne renégocie pas le handshake et ne fournit pas de timeout ou retry. Il reste dépendant du handshake TLS réel, qui attend un provisionnement noyau d’entropie cryptographique et d’ancre X.509. OpenAI demande également un credential interne. Fermeture explicite, annulation, timeout/retry, historique conversationnel, outils, multimodal et Unicode complet restent hors périmètre.

## Références

[1] [AOS-577 à AOS-584 — réutilisation d’une session TLS LLM](aos577_584_llm_session_reuse.md)

[2] [AOS-689 à AOS-696 — syscall de streaming SSE LLM contrôlé](aos689_696_llm_sse_syscall.md)

[3] [AOS-681 à AOS-688 — syscalls HTTP LLM et polling texte contrôlés](aos681_688_llm_http_text_syscalls.md)
