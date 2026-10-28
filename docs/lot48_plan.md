# Plan Lot 48 Mohhdy OS - Inventaire et Feuille de Route PR A a D

**Date :** October 2026
**Base :** origin/main (post PR #204 / Lot 47)

---

## 1. Inventaire Reel des User Stories (Phases 3 a 8 + Phase 9)

Status of this inventory (Lot 48 review): the first version marked as `done` ten US that the US master or `roadmap.txt` keep `partial` (US-038, US-039, US-055, US-058, US-059, US-060, US-063, US-073, US-074, US-080); they are corrected below with the source. Phases 6-8: `roadmap.txt` is the status source. Nothing in PR A-D was implemented in the first commit except the socket cleanup on an address change, which the review replaced (see section 3).

### Phase 3 - Web Runtime (US-031 a US-045)
- **US-031 (Navigateur-OS / Moteur web)** : `partial`. Moteur minimal `userspace/webeng.c` (HTML tokenizer, cascade CSS, layout bloc/inline, display list VBE, scroll, liens cliquables). Manque : JavaScript dynamic VM, layout flex/grid complet, rendu reseau direct (se fait via `web FILE`). Faisable/prouvable dans QEMU : oui.
- **US-032 (Gestionnaire apps web natives)** : `done`. Multi-onglets, onglets ancres/renommes, isolation par origine (`userspace/osui_runtime.c`), stockage local chiffre (`browser-fs-save`).
- **US-033 (Systeme de fichiers web)** : `done`. Peripherique VFS/Web, inspection VFS dans `/browser` et `/fs`.
- **US-034 (Explorateur de fichiers web)** : `done`. Application graphique explorateur (`userspace/osui_files.c`, `/files`), arborescence VFS, actions CRUD.
- **US-035 (Gestionnaire de medias web)** : `done` / `simule`. Commandes `media-play`, `media-stream` avec ticks dans l'event loop (`userspace/osui_runtime.c`). Flux audio/video simule sans sous-systeme audio materiel QEMU.
- **US-036 (Progressive Web Apps - PWA)** : `done`. Manifestes PWA (`pwa-manifest`, `pwa-install`, `pwa-run`), origines isolees `pwa://`.
- **US-037 (Synchronisation web)** : `done`. Synchronisation VFS/Web local (`browser-fs-sync`).
- **US-038 (Gestionnaire de cache intelligent)** : `partial` (cache LRU real; origin `simule` per the US master, lot 5). Cache HTTP LRU (`web-cache`), en-tetes ETag/Cache-Control.
- **US-039 (Securite web & CSP)** : `partial` (sanitizer in the pipeline; CSP parsed but no external resource to filter, US master lot 7). Assainisseur HTML (`web-sanitize`), parseur et verificateur CSP (`webrt_csp_enforce`, `webrt_csp_validate_display_list_resource`), journal d'audit `osui-audit`.
- **US-040 (Interface responsive universelle)** : `done`. `@media` queries (width/height/aspect-ratio), adaptation aux resolutions VBE.
- **US-041 (Themes adaptatifs & variables CSS)** : `done`. Variables CSS (`var()`), themes clair/sombre (`web-theme`).
- **US-042 (Web Components)** : `done`. Compagnon `<template id=...>` et `<slot>` developpes sans JS framework.
- **US-043 (Gestionnaire de performances web)** : `done`. Mesures de cycles et temps de rendu (`web-perf`, `browser-telemetry perf`).
- **US-044 (Accessibilite web - a11y)** : `done`. Audit d'accessibilite `web-a11y` (balises lang, alt, structure H1..H6, labels).
- **US-045 (Debogueur web integre)** : `done`. Inspecteur DOM (`web-dom`), style calcule (`web-inspect`), console web (`web-console`).

### Phase 4 - PromptMessage (US-046 a US-060)
- **US-046 (Conception langage PromptMessage)** : `done`. Grammaire fixe (`userspace/promptmessage.c`).
- **US-047 (Compilateur PromptMessage)** : `done`. Bytecode PMC1, checksum FNV-1a.
- **US-048 (Interprete PromptMessage)** : `done`. Machine virtuelle bornee (20000 pas, pile 12).
- **US-049 (Validation syntaxique)** : `done`. Detection ligne/colonne, `pm-check`.
- **US-050 (IDE PromptMessage integre)** : `done`. Editeur plein ecran (`userspace/osui_ide.c`, `/pmrun`, `pm-ide`), coloration syntaxique, auto-completion Tab, volet d'execution (`ESC r`).
- **US-051 (Documentation automatique)** : `done`. Generateur de doc `pm-doc`.
- **US-052 (Debogueur PromptMessage)** : `done`. Trace et points d'arret (`pm-debug`, `pm-disasm`).
- **US-053 (Gestionnaire de bibliotheques)** : `done`. Directive `use` (inclusion de modules).
- **US-054 (Versioning de code)** : `done`. Instantanes `pm-version` / `pm-versions`.
- **US-055 (Compilateur croise multi-plateforme)** : `partial` (one C99 target, US master lot 5). Traduction PMC1 vers C99 portable (`pm-cross`).
- **US-056 (Optimiseur de code PromptMessage)** : `done`. Repliement de constantes.
- **US-057 (Systeme de test automatise)** : `done`. Directives `expect`, execution `pm-test`.
- **US-058 (IA d'assistance au developpement)** : `partial` (rule-based, no model, US master lot 5). Assistant de regles `pm-assist` (correction syntaxique, variables inutilisees).
- **US-059 (Marketplace de PromptPrograms)** : `partial` (local catalog only, US master lot 6). Catalogue local `/pm` (`pm-catalog`, `pm-install`).
- **US-060 (Certification et securite de code)** : `partial` (sealing key per boot, no TPM: `simule` part, US master lot 6). Cles CA Schnorr scellees par AES-128-GCM (`SYS_KEY_SEAL`), signatures `.ca` et liste de revocation CRL (`/etc/pm_ca.crl`).

### Phase 5 - P2P Network (US-061 a US-075)
- **US-061 (Protocole P2P MOHHDY)** : `done`. Handshake, messages types, session AES-128-GCM.
- **US-062 (Decouverte de pairs)** : `done`. HELLO, beaconing local, table de voisins.
- **US-063 (Routage intelligent)** : `partial` (lab network, US master lot 9). Vecteur de distances multi-sauts (jusqu'a 3 sauts), calcul de cout EWMA RTT/perte.
- **US-064 (Consensus distribue Raft)** : `done`. Election, re-election, pre-vote isole, bail de leader (10s), compaction du journal, InstallSnapshot, 2PC (`p2p_raft_2pc_*`).
- **US-065 (Replication de donnees)** : `done`. Store K/V distribue, resolution LWW (Last-Writer-Wins).
- **US-066 (Chiffrement P2P Zero-Trust)** : `done`. X25519 + AES-128-GCM, re-keying dynamique (`p2p_rekey_check_and_rotate`), verification anti-rejeu.
- **US-067 (Protocole gRPC optimise)** : `done`. Framing HTTP/2 (HEADERS, DATA, SETTINGS, PING, GOAWAY, RST_STREAM), sous-ensemble HPACK, protobuf, gRPC sur TLS entre invites QEMU (`grpc-tls-serve`, `grpc-tls-call`).
- **US-068 (Gestionnaire de bande passante & QoS)** : `partial`. File de lissage (shaping queue), fenetres AIMD, controle de debit par pair. Manque : test QEMU dynamique d'adaptation sous charge extreme.
- **US-069 (Tolerance aux pannes P2P)** : `done` (lot 48, reseau de laboratoire; was `partial`). Detection de crash, re-attribution de slot, re-jointure et rattrapage du store. Manque : test QEMU automatise multi-invites de crash/rejointe en continu.
- **US-070 (Cache distribue P2P)** : `done` (lot 48; was `partial`). Lignes de cache invalidees via Raft avec cloture et accuse de reception (`P2P_I_CACHE_INV`). Manque : invalidation P2P distribuee automatique sous deconnexion/reconnexion multi-nuds.
- **US-071 (Synchronisation P2P)** : `done`. Anti-entropie Merkle tree, devsync (`devsync-sync`).
- **US-072 (Moniteur de sante reseau)** : `done`. RTT EWMA, pings de garde, metriques `p2p-health`.
- **US-073 (QoS adaptative)** : `partial` (US master lot 9). Controleur adaptatif base sur RTT/perte.
- **US-074 (NAT traversal / STUN)** : `partial` (NAT simulated on the unit bus, punching simulated) Lot 48 review: / `simule`. Codec STUN RFC 5389 (`userspace/stun.c`), classification `direct`/`endpoint_independent`. NAT simule sur le bus unitaire (pas de NAT materiel sur le hub QEMU).
- **US-075 (Analyse de trafic intelligent)** : `done` (lot 48, statistique, pas d'apprentissage; was `partial`). Ligne de base EWMA par type de message, detection d'anomalie attribuee au pair source. Manque : validation QEMU multi-noeuds avec rapport d'analyse automatique.

### Phase 6 - Multi-Platform (US-076 a US-090)
- **US-076 (Portage ARM)** : `partial`. Noyau ARMv7-A (`arch/arm`) avec MMU, GIC, timer, SVC syscalls dans QEMU `qemu-system-arm`. Non porte : drivers/shell/OS-UI.
- **US-077 (Interface mobile native)** : `done`. `mobile-ui` rendu dans le bureau OS-UI VBE (disposition telephone/tablette, cibles >= 48px). Saisie tactile simulee.
- **US-078 (Gestionnaire de capteurs)** : `simule`. `sensor-read` indique explicitement `simule` (absence de materiel capteur dans QEMU).
- **US-079 (Gestes tactiles)** : `done`. Classifieurs de gestes simple/multi-touch (`gesture`, `gesture-multi`).
- **US-080 (Adaptation d'ecran automatique)** : `partial` (roadmap.txt: layout-computed; was wrongly `done` here). Redimensionnement VBE dynamique (`screen-adapt`, `screen-resize`), re-agencement des grilles.
- **US-081 (Gestionnaire d'energie)** : `partial`. Profils d'energie (`power-profile`, `power-status`). Pas de batterie materielle dans QEMU.
- **US-082 (Notifications push distribuees)** : `done`. File de notifications avec TTL (`notify-push`, `notify-ack`).
- **US-083 (Synchronisation multi-appareils)** : `done`. `devsync-pull`/`devsync-sync` sur canal scelle P2P.
- **US-084 (Continuite cross-platform)** : `done`. Export/import de session de continuite (`continuity-export`/`import`).
- **US-085 (Gestionnaire de peripheriques universels)** : `done`. Registre de peripheriques, regles de hotplug (`dev-list`, `dev-rules`).
- **US-086 (Emulation legacy DOS)** : `done`. Interprete .COM 8086 reel (`dosemu.c`, `legacy-emul`).
- **US-087 (Compatibilite applicative)** : `done`. Controle de format et headers ELF32 (`compat-check`).
- **US-088 (Migration de donnees)** : `done`. Export/import d'archives compressees/verifiees (`migrate-export`/`import`).
- **US-089 (Administration unifiee)** : `done`. Vues d'administration multi-nuds (`admin-all`, `admin-status`).
- **US-090 (Deploiement automatise)** : `done`. Pipeline de deploiement et rollback local (`deploy-make`, `deploy-verify`).

### Phase 7 - Collaborative (US-091 a US-105)
- **US-091 (Systeme de points MOHHDY)** : `done`. Registre de points (`collab-pay`).
- **US-092 (Gestionnaire de ressources partagees)** : `done`. Partage de ressources (`collab-tasks`).
- **US-093 (Authentification distribuee)** : `done`. Signatures Schnorr par nud et cles d'identite (`id-key`, `collab-keys`).
- **US-094 (Marketplace de ressources)** : `done`. Offres, encheres et reservations (`collab-market-*`, `collab-offer`).
- **US-095 (Reputation utilisateur)** : `done`. Score de reputation (`collab-rate`).
- **US-096 (Taches distribuees)** : `done`. Taches collaboratives avec sequestre.
- **US-097 (Paiement decentralise)** : `simule`. Conversion de points (`collab-pay-convert`). Pas de vraie monnaie ni fiat/crypto externe.
- **US-098 (Audit transparent)** : `done`. Chaine d'audit distribuee (`collab-audit`).
- **US-099 (Contrats intelligents)** : `done`. Langage de contrat MCL (`collab-contract-deploy`, `collab-signoff`).
- **US-100 (Gouvernance communautaire)** : `done`. Votes de gouvernance (`collab-vote`).
- **US-101 (Resolution de conflits)** : `done`. Arbitrage de litiges (`collab-dispute-*`).
- **US-102 (Gestionnaire de donnees personnelles)** : `done`. Support tickets et gestion de profil (`collab-ticket-*`, `collab-profile`).
- **US-103 (Confidentialite avancee / ZK)** : `done`. Preuve Schnorr NIZK en groupe 2048/256 (`collab-zk-strong`).
- **US-104 (Conformite reglementaire / RGPD)** : `done`. Auto-controles de conformite RGPD (`collab-compliance`).
- **US-105 (Support communautaire & Oubli)** : `done`. Droit a l'oubli et purge de profil (`profile-forget`).

### Phase 8 - Production (US-106 a US-120)
- **US-106 (Optimisation performances)** : `done`. Tuning automatique (`prod-perf-tune`).
- **US-107 (Monitoring en production)** : `done`. Moniteur de performance (`prod-perf-monitor`, `prod-metrics`).
- **US-108 (Deploiement continu CI/CD)** : `done`. Moteur CI/CD local (`prod-cicd`, `prod-pipeline`).
- **US-109 (Rollback automatique & Backups)** : `done`. Sauvegardes et restauration automatique (`prod-backups`).
- **US-110 (Autoscaling automatique)** : `done`. Redimensionnement automatique de taches reelles (`prod-scale-run`, `prod-autoscale-workload`).
- **US-111 (Analyse de logs)** : `done`. Analyseur de logs (`prod-log-analyze`).
- **US-112 (Securite en production)** : `done`. Base de signatures de vulnerabilites hors ligne (`prod-sec-db`, `prod-secdb`).
- **US-113 (Alertes intelligentes)** : `done`. Systeme d'alertes a derivee seconde (`prod-alerts`).
- **US-114 (Seuils d'apprentissage)** : `done`. Seuils d'alertes appris (`prod-alert-learn`).
- **US-115 (Maintenance predictive)** : `done`. Modele de regression logistique Q12 appris (`prod-maint-train`, `prod-maint-predict`).
- **US-116 (Support technique & Bundles)** : `done`. Paquets de support verifies par checksum FNV-1a (`prod-support-bundle`).
- **US-117 (Formation & Integrite)** : `done`. Verification d'integrite globale (`prod-integrity`).
- **US-118 (Feedback & Diagnostics)** : `done`. Diagnostics systeme autonomes (`prod-diag`).
- **US-119 (Metriques business)** : `done`. Calculateur de metriques business (`prod-biz-metrics`).
- **US-120 (Roadmap evolutive)** : `partial`. Lecture de `/roadmap.txt` par `prod-roadmap`. `make roadmap-check` verifie la phase 9. Manque : extension de `make roadmap-check` aux phases 6 a 8.

### Phase 9 - GUI Parity (US-G01 a US-G18)
- **US-G01 a US-G18** : `done` (18/18). Integration complete des familles de commandes terminales dans le bureau VBE VFS, visionneuse d'images, edition 64 KiB, configuration d'interface `net-if`, etc.

---

## 2. Planification des 4 PRs Empilees

### PR A : Reseau et dettes
1. **Changement d'adresse IP (`net-if`) & connexions TCP** :
   - Definir et implementer le comportement lors d'un changement d'adresse reseau (`net-if static` ou `net-if dhcp`) : fermeture propre (`close_notify` / RST) de toutes les connexions TCP actives pour eviter les sockets orphelines.
   - Ecrire un test QEMU validant qu'une connexion TCP ouverte est fermee proprement ou re-etablie lors d'un changement d'interface.
2. **Reduction de la duree du handshake TLS** :
   - Chronometrer la duree de la poignee de main TLS (RSA 2048 / X25519 / AES-GCM).
   - Optimiser les calculs `bigint_modexp` (ex. fenetrage de montgomery/exponentiation rapide) sans reduire la taille de cle ni affaiblir la cryptographie.
   - Consigner les metriques avant/apres.
3. **Phase 5 (Tolerance aux pannes, Cache, QoS, Analyse de trafic)** :
   - Renforcer la tolerance aux pannes (US-069), le cache distribue avec invalidation automatique (US-070), la QoS adaptative avec faconnage de debit (US-068/073) et l'analyse de trafic (US-075).
   - Creer/enrichir un test QEMU multi-invites prouvant ces 4 fonctionnalites en reseau de laboratoire.

### PR B : Restes Phase 4 et Phase 3
1. **Completion Phase 4 (PromptMessage)** :
   - Verifier et completer les capacites de l'IDE, du compilateur croise et du gestionnaire de bibliotheques.
   - Ajouter/renforcer des tests unitaires et d'integration QEMU pour PromptMessage.
2. **Completion Phase 3 (Web Runtime)** :
   - Ameliorer l'integration du moteur web (`userspace/webeng.c`), la gestion du stockage local et des medias.
   - Valider la non-regression de `phase3_complete=false` (absence d'imposture Chromium/WebKit) tout en maximisant l'execution freestanding.

### PR C : Phases 6 a 8 hors ligne & US-120
1. **Audit & finalisation Phases 6 a 8** :
   - Implementer toutes les extensions realisables hors ligne pour US-076..US-120.
   - Documenter de facon transparente ce qui reste `simule` (materiel capteur US-078, paiement reel US-097) ou `non prouvable`.
2. **US-120 & Extension de `make roadmap-check`** :
   - Etendre `make roadmap-check` (`scripts/check_roadmap.py`) pour valider la totalite de `/roadmap.txt` (Phases 6 a 8 et Phase 9) par rapport a `US/mohhdy_user_stories_master.md` et aux tests QEMU.

### PR D : Qualite, Couverture Reelle & Image JPEG Progressive
1. **Mesure reelle de couverture de code** :
   - Corriger la generation gcov/lcov pour produire une mesure exacte dans `make coverage` et la CI, en remplacant la mention "Coverage: not measured".
2. **Image JPEG progressive** :
   - Ajouter le support du JPEG progressif (SOF2) dans `userspace/imgdec.c` si le budget memoire/calcul freestanding le permet, sinon documenter la limite de maniere rigoureuse dans `docs/ETAT_REEL.md`.
3. **Mise a jour documentaire globale & validation** :
   - Mettre a jour `docs/ETAT_REEL.md`, `docs/mohhdy_execution_and_roadmap.md` et `US/mohhdy_user_stories_master.md`.
   - Executer toutes les portes de controle (`make test-all`, `make strict-image-check`, `make secrets-check`, `make tree-clean-check`, `make roadmap-check`, `python3 scripts/extract_guest_commands.py --check`).


## 3. Lot 48 review (first fix-forward on this branch)

- PR A item 1 (TCP across a `net-if` address change): done. The first
  commit freed the sockets silently and set `g_web_open = 0`, which stopped
  a running background `web-serve` without telling the shell, and sent
  nothing to the peer. Now `ifc_drop_connections` (kernel/net_llm_client.c,
  Ring 3 networker) sends FIN+ACK from the old address on each open
  connection, frees it, keeps the listeners (the web table is re-armed) and
  does nothing when the address does not change. `net-if` prints
  `closed N`. Proof: `test_net_ifcfg` (test_closed_connections_reported)
  and `make qemu-osui-web` step 6: an open connection with a partial
  request receives FIN from 10.32.0.15, `closed 1`, and the same
  background server answers 200 on 10.32.0.77.
- `scripts/check_roadmap.py`: the phase 6-8 part checks that US-076..US-120
  are all present and in the right phase. It does not compare their
  statuses: the US master has no status table for phases 6-8. US-120 stays
  partial.
- Not done in this PR: PR A items 2 (TLS timing) and 3 (phase 5 QEMU
  tests), PR B, PR C (apart from the presence check), PR D.

## 4. Lot 48 delivered (merged with PR #205, head 7593af4)

- Net-if address change: TCP connections on the old address get FIN from it
  (`closed N`), a background web-serve is re-armed, the LLM TLS session gets
  close_notify + FIN (`llm-closed 1`). QEMU: qemu-osui-web step 6,
  qemu-ne2k-tls-http net-if step. Jules commit c6568c5 had reverted this and
  deleted its tests; it was merged with our side kept (4b9943b).
- A2 TLS handshake, measured in the guest with `ai-tls-poll ticks N`
  (10 ms ticks): before 198 ticks (1.98 s; X25519 step 160, RSA steps 18
  and 17), after about 11 ticks (0.11 s, no step above 3). Root cause:
  every modular product was 32 * limbs double-and-add modular additions.
  Fix: X25519 field arithmetic mod 2^255-19 (8x8 limb product, fold by 38,
  constant time, same RFC 7748 ladder) and Montgomery modexp for odd moduli
  (masked final subtraction). Crypto unchanged. The wall time of the test
  (20 s) is the harness typing keys, not guest work. Guard: the QEMU test
  fails above 60 ticks total or 30 per step; unit tests: RFC 7748 iterated
  1 and 1000 rounds, 48 differential cases against the old code, Montgomery
  vectors 32..4096 bit against Python pow().
- A3 phase 5 (make qemu-p2p-phase5, CI job, about 4 min, three guests):
  US-070 fence pushed on reconnect (a cut peer that missed an invalidation
  refuses the stale fill); US-068/073 adaptive window backs off under a hub
  delay (16 -> 10, burst shaped without p2p-limit, 12/12 delivered), a 4 s
  drop counts a ping loss, the window grows back to 16; US-075 automatic
  `p2p traffic alert peer X` lines for two flooders, none on quiet nodes;
  US-069 three crash/reboot cycles, each with a key put during the crash.
- US-068/073 stay `partial`: adaptation proven under delay and drop on the
  lab hub, not under extreme load or on a WAN.
- Not done in lot 48: PR B (phases 3-4), PR C status table (done in lot 49),
  PR D measured coverage and progressive JPEG. Lot 49 takes those
  (`docs/lot49_plan.md`): flex/grid and a Python cross target stay partial,
  SOF2 decodes, the coverage figure is quoted from `make coverage`.
  US-059, US-060, US-063 and US-074 are not advanced there.
  Lot 50 (`docs/lot50_plan.md`) does not reopen this plan: it only adds
  desktop menus for commands the shell already has.
  Lot 51 (`docs/lot51_plan.md`) does not reopen it either: `/child`,
  `/release` and `/keys` are the same kind of menu.
