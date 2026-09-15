# AOS-545 à AOS-552 — Progression TLS authentifiée dans la session LLM

**Auteur :** Manus AI  
**Statut :** implémenté et validé localement  
**Périmètre :** session LLM caller-owned, polling TLS, chaîne reçue, RTC UTC, NE2000

## Objectif

Ce macro-lot prolonge le contexte de connexion LLM jusqu’au handshake TLS authentifié. La façade ajoute une phase `TLS_COMPLETE` et délègue le traitement des records au polling existant qui utilise la chaîne de certificats reçue et lit l’UTC depuis le RTC i386.

> La politique TLS continue donc d’exiger l’identité X.509 et une date UTC lue au moment du polling. Le contexte LLM ne remplace ni la validation de chaîne ni l’horloge RTC ; il les raccorde à sa progression de phase.

## Phase ajoutée et API

| Phase | Valeur | Signification |
|---|---:|---|
| `NE2K_LLM_CONNECTION_TLS_STARTED` | 2 | ClientHello émis ; le handshake authentifié peut être pollé. |
| `NE2K_LLM_CONNECTION_TLS_COMPLETE` | 3 | Finished serveur validé, session TLS marquée complète. |

| API | Rôle |
|---|---|
| `ne2k_llm_connection_poll_tls` | Polling TLS de session LLM, avec chaîne intermédiaire reçue et instant UTC lu par `rtc_io_t`. |

## Comportement transactionnel

L’API copie le contexte de phase, la connexion TCP et le client TLS avant de déléguer à `ne2k_tls_client_poll_received_chain_rtc`. Les erreurs négatives n’exposent aucune mutation. Un polling non négatif publie les éventuels progrès du handshake et du transport, mais la phase LLM ne passe à `TLS_COMPLETE` que si le client est marqué complet et que l’automate TLS confirme son état final.

| Situation | Résultat |
|---|---|
| Phase différente de `TLS_STARTED` | Rejet sans lecture RTC, réseau ou cryptographie. |
| Erreur RTC, RX, TLS, X.509 ou AEAD | État LLM, TCP et TLS externe inchangé. |
| Polling incomplet ou RX vide | Progrès éventuels publiés ; phase conservée à `TLS_STARTED`. |
| Finished serveur validé | Publication de TCP/TLS et phase `TLS_COMPLETE`. |

Aucune allocation dynamique n’est introduite. Les buffers RX/TX, records de flight, plaintext, workspaces RSA/X25519/PRF, contextes TCP/TLS et l’interface RTC restent tous caller-owned.

## Tests et validation locale

Le harnais NE2000 ajoute une garde de phase pour la nouvelle façade. Elle confirme qu’un appel avant `TLS_STARTED` est rejeté avant toute interaction RTC, réseau ou cryptographique et que la phase ainsi que la séquence TCP restent intactes. Les lots précédents conservent la couverture des records TLS, de la chaîne reçue, de la politique RTC, du rollback AEAD et de l’automate de handshake.

| Vérification | Résultat |
|---|---|
| Suite noyau | **32/32** exécutables réussis. |
| Suite complète | **375/375** tests réussis. |
| Compilation freestanding i386 | Réussie. |
| Smoke QEMU `qemu-ai-provider` | Réussi. |
| Smoke QEMU `qemu-ne2k-status` | Réussi. |

## Limites connues

La façade n’ajoute ni timeout, ni retransmission, ni backoff, ni annulation de handshake. Elle n’établit pas automatiquement une chaîne de certificats de profondeur arbitraire et ne remplace pas les limites cryptographiques existantes. Les phases applicatives POST/streaming LLM restent séparées : elles doivent exiger `TLS_COMPLETE` dans le lot suivant.

## Références

[1] [AOS-497 à AOS-504 — RTC UTC dans la politique NE2000/TLS](aos497_504_ne2k_rtc_time_policy.md)  
[2] [AOS-537 à AOS-544 — orchestrateur de connexion LLM](aos537_544_llm_connection_orchestrator.md)  
[3] [AOS-449 à AOS-456 — chaîne reçue dans NE2000/TLS](aos449_456_ne2k_received_intermediate.md)
