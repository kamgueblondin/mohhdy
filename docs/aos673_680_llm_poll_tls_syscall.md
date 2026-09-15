# AOS-673 à AOS-680 — Syscall de polling TLS LLM contrôlé

**Auteur :** Manus AI

**Statut :** implémenté et validé localement

**Périmètre :** contrôle Ring 3 du polling SYN-ACK/TLS sans transfert de buffers, secrets ou politique de confiance

## Objectif

Ce macro-lot ajoute `SYS_LLM_POLL_TLS`, qui permet au shell de demander une progression du handshake après le bootstrap DHCP→DNS→SYN. Le syscall ne prend aucun argument. Il opère exclusivement sur le contexte LLM, la connexion TCP, le client TLS, les buffers de records, les espaces de travail cryptographiques et l’interface RTC persistants du noyau.

La commande correspondante est `ai-tls-poll`. Elle ne reçoit ni hostname, ni identifiant, ni token, ni clé, ni buffer TLS. Elle restitue uniquement le code de contrôle transformé en diagnostic lisible.

## Contrat de contrôle

| Élément | Contrat |
|---|---|
| Numéro ABI | `SYS_LLM_POLL_TLS = 92` |
| Registres d’entrée | Aucun argument. |
| Phases acceptées | `SYN_SENT` pour le traitement SYN-ACK/ClientHello, ou `TLS_STARTED` pour le polling authentifié. |
| Buffers | Tous statiques, bornés et exclusivement noyau. |
| Succès `0` | Une progression TLS transactionnelle a été publiée. |
| Attente positive | Une trame ou un progrès supplémentaire est requis. |
| Retour négatif | Phase incorrecte, NIC absente, prérequis TLS non configurés ou échec sans publication partielle. |

La préparation lors de `SYS_LLM_ACQUIRE_START` initialise le client TLS et retient uniquement une copie bornée du hostname non secret pour la future validation d’identité. Le contexte réseau LLM continue de ne pas retenir de buffer, endpoint ou clé ; les données de contrôle TLS sont séparées et privées au noyau.

## Politique de sécurité

Le polling TLS authentifié exige deux prérequis qui ne sont pas encore provisionnés par ce dépôt : une source d’entropie cryptographique pour `client_random` et la clé éphémère X25519, ainsi qu’une ancre X.509 de production vérifiée. Le RTC fournit une heure UTC pour la validation temporelle, mais n’est pas une source d’entropie.

> Le syscall refuse donc `OS_LLM_TLS_UNCONFIGURED` avant toute émission de ClientHello si ces matériaux ne sont pas présents. Il ne fabrique pas de hasard à partir de l’horloge, n’accepte pas une ancre vide et ne dégrade jamais la vérification X.509 pour rendre le test plus facile.

| Garde | Code | Publication de session |
|---|---:|---|
| NE2000 absent | `OS_LLM_ACQUIRE_UNAVAILABLE` | Aucune. |
| Phase différente de `SYN_SENT` ou `TLS_STARTED` | `OS_LLM_TLS_BAD_PHASE` | Aucune. |
| Entropie ou ancre X.509 indisponible | `OS_LLM_TLS_UNCONFIGURED` | Aucune. |
| Erreur RTC, RX, TLS, X.509 ou AEAD | `OS_LLM_TLS_FAILED` | Aucune mutation partielle exposée. |
| Polling valide | `0` ou valeur positive | Progression transactionnelle seulement. |

## Mémoire et confinement

Le noyau utilise un client TLS persistant, deux buffers de records de 8 Kio, un transcript de 8 Kio, un buffer ClientHello de 512 octets, deux espaces de travail bigint/X25519 de 224 mots et des buffers bornés pour PRF, segments TCP, flight et plaintext. Aucun de ces objets n’est alloué dynamiquement ou accessible par une adresse exposée au shell.

Les clés et aléas restent des emplacements noyau non initialisés tant qu’un futur provisionnement interne sécurisé ne les a pas validés. Le syscall n’offre aucune voie Ring 3 pour lire ou écrire ces matériaux.

## Validation locale

Le smoke QEMU fournisseur exécute `ai-tls-poll` sans NE2000. Il exige le diagnostic d’indisponibilité et vérifie ensuite que le statut LLM demeure `IDLE` avec bail DHCP absent. Le scénario confirme que l’entrée de polling ne publie pas d’état sur cette garde.

| Vérification | Résultat |
|---|---|
| Compilation freestanding i386 | Réussie. |
| Smoke QEMU fournisseur | Réussi ; garde TLS sans NIC et contexte inchangé. |
| Smoke QEMU NE2000 | Réussi ; carte prête, phase `IDLE`, bail absent. |
| Suite unitaire complète | Réussie : **384/384** tests. |

## Limites connues

Un lot de provisionnement noyau doit encore fournir une entropie cryptographique et une ancre X.509 de production avant que le poller puisse émettre un ClientHello réel. Après cela, les prochains syscalls pourront achever le polling TLS, émettre la requête HTTP chiffrée et lire texte ou SSE. Les identifiants fournisseur, timeout/retry, fermeture, outils, multimodal et Unicode complet restent hors périmètre.

## Références

[1] [AOS-545 à AOS-552 — progression TLS authentifiée dans la session LLM](aos545_552_llm_tls_session_progress.md)

[2] [AOS-353 à AOS-360 — politique temporelle dans l’orchestrateur NE2000](aos353_360_ne2k_tls_time_policy.md)

[3] [AOS-665 à AOS-672 — syscall contrôlé de démarrage DHCP vers LLM](aos665_672_llm_acquire_start_syscall.md)
