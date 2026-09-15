# AOS-585 à AOS-592 — Plan de contrôle shell/noyau de session LLM

**Auteur :** Manus AI  
**Statut :** implémenté et validé localement  
**Périmètre :** ABI Ring 3/noyau, état LLM NE2000, diagnostic shell, smoke QEMU déterministe

## Objectif

Ce macro-lot rend visible au shell utilisateur l’état du contexte LLM unifié déjà défini dans le noyau. Il ne prétend pas émettre une requête HTTPS sans configuration réseau ni identifiant ; il installe le **plan de contrôle** qui permet de constater, sans exposer de donnée sensible, l’état de préparation de la session.

> Le shell reçoit uniquement un mot de statut. Il ne reçoit ni adresse IPv4 distante, ni clé TLS, ni secret fournisseur, ni pointeur vers les buffers noyau.

## ABI ajoutée

| Élément | Contrat |
|---|---|
| `SYS_LLM_SESSION_STATUS` | Syscall sans argument, numéro `90`. |
| Bits `0..0` | `1` lorsque le NE2000 de boot est prêt ; `0` sinon. |
| Bits `8..15` | Phase `ne2k_llm_connection_state_t`. |
| `MAX_SYSCALLS` | Porté à `91`. |

Le noyau possède un contexte `boot_llm_connection` initialisé par `ne2k_llm_connection_state_init` pendant la sonde NE2000. Sa phase initiale est donc `IDLE`, même lorsqu’aucun contrôleur NE2000 n’est détecté. `kernel_llm_session_status` compose le mot de statut en lecture seule.

## Intégration shell

La commande existante `ai-runtime` appelle le nouveau syscall et ajoute une ligne du type :

```text
Session LLM noyau  : IDLE (NE2000 absent)
```

| Valeur de phase | Libellé shell |
|---:|---|
| `0` | `IDLE` |
| `1` | `SYN_SENT` |
| `2` | `TLS_STARTED` |
| `3` | `TLS_COMPLETE` |
| `4` | `REQUEST_SENT` |
| `5` | `RESPONSE_READY` |
| `6` | `STREAMING` |
| autre | `INCONNUE` |

La sélection `ai-provider openai` demeure un réglage de contrôle. Le shell avertit correctement que DHCP, DNS, une configuration de destination, la validation TLS et un mécanisme explicite de fourniture d’identifiant sont nécessaires avant un appel réel. Aucun Bearer OpenAI n’est intégré dans l’image de boot.

## Garanties de sécurité et de mémoire

Le syscall ne consomme aucun pointeur utilisateur et n’effectue aucune copie de chaîne. Il ne fournit qu’un entier 32 bits composé d’un bit de disponibilité et de la phase non secrète. Les contextes TCP/TLS/HTTP/SSE, les clés AES-GCM, les buffers et l’adresse distante restent dans le noyau ou restent caller-owned selon les API réseau existantes. Aucune allocation dynamique n’est introduite.

## Tests et validation locale

Le smoke QEMU `qemu-ai-provider` démarre l’OS sans carte NE2000, exécute `ai-runtime` et vérifie explicitement la sortie `Session LLM noyau : IDLE (NE2000 absent)`. Il conserve aussi les contrôles de sélection de fournisseur et de modèle. La compilation complète vérifie l’ABI partagée, le dispatcher et le shell Ring 3.

| Vérification | Résultat |
|---|---|
| Compilation freestanding i386 et shell | Réussie. |
| Smoke QEMU fournisseur / plan de contrôle | Réussi. |
| Smoke QEMU NE2000 | Réussi. |
| Suite complète | **378/378** tests réussis. |

## Limites connues

Cette livraison ne crée pas encore un chemin de requête HTTPS depuis Ring 3. Il manque le provisionnement DHCP ou statique, les endpoints fournisseur, le DNS live, les buffers de service LLM noyau, les syscalls d’émission/polling sous contrôle de capacité, la gestion d’identifiant hors image, les timeouts, le retry/backoff, la fermeture TCP/TLS, l’historique conversationnel, les tool calls, le multimodal et le support Unicode complet.

## Références

[1] [AOS-537 à AOS-544 — orchestrateur LLM DNS vers ClientHello](aos537_544_llm_connection_orchestrator.md)  
[2] [AOS-569 à AOS-576 — façade SSE du contexte LLM](aos569_576_llm_session_sse.md)  
[3] [AOS-577 à AOS-584 — réutilisation d’une session TLS LLM](aos577_584_llm_session_reuse.md)
