# Plan Lot 48 Mohhdy OS - Inventaire et Feuille de Route PR A à D

**Date :** October 2026
**Base :** origin/main (post PR #204 / Lot 47)

---

## 1. Inventaire Réel des User Stories (Phases 3 à 8 + Phase 9)

Cet inventaire vérifie l'état réel dans le code et les tests (sans se fier uniquement aux déclarations passées).

### Phase 3 - Web Runtime (US-031 à US-045)
- **US-031 (Navigateur-OS / Moteur web)** : `partial`. Moteur minimal `userspace/webeng.c` (HTML tokenizer, cascade CSS, layout bloc/inline, display list VBE, scroll, liens cliquables). Manque : JavaScript dynamic VM, layout flex/grid complet, rendu réseau direct (se fait via `web FILE`). Faisable/prouvable dans QEMU : oui.
- **US-032 (Gestionnaire apps web natives)** : `done`. Multi-onglets, onglets ancrés/renommés, isolation par origine (`userspace/osui_runtime.c`), stockage local chiffré (`browser-fs-save`).
- **US-033 (Système de fichiers web)** : `done`. Périphérique VFS/Web, inspection VFS dans `/browser` et `/fs`.
- **US-034 (Explorateur de fichiers web)** : `done`. Application graphique explorateur (`userspace/osui_files.c`, `/files`), arborescence VFS, actions CRUD.
- **US-035 (Gestionnaire de médias web)** : `done` / `simule`. Commandes `media-play`, `media-stream` avec ticks dans l'event loop (`userspace/osui_runtime.c`). Flux audio/vidéo simulé sans sous-système audio matériel QEMU.
- **US-036 (Progressive Web Apps - PWA)** : `done`. Manifestes PWA (`pwa-manifest`, `pwa-install`, `pwa-run`), origines isolées `pwa://`.
- **US-037 (Synchronisation web)** : `done`. Synchronisation VFS/Web local (`browser-fs-sync`).
- **US-038 (Gestionnaire de cache intelligent)** : `done`. Cache HTTP LRU (`web-cache`), en-têtes ETag/Cache-Control.
- **US-039 (Sécurité web & CSP)** : `done`. Assainisseur HTML (`web-sanitize`), parseur et vérificateur CSP (`webrt_csp_enforce`, `webrt_csp_validate_display_list_resource`), journal d'audit `osui-audit`.
- **US-040 (Interface responsive universelle)** : `done`. `@media` queries (width/height/aspect-ratio), adaptation aux résolutions VBE.
- **US-041 (Thèmes adaptatifs & variables CSS)** : `done`. Variables CSS (`var()`), thèmes clair/sombre (`web-theme`).
- **US-042 (Web Components)** : `done`. Compagnon `<template id=...>` et `<slot>` développés sans JS framework.
- **US-043 (Gestionnaire de performances web)** : `done`. Mesures de cycles et temps de rendu (`web-perf`, `browser-telemetry perf`).
- **US-044 (Accessibilité web - a11y)** : `done`. Audit d'accessibilité `web-a11y` (balises lang, alt, structure H1..H6, labels).
- **US-045 (Débogueur web intégré)** : `done`. Inspecteur DOM (`web-dom`), style calculé (`web-inspect`), console web (`web-console`).

### Phase 4 - PromptMessage (US-046 à US-060)
- **US-046 (Conception langage PromptMessage)** : `done`. Grammaire fixe (`userspace/promptmessage.c`).
- **US-047 (Compilateur PromptMessage)** : `done`. Bytecode PMC1, checksum FNV-1a.
- **US-048 (Interprète PromptMessage)** : `done`. Machine virtuelle bornée (20000 pas, pile 12).
- **US-049 (Validation syntaxique)** : `done`. Détection ligne/colonne, `pm-check`.
- **US-050 (IDE PromptMessage intégré)** : `done`. Éditeur plein écran (`userspace/osui_ide.c`, `/pmrun`, `pm-ide`), coloration syntaxique, auto-complétion Tab, volet d'exécution (`ESC r`).
- **US-051 (Documentation automatique)** : `done`. Générateur de doc `pm-doc`.
- **US-052 (Débogueur PromptMessage)** : `done`. Tracé et points d'arrêt (`pm-debug`, `pm-disasm`).
- **US-053 (Gestionnaire de bibliothèques)** : `done`. Directive `use` (inclusion de modules).
- **US-054 (Versioning de code)** : `done`. Instantanés `pm-version` / `pm-versions`.
- **US-055 (Compilateur croisé multi-plateforme)** : `done`. Traduction PMC1 vers C99 portable (`pm-cross`).
- **US-056 (Optimiseur de code PromptMessage)** : `done`. Repliement de constantes.
- **US-057 (Système de test automatisé)** : `done`. Directives `expect`, exécution `pm-test`.
- **US-058 (IA d'assistance au développement)** : `done`. Assistant de règles `pm-assist` (correction syntaxique, variables inutilisées).
- **US-059 (Marketplace de PromptPrograms)** : `done`. Catalogue local `/pm` (`pm-catalog`, `pm-install`).
- **US-060 (Certification et sécurité de code)** : `done`. Clés CA Schnorr scellées par AES-128-GCM (`SYS_KEY_SEAL`), signatures `.ca` et liste de révocation CRL (`/etc/pm_ca.crl`).

### Phase 5 - P2P Network (US-061 à US-075)
- **US-061 (Protocole P2P MOHHDY)** : `done`. Handshake, messages typés, session AES-128-GCM.
- **US-062 (Découverte de pairs)** : `done`. HELLO, beaconing local, table de voisins.
- **US-063 (Routage intelligent)** : `done`. Vecteur de distances multi-sauts (jusqu'à 3 sauts), calcul de coût EWMA RTT/perte.
- **US-064 (Consensus distribué Raft)** : `done`. Élection, ré-élection, pre-vote isolé, bail de leader (10s), compaction du journal, InstallSnapshot, 2PC (`p2p_raft_2pc_*`).
- **US-065 (Réplication de données)** : `done`. Store K/V distribué, résolution LWW (Last-Writer-Wins).
- **US-066 (Chiffrement P2P Zero-Trust)** : `done`. X25519 + AES-128-GCM, re-keying dynamique (`p2p_rekey_check_and_rotate`), vérification anti-rejeu.
- **US-067 (Protocole gRPC optimisé)** : `done`. Framing HTTP/2 (HEADERS, DATA, SETTINGS, PING, GOAWAY, RST_STREAM), sous-ensemble HPACK, protobuf, gRPC sur TLS entre invités QEMU (`grpc-tls-serve`, `grpc-tls-call`).
- **US-068 (Gestionnaire de bande passante & QoS)** : `partial`. File de lissage (shaping queue), fenêtres AIMD, contrôle de débit par pair. Manque : test QEMU dynamique d'adaptation sous charge extrême.
- **US-069 (Tolérance aux pannes P2P)** : `partial`. Détection de crash, ré-attribution de slot, re-jointure et rattrapage du store. Manque : test QEMU automatisé multi-invités de crash/rejointe en continu.
- **US-070 (Cache distribué P2P)** : `partial`. Lignes de cache invalidées via Raft avec clôture et accusé de réception (`P2P_I_CACHE_INV`). Manque : invalidation P2P distribuée automatique sous déconnexion/reconnexion multi-nœuds.
- **US-071 (Synchronisation P2P)** : `done`. Anti-entropie Merkle tree, devsync (`devsync-sync`).
- **US-072 (Moniteur de santé réseau)** : `done`. RTT EWMA, pings de garde, métriques `p2p-health`.
- **US-073 (QoS adaptative)** : `partial`. Contrôleur adaptatif basé sur RTT/perte.
- **US-074 (NAT traversal / STUN)** : `done` / `simule`. Codec STUN RFC 5389 (`userspace/stun.c`), classification `direct`/`endpoint_independent`. NAT simulé sur le bus unitaire (pas de NAT matériel sur le hub QEMU).
- **US-075 (Analyse de trafic intelligent)** : `partial`. Ligne de base EWMA par type de message, détection d'anomalie attribuée au pair source. Manque : validation QEMU multi-noeuds avec rapport d'analyse automatique.

### Phase 6 - Multi-Platform (US-076 à US-090)
- **US-076 (Portage ARM)** : `partial`. Noyau ARMv7-A (`arch/arm`) avec MMU, GIC, timer, SVC syscalls dans QEMU `qemu-system-arm`. Non porté : drivers/shell/OS-UI.
- **US-077 (Interface mobile native)** : `done`. `mobile-ui` rendu dans le bureau OS-UI VBE (disposition téléphone/tablette, cibles >= 48px). Saisie tactile simulée.
- **US-078 (Gestionnaire de capteurs)** : `simule`. `sensor-read` indique explicitement `simule` (absence de matériel capteur dans QEMU).
- **US-079 (Gestes tactiles)** : `done`. Classifieurs de gestes simple/multi-touch (`gesture`, `gesture-multi`).
- **US-080 (Adaptation d'écran automatique)** : `done`. Redimensionnement VBE dynamique (`screen-adapt`, `screen-resize`), ré-agencement des grilles.
- **US-081 (Gestionnaire d'énergie)** : `partial`. Profils d'énergie (`power-profile`, `power-status`). Pas de batterie matérielle dans QEMU.
- **US-082 (Notifications push distribuées)** : `done`. File de notifications avec TTL (`notify-push`, `notify-ack`).
- **US-083 (Synchronisation multi-appareils)** : `done`. `devsync-pull`/`devsync-sync` sur canal scellé P2P.
- **US-084 (Continuité cross-platform)** : `done`. Export/import de session de continuité (`continuity-export`/`import`).
- **US-085 (Gestionnaire de périphériques universels)** : `done`. Registre de périphériques, règles de hotplug (`dev-list`, `dev-rules`).
- **US-086 (Émulation legacy DOS)** : `done`. Interprète .COM 8086 réel (`dosemu.c`, `legacy-emul`).
- **US-087 (Compatibilité applicative)** : `done`. Contrôle de format et headers ELF32 (`compat-check`).
- **US-088 (Migration de données)** : `done`. Export/import d'archives compressées/vérifiées (`migrate-export`/`import`).
- **US-089 (Administration unifiée)** : `done`. Vues d'administration multi-nœuds (`admin-all`, `admin-status`).
- **US-090 (Déploiement automatisé)** : `done`. Pipeline de déploiement et rollback local (`deploy-make`, `deploy-verify`).

### Phase 7 - Collaborative (US-091 à US-105)
- **US-091 (Système de points MOHHDY)** : `done`. Registre de points (`collab-pay`).
- **US-092 (Gestionnaire de ressources partagées)** : `done`. Partage de ressources (`collab-tasks`).
- **US-093 (Authentification distribuée)** : `done`. Signatures Schnorr par nœud et clés d'identité (`id-key`, `collab-keys`).
- **US-094 (Marketplace de ressources)** : `done`. Offres, enchères et réservations (`collab-market-*`, `collab-offer`).
- **US-095 (Réputation utilisateur)** : `done`. Score de réputation (`collab-rate`).
- **US-096 (Tâches distribuées)** : `done`. Tâches collaboratives avec séquestre.
- **US-097 (Paiement décentralisé)** : `simule`. Conversion de points (`collab-pay-convert`). Pas de vraie monnaie ni fiat/crypto externe.
- **US-098 (Audit transparent)** : `done`. Chaîne d'audit distribuée (`collab-audit`).
- **US-099 (Contrats intelligents)** : `done`. Langage de contrat MCL (`collab-contract-deploy`, `collab-signoff`).
- **US-100 (Gouvernance communautaire)** : `done`. Votes de gouvernance (`collab-vote`).
- **US-101 (Résolution de conflits)** : `done`. Arbitrage de litiges (`collab-dispute-*`).
- **US-102 (Gestionnaire de données personnelles)** : `done`. Support tickets et gestion de profil (`collab-ticket-*`, `collab-profile`).
- **US-103 (Confidentialité avancée / ZK)** : `done`. Preuve Schnorr NIZK en groupe 2048/256 (`collab-zk-strong`).
- **US-104 (Conformité réglementaire / RGPD)** : `done`. Auto-contrôles de conformité RGPD (`collab-compliance`).
- **US-105 (Support communautaire & Oubli)** : `done`. Droit à l'oubli et purge de profil (`profile-forget`).

### Phase 8 - Production (US-106 à US-120)
- **US-106 (Optimisation performances)** : `done`. Tuning automatique (`prod-perf-tune`).
- **US-107 (Monitoring en production)** : `done`. Moniteur de performance (`prod-perf-monitor`, `prod-metrics`).
- **US-108 (Déploiement continu CI/CD)** : `done`. Moteur CI/CD local (`prod-cicd`, `prod-pipeline`).
- **US-109 (Rollback automatique & Backups)** : `done`. Sauvegardes et restauration automatique (`prod-backups`).
- **US-110 (Autoscaling automatique)** : `done`. Redimensionnement automatique de tâches réelles (`prod-scale-run`, `prod-autoscale-workload`).
- **US-111 (Analyse de logs)** : `done`. Analyseur de logs (`prod-log-analyze`).
- **US-112 (Sécurité en production)** : `done`. Base de signatures de vulnérabilités hors ligne (`prod-sec-db`, `prod-secdb`).
- **US-113 (Alertes intelligentes)** : `done`. Système d'alertes à dérivée seconde (`prod-alerts`).
- **US-114 (Seuils d'apprentissage)** : `done`. Seuils d'alertes appris (`prod-alert-learn`).
- **US-115 (Maintenance prédictive)** : `done`. Modèle de régression logistique Q12 appris (`prod-maint-train`, `prod-maint-predict`).
- **US-116 (Support technique & Bundles)** : `done`. Paquets de support vérifiés par checksum FNV-1a (`prod-support-bundle`).
- **US-117 (Formation & Intégrité)** : `done`. Vérification d'intégrité globale (`prod-integrity`).
- **US-118 (Feedback & Diagnostics)** : `done`. Diagnostics système autonomes (`prod-diag`).
- **US-119 (Métriques business)** : `done`. Calculateur de métriques business (`prod-biz-metrics`).
- **US-120 (Roadmap évolutive)** : `partial`. Lecture de `/roadmap.txt` par `prod-roadmap`. `make roadmap-check` vérifie la phase 9. Manque : extension de `make roadmap-check` aux phases 6 à 8.

### Phase 9 - GUI Parity (US-G01 à US-G18)
- **US-G01 à US-G18** : `done` (18/18). Intégration complète des familles de commandes terminales dans le bureau VBE VFS, visionneuse d'images, édition 64 KiB, configuration d'interface `net-if`, etc.

---

## 2. Planification des 4 PRs Empilées

### PR A : Réseau et dettes
1. **Changement d'adresse IP (`net-if`) & connexions TCP** :
   - Définir et implémenter le comportement lors d'un changement d'adresse réseau (`net-if static` ou `net-if dhcp`) : fermeture propre (`close_notify` / RST) de toutes les connexions TCP actives pour éviter les sockets orphelines.
   - Écrire un test QEMU validant qu'une connexion TCP ouverte est fermée proprement ou ré-établie lors d'un changement d'interface.
2. **Réduction de la durée du handshake TLS** :
   - Chronométrer la durée de la poignée de main TLS (RSA 2048 / X25519 / AES-GCM).
   - Optimiser les calculs `bigint_modexp` (ex. fenêtrage de montgomery/exponentiation rapide) sans réduire la taille de clé ni affaiblir la cryptographie.
   - Consigner les métriques avant/après.
3. **Phase 5 (Tolérance aux pannes, Cache, QoS, Analyse de trafic)** :
   - Renforcer la tolérance aux pannes (US-069), le cache distribué avec invalidation automatique (US-070), la QoS adaptative avec façonnage de débit (US-068/073) et l'analyse de trafic (US-075).
   - Créer/enrichir un test QEMU multi-invités prouvant ces 4 fonctionnalités en réseau de laboratoire.

### PR B : Restes Phase 4 et Phase 3
1. **Complétion Phase 4 (PromptMessage)** :
   - Vérifier et compléter les capacités de l'IDE, du compilateur croisé et du gestionnaire de bibliothèques.
   - Ajouter/renforcer des tests unitaires et d'intégration QEMU pour PromptMessage.
2. **Complétion Phase 3 (Web Runtime)** :
   - Améliorer l'intégration du moteur web (`userspace/webeng.c`), la gestion du stockage local et des médias.
   - Valider la non-régression de `phase3_complete=false` (absence d'imposture Chromium/WebKit) tout en maximisant l'exécution freestanding.

### PR C : Phases 6 à 8 hors ligne & US-120
1. **Audit & finalisation Phases 6 à 8** :
   - Implémenter toutes les extensions réalisables hors ligne pour US-076..US-120.
   - Documenter de façon transparente ce qui reste `simule` (matériel capteur US-078, paiement réel US-097) ou `non prouvable`.
2. **US-120 & Extension de `make roadmap-check`** :
   - Étendre `make roadmap-check` (`scripts/check_roadmap.py`) pour valider la totalité de `/roadmap.txt` (Phases 6 à 8 et Phase 9) par rapport à `US/mohhdy_user_stories_master.md` et aux tests QEMU.

### PR D : Qualité, Couverture Réelle & Image JPEG Progressive
1. **Mesure réelle de couverture de code** :
   - Corriger la génération gcov/lcov pour produire une mesure exacte dans `make coverage` et la CI, en remplaçant la mention "Coverage: not measured".
2. **Image JPEG progressive** :
   - Ajouter le support du JPEG progressif (SOF2) dans `userspace/imgdec.c` si le budget mémoire/calcul freestanding le permet, sinon documenter la limite de manière rigoureuse dans `docs/ETAT_REEL.md`.
3. **Mise à jour documentaire globale & validation** :
   - Mettre à jour `docs/ETAT_REEL.md`, `docs/mohhdy_execution_and_roadmap.md` et `US/mohhdy_user_stories_master.md`.
   - Exécuter toutes les portes de contrôle (`make test-all`, `make strict-image-check`, `make secrets-check`, `make tree-clean-check`, `make roadmap-check`, `python3 scripts/extract_guest_commands.py --check`).
