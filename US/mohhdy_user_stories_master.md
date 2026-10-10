# MOHHDY - Plan de Développement par User Stories

> **État réel (août 2026).** Document de **vision / planification MOHHDY**, pas le backlog du prototype. Le code i386 (shell Ring 3, overlay ATA, GPT-2 optionnel) est décrit dans [mohhdy_us.md](mohhdy_us.md) et [docs/ETAT_REEL.md](../docs/ETAT_REEL.md).
> Les titres US-001...US-120 ci-dessous **divergent** souvent des fichiers `individual_us/` (numéros 023-025 en double ; US-031+ des fichiers != navigateur-OS du maître). Ne pas traiter cette liste comme un sprint board.

## Vue d'Ensemble du Projet

**Mohhdy** est le nom produit unique : le SE agentique autonome. Ce document conserve le plan de vision historique (user stories) du prototype guest, sans second nom de projet et sans piste "Agent Support" distincte. Il ne decrit pas l'etat livre : voir [mohhdy_us.md](mohhdy_us.md) (guest) et [docs/ETAT_REEL.md](../docs/ETAT_REEL.md). Capacites OS : [mohhdy_agent_support_web.md](mohhdy_agent_support_web.md).

### Vision MOHHDY

MOHHDY sera le premier système d'exploitation où l'intelligence artificielle constitue le cœur même du système, capable de comprendre les instructions utilisateur en langage naturel et de réagir en conséquence. Le système fonctionnera avec un modèle d'IA réduit en local et pourra se connecter à des modèles plus puissants en ligne, créant un écosystème d'intelligence distribuée.

### Caractéristiques Révolutionnaires

- **IA au Cœur** : L'intelligence artificielle est intégrée nativement dans le noyau du système
- **Langage PromptMessage** : Langage universel pour programmer et communiquer avec le système
- **Multi-Plateforme Unifié** : Un seul OS pour desktop, mobile, et tous environnements
- **Navigateur Amélioré** : Système de fichiers basé sur un navigateur web avancé
- **Réseau Distribué** : Interconnexion P2P inspirée de BitTorrent et gRPC
- **Économie Collaborative** : Système de points pour partager les ressources communautaires

## Méthodologie de Développement

### Structure des User Stories

Chaque User Story suit la structure suivante :
- **Identifiant** : Code unique (US-XXX)
- **Titre** : Description concise de la fonctionnalité
- **En tant que** : Rôle utilisateur concerné
- **Je veux** : Fonctionnalité désirée
- **Afin de** : Valeur métier apportée
- **Critères d'Acceptation** : Conditions de validation
- **Spécifications Techniques** : Détails d'implémentation
- **Dépendances** : Liens avec autres US
- **Estimation** : Complexité et effort requis

### Phases de Développement

Le développement MOHHDY est organisé en 8 phases principales :

1. **Phase Foundation** : Migration et modernisation du noyau MOHHDY
2. **Phase AI Core** : Intégration de l'IA au niveau système
3. **Phase Web Runtime** : Développement du navigateur-OS
4. **Phase PromptMessage** : Création du langage universel
5. **Phase P2P Network** : Implémentation du réseau distribué
6. **Phase Multi-Platform** : Adaptation multi-plateforme
7. **Phase Collaborative** : Système de points et économie
8. **Phase Production** : Optimisation et déploiement

## Index des User Stories

### Phase 1 - Foundation (US-001 à US-015)
- US-001 : Migration du noyau MOHHDY vers architecture microkernel
- US-002 : Implémentation du gestionnaire de ressources intelligent
- US-003 : Développement du système de sécurité adaptatif
- US-004 : Création du framework de plugins modulaires
- US-005 : Intégration du système de logging distribué
- US-006 : Développement de l'API système unifiée
- US-007 : Implémentation du gestionnaire de configuration dynamique
- US-008 : Création du système de mise à jour automatique
- US-009 : Développement du moniteur de performance intelligent
- US-010 : Intégration du système de sauvegarde distribuée
- US-011 : Création du gestionnaire d'erreurs prédictif
- US-012 : Développement de l'interface de diagnostic avancée
- US-013 : Implémentation du système de compatibilité legacy
- US-014 : Création du framework de test automatisé
- US-015 : Développement de la documentation interactive

### Phase 2 - AI Core (US-016 à US-030)
- US-016 : Intégration du moteur IA local TensorFlow Lite
- US-017 : Développement du système de compréhension du langage naturel
- US-018 : Création du gestionnaire de modèles IA distribués
- US-019 : Implémentation du système d'apprentissage fédéré
- US-020 : Développement de l'orchestrateur cloud-edge
- US-021 : Création du système de prédiction comportementale
- US-022 : Intégration de l'IA conversationnelle avancée
- US-023 : Développement du système d'optimisation automatique
- US-024 : Création du gestionnaire de contexte intelligent
- US-025 : Implémentation du système de recommandations
- US-026 : Développement de l'IA de sécurité proactive
- US-027 : Création du système d'adaptation d'interface
- US-028 : Intégration de l'IA de gestion des ressources
- US-029 : Développement du système d'analyse prédictive
- US-030 : Création de l'IA de support utilisateur

### Phase 3 - Web Runtime (US-031 à US-045)
- US-031 : Développement du navigateur-OS intégré
- US-032 : Création du gestionnaire d'applications web natives
- US-033 : Implémentation du système de fichiers web
- US-034 : Développement de l'explorateur de fichiers web
- US-035 : Création du gestionnaire de médias web
- US-036 : Intégration des Progressive Web Apps (PWA)
- US-037 : Développement du système de synchronisation web
- US-038 : Création du gestionnaire de cache intelligent
- US-039 : Implémentation du système de sécurité web
- US-040 : Développement de l'interface responsive universelle
- US-041 : Création du système de thèmes adaptatifs
- US-042 : Intégration des Web Components
- US-043 : Développement du gestionnaire de performances web
- US-044 : Création du système d'accessibilité web
- US-045 : Implémentation du débogueur web intégré

### Phase 4 - PromptMessage (US-046 à US-060)
- US-046 : Conception du langage PromptMessage
- US-047 : Développement du compilateur PromptMessage
- US-048 : Création de l'interpréteur PromptMessage
- US-049 : Implémentation du système de validation syntaxique
- US-050 : Développement de l'IDE PromptMessage intégré
- US-051 : Création du système de documentation automatique
- US-052 : Intégration du débogueur PromptMessage
- US-053 : Développement du gestionnaire de bibliothèques
- US-054 : Création du système de versioning de code
- US-055 : Implémentation du compilateur croisé multi-plateforme
- US-056 : Développement de l'optimiseur de code PromptMessage
- US-057 : Création du système de test automatisé PromptMessage
- US-058 : Intégration de l'IA d'assistance au développement
- US-059 : Développement du marketplace de PromptPrograms
- US-060 : Création du système de certification de code

### Phase 5 - P2P Network (US-061 à US-075)
- US-061 : Implémentation du protocole P2P MOHHDY
- US-062 : Développement du gestionnaire de découverte de pairs
- US-063 : Création du système de routage intelligent
- US-064 : Implémentation du protocole de consensus distribué
- US-065 : Développement du gestionnaire de réplication de données
- US-066 : Création du système de chiffrement P2P
- US-067 : Intégration du protocole gRPC optimisé
- US-068 : Développement du gestionnaire de bande passante
- US-069 : Création du système de tolérance aux pannes
- US-070 : Implémentation du gestionnaire de cache distribué
- US-071 : Développement du système de synchronisation P2P
- US-072 : Création du moniteur de santé réseau
- US-073 : Intégration du système de QoS adaptatif
- US-074 : Développement du gestionnaire de NAT traversal
- US-075 : Création du système d'analyse de trafic intelligent

Etat Phase 5 (10 octobre 2026, lot P2P ; detail et limites dans `docs/p2p.md`) :
- Fait (preuve QEMU trois invites `make qemu-p2p` + unitaires) : US-061 protocole, US-062 decouverte, US-063 routage multi-sauts (PR #127, quatre invites en ligne `make qemu-p2p-route`), US-065 replication (LWW), US-066 chiffrement (X25519 + AES-128-GCM par paire, cle de reseau, anti-rejeu ; hub verifie l'absence de clair), US-071 synchronisation (anti-entropie apres partition), US-072 moniteur de sante (RTT, vu, erreurs, pannes).
- Partiel : US-064 consensus (vote majoritaire a un tour, QEMU ; pas Raft/Paxos), US-069 tolerance aux pannes (detection + reroutage + resync, QEMU ; magasin cle/valeur et identite persistes au redemarrage depuis le lot de consolidation #121), US-070 cache distribue (lecture distante, QEMU), US-068 bande passante et US-073 QoS (budget par pair, controle prioritaire ; unitaires seulement, pas adaptatif), US-075 analyse de trafic (compteurs par type + `p2p-analyze` a regles fixes avec verdict depuis #121, QEMU ; pas d'apprentissage).
- Non livre : US-067 gRPC, US-074 NAT traversal (segment unique sans NAT dans le labo).

### Phase 6 - Multi-Platform (US-076 à US-090)
- US-076 : Adaptation du noyau pour architecture ARM
- US-077 : Développement de l'interface mobile native
- US-078 : Création du gestionnaire de capteurs mobiles
- US-079 : Implémentation du système de gestes tactiles
- US-080 : Développement de l'adaptation d'écran automatique
- US-081 : Création du gestionnaire d'énergie intelligent
- US-082 : Intégration des notifications push distribuées
- US-083 : Développement du système de synchronisation multi-appareils
- US-084 : Création de l'interface de continuité cross-platform
- US-085 : Implémentation du gestionnaire de périphériques universels
- US-086 : Développement du système d'émulation legacy
- US-087 : Création du gestionnaire de compatibilité applications
- US-088 : Intégration du système de migration de données
- US-089 : Développement de l'interface d'administration unifiée
- US-090 : Création du système de déploiement automatisé

Etat Phase 6 (10 octobre 2026, socle i386 ; detail dans `docs/platform.md`, preuve `make qemu-platform`) :
- Fait (sur cette plate-forme) : US-080 adaptation d'ecran (`screen-adapt WxH` re-agence le bureau VBE a cette taille, verifie par screendump QEMU), US-083 synchronisation multi-appareils (`sync-push`, fichiers <= 300 octets entre invites P2P, dernier ecrivain gagne, somme verifiee), US-084 continuite (`session-handoff`/`session-resume` : repertoire, variables, historique recent), US-087 compatibilite applicative (ELF), US-088 migration de donnees (ramfs, archive verifiee), US-090 deploiement automatise (manifeste local, retour arriere).
- Partiel : US-076 (socle HAL i386 seulement, aucun portage ARM), US-079 gestes (classifieur, pas de peripherique tactile), US-081 energie (profils appliques : quantum de preemption du noyau 10/20/40 ticks via SYS_SCHED_TUNE et cadence d'attente du shell ; pas de batterie ni de frequence CPU), US-082 notifications (file locale), US-085 peripheriques (registre a partir des sondes existantes), US-089 administration (vue agregee).
- Non livre : US-077 interface mobile, US-078 capteurs, US-086 emulation legacy.
- Preuves des ajouts : `make qemu-desktop-partials` (une invite VGA) et `make qemu-fleet` (trois invites P2P).

### Phase 7 - Collaborative (US-091 à US-105)
- US-091 : Implémentation du système de points MOHHDY
- US-092 : Développement du gestionnaire de ressources partagées
- US-093 : Création du système d'authentification distribuée
- US-094 : Implémentation du marketplace de ressources
- US-095 : Développement du système de réputation utilisateur
- US-096 : Création du gestionnaire de tâches distribuées
- US-097 : Intégration du système de paiement décentralisé
- US-098 : Développement du système d'audit transparent
- US-099 : Création du gestionnaire de contrats intelligents
- US-100 : Implémentation du système de gouvernance communautaire
- US-101 : Développement du système de résolution de conflits
- US-102 : Création du gestionnaire de données personnelles
- US-103 : Intégration du système de confidentialité avancée
- US-104 : Développement du système de conformité réglementaire
- US-105 : Création du système de support communautaire

Etat Phase 7 (10 octobre 2026, PR #119 empilee sur #117 ; detail dans `docs/collab.md`, preuve `make qemu-collab`) :
- Fait : US-091 points, US-092 ressources partagees, US-094 marche avec recherche et encheres (PR #127), US-099 contrats definis par l'utilisateur en PromptMessage (PR #127), US-101 litiges arbitres par les membres (PR #127), US-095 reputation, US-096 taches distribuees, US-098 audit, US-100 gouvernance, US-102 donnees personnelles, US-105 support.
- Partiel : US-093 authentification (cle de reseau, pas de signature par noeud), US-103 confidentialite (champs prives locaux, redaction), US-104 conformite (export et oubli seulement).
- Non livre : US-097 paiement reel (exclu).

### Phase 8 - Production (US-106 à US-120)
- US-106 : Optimisation des performances système globales
- US-107 : Développement du système de monitoring en production
- US-108 : Création du système de déploiement continu
- US-109 : Implémentation du système de rollback automatique
- US-110 : Développement du système de mise à l'échelle automatique
- US-111 : Création du système de sauvegarde et récupération
- US-112 : Intégration du système de sécurité en production
- US-113 : Développement du système d'analyse de logs avancée
- US-114 : Création du système d'alertes intelligentes
- US-115 : Implémentation du système de maintenance prédictive
- US-116 : Développement du système de support technique
- US-117 : Création du système de formation utilisateur
- US-118 : Intégration du système de feedback continu
- US-119 : Développement du système de métriques business
- US-120 : Création du système de roadmap évolutive

Etat Phase 8 (10 octobre 2026, PR #120 ; detail dans `docs/production.md`, preuve `make qemu-production`) :
- Fait : US-107 monitoring, US-109 retour arriere automatique, US-110 mise a l'echelle sur la file d'execution mesuree (PR #127), US-111 sauvegarde/restauration (RAM), US-112 audit de securite (PR #127), US-113 analyse de journaux, US-114 alertes a seuil adaptatif EWMA (PR #127), US-118 retours.
- Partiel : US-106 (mesures ; collecteur de metriques et journaux entre invites `fleet-report`/`fleet-collect`), US-108 (deploiement local, et deploiement par etapes entre invites `deploy-stage` canari -> `deploy-promote` -> `deploy-rollback` avec accuses par noeud), US-115 (tendance lineaire), US-116 (diagnostic), US-117 (lecons fixes), US-119 (compteurs de commandes), US-120 (resume statique).

## Estimation Globale

### Complexité par Phase
- **Phase 1 - Foundation** : 180 jours-homme
- **Phase 2 - AI Core** : 240 jours-homme
- **Phase 3 - Web Runtime** : 200 jours-homme
- **Phase 4 - PromptMessage** : 220 jours-homme
- **Phase 5 - P2P Network** : 260 jours-homme
- **Phase 6 - Multi-Platform** : 180 jours-homme
- **Phase 7 - Collaborative** : 200 jours-homme
- **Phase 8 - Production** : 160 jours-homme

**Total Estimé** : 1,640 jours-homme (~= 6.5 années avec une équipe de 10 développeurs)

### Ressources Recommandées
- **Architectes Système** : 2-3 experts
- **Développeurs IA** : 3-4 spécialistes
- **Développeurs Système** : 4-5 experts C/C++/Rust
- **Développeurs Web** : 3-4 experts JavaScript/WebAssembly
- **Ingénieurs Réseau** : 2-3 spécialistes P2P
- **Experts Sécurité** : 2 spécialistes
- **DevOps/Infrastructure** : 2-3 ingénieurs
- **UX/UI Designers** : 2-3 designers
- **Testeurs QA** : 3-4 testeurs
- **Product Managers** : 1-2 managers

## Prochaines Étapes

1. **Validation du Plan** : Révision et approbation du plan global
2. **Constitution de l'Équipe** : Recrutement des experts nécessaires
3. **Setup Infrastructure** : Mise en place de l'environnement de développement
4. **Démarrage Phase 1** : Lancement des premiers sprints de développement
5. **Itération Continue** : Développement agile avec feedback régulier

Ce document constitue la base du développement MOHHDY. Chaque User Story sera détaillée dans des documents séparés avec des spécifications techniques complètes.

## Feuille de route opérationnelle post-baseline — octobre 2026

Cette section traduit la vision historique ci-dessus en ordre d'implémentation concret. Elle ne marque aucune User Story comme livrée ; l'état livré reste celui de `docs/ETAT_REEL.md`.

### Baseline de départ

- Branche : `main`, commit `dcc248a`.
- `make all` : succès.
- `make test-all` : **678/678 réussis**, 0 échec, 0 ignoré.
- Répartition : 53 kernel, 4 userspace, 1 robustness.
- La couverture n'est pas mesurée ; les contrats QEMU doivent être relancés séparément.

### Ordre de réalisation

#### Étape 1 — Contrat IA local (priorité immédiate)

**But :** rendre explicite la différence entre complétion GPT-2, assistant instruction-tuned et fournisseur réseau.

**Critères d'acceptation :**

- `ai-runtime` expose le profil, le modèle, le contexte, la latence et la disponibilité réels.
- La sortie GPT-2 est présentée comme une complétion et non comme une réponse factuelle garantie.
- Les erreurs de modèle, de worker, de timeout et de réseau sont distinguées.
- Les tests couvrent le chemin worker Ring 3, le repli et la sélection de profil.

**Etat du lot du 7 octobre 2026 :** la commande `ai-runtime json` et sa sortie humaine sont livrees dans le shell du guest. Elles publient le profil FP32 ou GGUF, le modele, l'execution QEMU, les limites de contexte et de sortie, la nature de complétion et l'etat reseau. Le test QEMU GPT-2 et `make test-all` passent, avec **678/678** tests reussis. Restent a implementer dans cette meme etape le suivi precis de latence, les erreurs distinguees, le contrat assistant instruction-tuned et le contrat reseau complet.

**Etat du lot du 10 octobre 2026 :** livres dans le guest QEMU : latence mesuree de chaque appel GPT-2 (FP32 et GGUF 109/110, ticks 100 Hz), erreurs distinguees (`model-missing`, `model-failed`, `cancelled`, `no-worker`) et raison d'abandon du job relaye (`worker-lost`, `stalled`, `timeout`, `cancelled`), vivacite reelle du worker Ring 3 (battement par couche de transformeur, job declare bloque apres 15 s sans battement au lieu de 300 s), declaration explicite `assistant: not-available` (GPT-2 de base) et detail reseau (`nic`, `dhcp_lease`, `tls_entropy`, `x509_anchor`) dans `ai-runtime json`. Preuves : tests unitaires `test_ai_relay`, `test_gpt2_generate`, contrat QEMU `qemu-ai-worker` etendu (blocage detecte, annulation Echap). Restent : un vrai modele instruction-tuned et le contrat reseau de bout en bout vers un fournisseur.

#### Étape 2 — Parcours OS-UI IA réel

**But :** relier l'interface graphique à une session IA effectivement exécutée.

**Critères d'acceptation :**

- Le parcours `gui -> session IA -> worker -> réponse -> affichage` est démontré en QEMU.
- Le chat stub, une réponse générée, une erreur et l'absence de modèle sont visuellement distincts.
- Les sessions ont un historique borné, un statut, une annulation et une fin explicite.
- Un test QEMU de bout en bout couvre l'ouverture, la requête, la réponse et la fermeture.

**Etat du lot du 8 octobre 2026 :** le sous-parcours `chat ai ...` est livre dans le meme OS guest QEMU. Il appelle le syscall GPT-2 local, retourne la reponse dans le chat, conserve un historique borne, publie `idle/generating/ready/error` et distingue `gpt2_local` de `stub_echo` dans la scene VGA. Le contrat QEMU couvre une generation reelle et passe ; la suite complete reste a **678/678**. Restent ouverts dans cette etape : ouverture/fermeture GUI automatisee de bout en bout, annulation effective, affichage visuel distinct de l'erreur et de l'absence de modele, et gestion de fin explicite de session.

**Etat du lot du 10 octobre 2026 :** livres : annulation effective par la touche Echap (job relaye abandonne sans rejeu Ring 0, le worker s'arrete au battement suivant ; generation Ring 0 arretee au jeton suivant ; code -149), etats et libelles de scene distincts `no_model`/`gpt2_missing`, `error`/`gpt2_error`, `cancelled`/`gpt2_cancelled` avec la ligne `etat_ia=` sur la scene VGA, et fin explicite `session-end` (historique efface, chat refuse ensuite). Preuves : `test_osui_runtime` et contrat QEMU OS-UI (etat sans modele en CI, fin de session). Reste : ouverture/fermeture GUI automatisee de bout en bout.

**Etat du lot 2 du 10 octobre 2026 :** etape 2 close. Le contrat QEMU `qemu-osui-gui` ouvre le bureau VBE (`gui`), envoie `chat ai a` dans la GUI, attend la reponse (ou, sans poids GPT-2 empaquetes comme en CI, l'etat distinct `llm=gpt2_missing ai=no_model` dans l'instantane `OSUI-SNAP`, qui affichait auparavant `llm=stub_echo` en dur), termine la session (`session-end`, `ai=ended`), ferme (`console`) et verifie `gui_live=false` cote shell texte.

#### Étape 3 — Sessions et capacités agentiques bornées

**But :** donner à l'agent des actions utiles sans accès implicite illimité.

**Critères d'acceptation :**

- Les sessions peuvent être créées, restaurées, expirées et nettoyées.
- Les capacités de lecture VFS, recherche et commandes allowlistées sont accordées par scope.
- Toute mutation exige une confirmation et toute opération est révocable et traçable.
- Les tests couvrent succès, refus, expiration, worker disparu et capacité insuffisante.

**Etat du lot 2 du 10 octobre 2026 :** livres dans OS-UI Ring 3 (guest QEMU) : expiration des sessions inactives (`session-ttl <s>`, statut `expired`, mutation en attente abandonnee, historique garde), `session-restore <id>` (seulement une session expiree ; une session terminee par `session-end` n'est pas restaurable), `session-cleanup` (libere les sessions fermees), confirmation explicite de toute mutation (`mcp-invoice` rend un jeton `cNNNN`, `confirm`/`deny`, jeton a usage unique, capacite reverifiee a la confirmation donc une revocation entre-temps refuse), mutation revocable (`mcp-invoice-void`), et journal (`admin-status`) de chaque expiration, restauration, nettoyage, refus et annulation. Preuves : `test_osui_runtime` (cycle de vie, confirmation/refus/revocation/expiration) et contrat QEMU OS-UI (confirm/deny/void, expiration reelle a 2 s, restauration avec historique, nettoyage). Complements du meme lot : lecture VFS accordee par scope (`grant fs.read:<prefixe>`, aucune lecture sans scope, scope verifie aussi via l'agent), commandes de l'agent allowlistees et en lecture seule (`agent-run <commande>` sous la capacite `agent.run` : os-status, session-status, fs-list, fs-read, gui-status, stage ; toute mutation refusee `command_not_allowlisted`), et etat distinct quand le worker IA disparait (`ai_status=worker_lost`, `llm=gpt2_no_worker`, ligne `etat_ia=worker IA perdu` sur la scene, session gardee ouverte, nouvel essai possible). Preuves : `test_osui_runtime` (scopes, agent-run, worker perdu) et contrat QEMU OS-UI (scope refuse puis accorde, agent-run accorde/refuse). Lot 3 : le worker perdu est prouve en QEMU depuis OS-UI (worker affame puis tue : `ai_status=ready worker=stalled fallback=ring0`, puis service direct Ring 0) ; l'etat sans aucune reponse (`worker_lost`) reste prouve en test unitaire, le repli Ring 0 repondant toujours en QEMU quand le modele est present.

#### Étape 4 — Fournisseur réseau et repli local

**But :** passer des pairs QEMU locaux à un contrat réseau robuste sans confondre la preuve de test avec Internet public.

**Critères d'acceptation :**

- Les secrets ne sont jamais intégrés à l'image.
- TLS, requête, streaming, fermeture, timeout et erreurs sont testés séparément.
- Le repli local est explicite et observable.
- La documentation conserve la distinction entre pair QEMU, hôte réel et Internet public.

**Etat du lot 3 du 10 octobre 2026 :** partiel. Livre : aucun secret dans les images (`make secrets-check` en CI), fournisseur OS-UI `peer` (pair QEMU local via le networker, pas d'Internet public) avec repli local explicite et observable dans chaque reponse (`provider=peer fallback=local reason=no_net_worker|no_nic|no_dhcp_lease|no_trust_anchor|peer_request_not_wired`). TLS, requete, streaming, fermeture et erreurs sont testes separement par les contrats shell `qemu-ne2k-tls-*` et `qemu-net-peer-tls-worker`. Lot 4 : OS-UI envoie la requete au pair QEMU lui-meme (echange borne, session TLS rearmee, une raison de repli par etape) et le timeout est un contrat distinct (`qemu-osui-peer` : pair arrete, `reason=response_timeout`). Critere d'acceptation de l'etape 4 tenu pour le pair QEMU ; hote reel et Internet public : hors perimetre.

#### Étape 5 — Web Runtime après stabilisation de l'IA

**But :** fournir une supervision et une console web contrôlées avant toute prétention de navigateur-OS.

**Critères d'acceptation :**

- Une API locale contrôlée expose le statut et les sessions autorisées.
- Le VFS et la console IA web respectent les mêmes capacités que le guest.
- Les routes, erreurs, authentification et limites de ressources sont testées.
- `phase3_complete=false` reste inchangé tant qu'un moteur navigateur réel n'est pas livré ; Chromium/WebKit n'est pas déclaré présent par anticipation.

**Etat du lot 3 du 10 octobre 2026 :** API locale controlee livree dans OS-UI Ring 3 (guest QEMU, sans socket ni moteur navigateur) : `/status` public, `/sessions` limite a la session du jeton, `/vfs/<chemin>` sous les memes scopes `fs.read:` que la console, `/ai/chat` par le meme chemin de chat ; jeton `api-token` sous `web.api`, `api-revoke` ; erreurs 400/401/403/404/405/413/429 testees (unitaire et QEMU). Lot 4 : route `/supervision` (workers IA et reseau, NIC, journal) sous `admin.observe`. Lot 5 : API et console servies sur le reseau en HTTP/1.0 via le networker Ring 3 (`web-serve`, `SYS_PEER_DATA`), contrat QEMU `qemu-osui-web` (client HTTP hote, pas d'Internet). Lot 6 : HTTPS (TLS 1.2, certificat de test), 4 connexions concurrentes, `web-serve` en arriere-plan (console utilisable), 1 Kio par appel ; contrat `qemu-osui-web` etendu. Reste : moteur de rendu, retransmission TCP cote invite, certificat de confiance ; `phase3_complete=false` inchange.

#### Phase 4 - PromptMessage (ouverte le 10 octobre 2026)

**Etat du lot phase 4 du 10 octobre 2026 :** livres dans le shell guest QEMU (C, Ring 3), voir `docs/promptmessage.md` : US-046 langage (grammaire a mots fixes, exemples de la synthese executables), US-047 compilateur (bytecode, image PMC1 avec somme de controle), US-048 interpreteur (VM bornee, `pm-run`, `pm`, declencheurs `pm-say`), US-049 validation syntaxique (ligne et colonne), US-050 editeur de lignes `pm-edit` et IDE plein ecran `pm-ide` sur l'ecran texte VGA (saisie, curseur, defilement, sauvegarde et verification avec erreur dans la ligne d'etat ; preuve `make qemu-desktop-partials`), US-051 documentation automatique `pm-doc`, US-052 debogueur (`pm-debug` trace et point d'arret, `pm-disasm`), US-053 bibliotheques (`use`, un niveau), US-054 versions (`pm-version`, `pm-versions`, 9 instantanes), US-056 optimiseur (repli de constantes), US-057 tests automatises (`expect`, `pm-test`), US-059 partiel (catalogue local `pm-catalog`/`pm-install`, pas de marketplace reseau), US-060 partiel (`pm-certify`/`pm-verify` : enregistrement local, somme FNV-1a et tests passes, pas une signature). Non livres : US-055 compilateur croise, US-058 assistance IA (pas de modele instruit hors ligne). Preuves : `test_promptmessage` et `make qemu-promptmessage` en CI. Depuis le lot de consolidation #121 : US-060 ajoute un fichier `.sig` (signature Schnorr de la cle du noeud, verifiee par `pm-verify`), sans autorite de certification.

### Règle de progression

Une étape ne passe à l'état livré qu'après implémentation, test ciblé, test de régression (`make test-all`) et mise à jour de `docs/ETAT_REEL.md`. Les compteurs historiques de ce document sont conservés pour la traçabilité ; le compteur vivant de la baseline courante est **678/678**.

## Regles obligatoires du backlog

Ces regles completent toutes les phases et toutes les User Stories du document :

1. Toutes les User Stories US-001 a US-120 restent a implementer. Aucune User Story et aucune fonctionnalite demandee n'est annulee.
2. Toutes les implementations doivent fonctionner dans le meme systeme d'exploitation MOHHDY lance dans QEMU.
3. Il n'y aura pas d'application parallele, de prototype detache, de second code source ou de service qui remplace le guest MOHHDY.
4. Le noyau, Ring 3, l'OS-UI, l'IA locale, l'IA reseau, le stockage, le Web Runtime, PromptMessage, le reseau P2P, le multi-plateforme, la collaboration et la production doivent etre integres progressivement dans le meme code et le meme systeme.
5. Une fonctionnalite est livree uniquement lorsqu'elle est implementee dans le guest, utilisable depuis MOHHDY dans QEMU et couverte par un test reproductible.
6. Une limite technique ou une etape non terminee est un travail restant. Elle ne constitue jamais une annulation du besoin.
7. Le Web Runtime et toute interface web future sont des composants de MOHHDY executes dans QEMU. Ils ne sont pas un produit independant.

La feuille de route operationnelle doit etre l'ordre de construction de l'ensemble des phases, et non une selection qui reduit le perimetre historique. Les priorites indiquent l'ordre de travail, mais elles ne suppriment aucune User Story.
