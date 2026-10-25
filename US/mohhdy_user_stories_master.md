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

Etat US-001 (17 octobre 2026, lot 8). Ferme sur l'image par defaut (decision argumentee, a valider par le proprietaire). Le dernier pilote de peripherique a chemin de donnees encore en Ring 0, la tablette USB UHCI, tourne en Ring 3 (`bin/usbdriver`, service `usb-driver` epingle a ce binaire). DMA borne sans IOMMU : le noyau possede la liste de trames et la page d'ordonnancement (QH + 8 TD), ecrit chaque TD apres verification (tampon dans la page de donnees du pilote, liens dans la page d'ordonnancement, pas de cycle), USBCMD et FLBASEADD restent hors du bitmap d'E/S du pilote ; a la mort du pilote le noyau arrete le controleur puis le superviseur relance le pilote (budget 4). Plus aucun sondage Ring 0 de la tablette. Restent noyau, par conception : PIC, PIT, entrees d'IRQ, console du debut de boot, mode panique, sondes PCI de debut de boot (UHCI, NE2000). L'image legacy de comparaison garde le NIC Ring 0 (non livree). Preuve : `make qemu-usb-driver` (sonde de bornes refusee, rapports QMP, devrogue -180, kill : arret puis relance), `make qemu-osui-gui` (clics tablette via le pilote Ring 3), `test_usb_dma`, `test_device_service`.

Etat US-001 (17 octobre 2026, lot 7). Ouvert, plus etroit, decision de non-fermeture documentee. La console texte VGA est peinte par le pilote Ring 3 `bin/vgadriver` (service `vga-driver`) : page texte 0xB8000 mappee en ecriture dans sa seule tache, ports CRTC 0x3D4/0x3D5 dans son bitmap d'E/S TSS, trames prises par `OS_DEV_OP_VGA_TAKE`. Repli Ring 0 honnete : debut du boot (avant la reclamation), apres la mort du pilote jusqu'a la reclamation du pilote relance, blocage de 5 ticks, faute noyau ; trames comptees. Le modele de console (lignes, historique, curseur) reste noyau. Restent Ring 0 : pilote UHCI de la tablette USB (DMA, non deplace), chemin NE2000 de l'image legacy de comparaison. Noyau irreductible assume : PIC, PIT, entrees d'IRQ, peinture du debut de boot, mode panique. Preuve : `make qemu-input-drivers` (etape 8, memoire video relue par `pmemsave`), `test_device_service`, `test_io_bitmap`, `test_vga_console`.

Etat US-001 (17 octobre 2026, lot 6). Ouvert, plus etroit. Souris PS/2 prouvee en QEMU (mouvement relatif et bouton QMP, paquets via `bin/ps2driver`, sommes verifiees dans `devices`). Desynchronisation de Shift a la mort du pilote corrigee : le pilote publie Shift/Caps (evenements `OS_INPUT_EV_MODS`), le noyau les garde a la mort du pilote et le pilote relance repart de cet etat ; QEMU : Shift tenu pendant le `kill` du pilote donne `B`, puis `b` apres relache. Restent Ring 0 : PIC, PIT, VGA texte, sondes de debut de boot. Preuve : `make qemu-input-drivers`, `test_ps2_decode`.

Etat US-001 (17 octobre 2026, lot 5). Ouvert, plus etroit. Le controleur PS/2 (clavier/souris) et COM1 sont pilotes par des pilotes Ring 3 supervises : `bin/ps2driver` (service `ps2-driver`) et `bin/serialdriver` (service `serial-driver`), lies a leurs binaires, lances au boot en priorite HIGH, avec seulement les ports 0x60/0x64 et 0x3F8-0x3FF dans leur bitmap d'E/S TSS apres la reclamation `SYS_DEVICE_SERVICE`. Repli Ring 0 honnete : avant la reclamation (debut du boot), apres la mort d'un pilote jusqu'a la reclamation du pilote relance, pilote bloque (5 ticks), file pleine, et fautes CPU (mode panique). Souris : decodeur prouve par tests unitaires seulement. Preuve : `make qemu-input-drivers`, `test_device_service`, `test_ps2_decode`, `test_io_bitmap`.

Etat US-001 (17 octobre 2026, lot 4). Ouvert, plus etroit. L'enumeration PCI est un worker Ring 3 : `bin/pcidriver` (service `pci-driver`, lie a ce binaire) est lance au boot, n'a que les ports 0xCF8-0xCFF dans son bitmap d'E/S TSS, enumere les bus 0-7 et publie l'inventaire (`SYS_PCI_SERVICE` 157). Apres la remise, les lectures de configuration Ring 0 sont refusees et comptees (`ring0denied`) et le noyau lit les BAR (framebuffer VGA) dans l'inventaire publie. Supervision paresseuse : un publieur mort est detecte a la requete suivante et relance (budget 4). `lspci`, `pci-rescan` (IPC). Preuves : `test_pci_service`, `test_io_bitmap`, `make qemu-pci-driver` (CI). Restent Ring 0 : sondes de debut de boot (tablette USB, NE2000 sur l'image legacy), PIC, PIT, PS/2, VGA texte, serie.

Etat US-001 (17 octobre 2026, lot 3). Ouvert, plus etroit. Les journaux de montage (LBA 4222-4223) et d'evenements (LBA 4224) sont lus par `bin/atadriver` Ring 3 et remis au noyau (`OS_ATA_FS_INSTALL` types 4 et 5, une fois par boot). `ata-status` imprime `jkread 0 jinst 2`. Sans disque IDE, aucun acces port pour les journaux. L'image legacy lie `boot_storage_strict.o` : montage FAT de boot, overlay et journaux viennent de Ring 3 sur les deux images. L'image legacy garde le NIC Ring 0 : c'est le noyau de comparaison de `NET_RING0_FALLBACK`, pas une image livree. Restent Ring 0 : PIC, PIT, PS/2, VGA, serie, PCI, `usb_tablet`. Preuve : `make strict-image-check`, `make qemu-ata-driver`, `make qemu-foundation-steps`.

Etat US-001 (15 octobre 2026, lot 2). Ouvert. Les deux images lient `ata_identify_absent.c` et `fat_pio_fallback_absent.c` : `kernel_ring0_ata_identify` et `kernel_ring0_fat_pio_fallback` sont absents de `build/mohhdy.bin` et de `build/mohhdy-netlegacy.bin`. L'image legacy garde le NIC Ring 0 et `kernel_ring0_boot_fat_mount`. `bin/atadriver` envoie IDENTIFY. Apres sa mort le noyau rearme le canal s'il est BSY ou DRQ, gare le store, relance `bin/atadriver` (budget 4) et rend le budget quand ce pid publie le store. `fatkpio` reste 0. Le journal de regime permanent passe par le RPC du pilote quand l'appelant peut attendre. La lecture du journal au boot, avant tout pilote, et le chargement overlay au boot restent Ring 0. Preuve : `make strict-image-check`, `make qemu-ata-driver`. Restent Ring 0 : cette lecture de boot, ce chargement overlay, PIC, PIT, PS/2, VGA, serie, PCI, `usb_tablet`, et le montage FAT de boot de l'image legacy.

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

Etat Phase 2 (17 octobre 2026, lot 9; inventaire dans `docs/lot9_plan.md`). US-016 a US-030 restent ouverts : specifications seulement (TensorFlow Lite, NLU a 90 %, modeles distribues, apprentissage federe, cloud-edge, services IA au-dela de l'assistant GPT-2 local). Non faisables hors ligne dans ce lot.

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

Etat Phase 3 (17 octobre 2026, lot 7). US-039 : l'assainisseur tourne en tete du pipeline du moteur web (`sanitized N`, page hostile prouvee en QEMU) ; CSP analysee mais sans ressource externe a filtrer : reste partiel. US-038 inchange (origine `simule`). `phase3_complete` reste false.

Etat Phase 3 (17 octobre 2026, lot 6; detail dans `docs/ETAT_REEL.md`). US-031 livre comme moteur web minimal en C guest (`userspace/webeng.c` : analyse HTML, cascade CSS, mise en page bloc/ligne, liste d'affichage peinte dans le volet OS-UI `OS_FB_PANE_WEB`), sans JavaScript, sans images, sans flottants ni flex, police fixe 8x10, fichiers locaux seulement. US-043 livre (minimal) : `web-perf`. US-045 livre (minimal) : `web-dom`, `web-inspect`, `web-console`. US-040, US-041, US-042, US-044 appliques dans le pipeline du moteur (composants, `@media` pour 640x480, `var()`, audit a11y dans `web-console`). US-038 (origine `simule`) et US-039 (assainisseur hors pipeline) restent partiels. `phase3_complete` reste false (pas de chargement reseau dans le moteur, pas de script).

Etat Phase 3 (17 octobre 2026, lot 5; detail dans `docs/ETAT_REEL.md`). Partiels : US-038 (cache HTTP, origine `simule`), US-039 (CSP, assainisseur HTML), US-040 (media queries), US-041 (proprietes CSS personnalisees, themes), US-042 (composants `<template>`/`<slot>`, sans script), US-044 (audit d'accessibilite), dans `userspace/webrt.c` et les commandes OS-UI `web-*`. Non livres : US-031 (pas de moteur web), US-043, US-045. `phase3_complete` reste false.

Etat Phase 3 (14 octobre 2026; detail in `docs/ETAT_REEL.md`). Pull request #147 does not deliver US-031. `phase3_complete` stays false. Chromium and WebKit are not present.
- `browser-html-render` is a text-mode printer in `userspace/osui_runtime.c`. It parses a flat list of at most 32 tags (name, id, class, text) and prints headings, lists, links, buttons, inputs, and other text. `osui_css_apply_styles` is not a selector engine: the CSS text is ignored. h1 and b get font-weight 700, links are marked blue, buttons get a gray background and padding 4, and a node whose id is `app` gets margin 8 and padding 8 when the CSS pointer is non-NULL. No parent links, no layout boxes. Proof: `test_html_css_rendering`.
- `bigint_modexp` skips leading zero bits of the highest limb. That is the shared modular exponentiation primitive, not an RSA or TLS handshake change by itself. Proof: `test_bigint_modexp_leading_zeros`.
- The worker wire table in `kernel/net_wire.c` accepts a full wildcard listen and prefers a specific 4-tuple. The OS-UI `web-serve` pump in `kernel/net_llm_client.c` is a separate table and still drops other destination ports.
- Host unit-test coverage, measured 15 October 2026 by `make coverage`: 79.55% lines executed (24434/30716) across 143 source files, 572 `.gcda`. Flags: `--coverage`. Not guest-image coverage. Not part of `make test-all` or CI.

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

Etat Phase 5 (17 octobre 2026, lot 9). US-067 ferme : appels gRPC (trames HTTP/2, sous-ensemble HPACK, protobuf) transportes dans les enregistrements applicatifs de la session TLS 1.2 entre deux invites (`grpc-tls-serve`, `grpc-tls-call`, AES-128-GCM), aucun gRPC en clair sur le fil ; preuve `make qemu-net-peer-tls-worker`. HPACK reste un sous-ensemble. US-063, US-068/US-073, US-069, US-070, US-074, US-075 restent partiels (reseau de laboratoire).

Etat Phase 4 et 5 (17 octobre 2026, lot 6; detail dans `docs/ETAT_REEL.md`). US-059/US-060 : la cle privee de la CA n'est plus stockee en clair, elle est scellee par le noyau (`SYS_KEY_SEAL`, AES-128-GCM lie au nom de la tache) ; simule : cle de scellement par boot (pas de TPM), la cle scellee ne survit pas au redemarrage ; CA et catalogue restent locaux, partiels. US-067 : appel gRPC unaire sur une vraie connexion TCP entre deux invites (`grpc-serve`, `grpc-call`, h2c en clair, un appel par connexion, sans controle de flux), `make qemu-grpc-guests` ; partiel au sens gRPC complet. US-068/US-073 : file de lissage au lieu de rejet, debits mesures (QEMU scenario 1b). US-070 : lignes de cache via le journal Raft (QEMU scenario 2b). US-075 : base par pair, anomalie attribuee a sa source, rafales sollicitees ignorees (QEMU scenario 1c) ; statistique, pas d'apprentissage. Ces trois restent partiels (reseau de laboratoire, pas de WAN).

Etat Phase 4 et 5 (17 octobre 2026, lot 5; detail dans `docs/ETAT_REEL.md`). US-055 partiel : `pm-cross` traduit le bytecode PMC1 en un programme C99 (une seule cible), compile et compare par le compilateur hote. US-058 partiel : `pm-assist` a regles, sans modele. US-059/US-060 partiels : liste de revocation locale `/etc/pm_ca.crl` (`pm-ca-revoke`), verification CA a la recuperation du catalogue local. US-067 partiel : appel gRPC unaire encode en trames HTTP/2 (sous-ensemble HPACK, protobuf) dans `userspace/grpcwire.c`, commande `grpc-wire` en memoire, pas de connexion TCP HTTP/2. US-068/US-073, US-070, US-075 inchanges dans ce lot.

Etat Phase 5 (17 octobre 2026, lot 7). US-067 : moteur de connexion HTTP/2 (plusieurs appels par connexion, flux impairs, fenetres de controle de flux connexion et flux, WINDOW_UPDATE, SETTINGS, PING, GOAWAY, RST_STREAM) ; `grpc-calls` enchaine 3 appels sur une connexion TCP avec une fenetre de 16 octets entre deux invites QEMU (`make qemu-grpc-guests`). Reste partiel : h2c en clair, HPACK partiel, pas de TLS.

Etat Phase 5 (17 octobre 2026, lot 4; detail dans `docs/ETAT_REEL.md` et `docs/p2p.md`). US-063 partiel, prouve en QEMU avec deux relais (`make qemu-p2p-mesh`, 4 invites, ligne alpha-beta-gamma-delta). US-069 partiel : crash d'un relais (processus QEMU tue), reroutage, redemarrage sous une nouvelle identite, reprise du slot (`rejoins`) et store repousse, en QEMU. US-074 partiel (n'est plus seulement simule) : vrai codec STUN RFC 5389 (Binding, XOR-MAPPED-ADDRESS, vecteur RFC 5769) dans le lien chiffre, classification `direct` / `endpoint_independent` / `endpoint_dependent` ; QEMU prouve `direct` entre invites ; le NAT reste simule (reecriture d'adresse sur le bus des tests unitaires) et le percage (`p2p-punch`) reste simule. Pas de NAT reel.

Etat Phase 5 (17 octobre 2026, lot 3; detail dans `docs/ETAT_REEL.md` et `docs/p2p.md`). Toujours partiels, plus avances : US-063 vecteur de distances, relais saut par saut jusqu'a 3 sauts (unitaire sur 4 noeuds; QEMU 3 invites = un relais). US-068/US-073 controleur adaptatif (RTT lisse, RTT minimum, coupe d'un quart sur delai, moitie sur perte), prouve en QEMU scenario 3. US-070 cache versionne, barrieres anti-ecriture perimee, invalidation acquittee et re-emise, QEMU scenario 4. US-075 base apprise par EWMA par type (pas de ML), anomalie classee `degraded`. US-069 inchange.

Etat Phase 5 (15 octobre 2026, lot 2; detail and limits in `docs/p2p.md` and `docs/ETAT_REEL.md`). US-064 ferme pour le journal Raft du guest. Pas Paxos. US-068, US-069, US-070, US-073 et US-075 avancent et restent partiels.
- Fait (preuve QEMU trois invites `make qemu-p2p` + unitaires) : US-061 protocole, US-062 decouverte, US-065 replication (LWW), US-066 chiffrement (X25519 + AES-128-GCM par paire, cle de reseau, anti-rejeu ; hub verifie l'absence de clair), US-071 synchronisation (anti-entropie apres partition), US-072 moniteur de sante (RTT, vu, erreurs, pannes).
- US-064 : InstallSnapshot pour un suiveur derriere le snapshot, lease de leader 10 s renouvele par un heartbeat qui joint un pair, pre-vote, fenetre physique 16, compaction (`p2p-raft-compact`). Scenario QEMU 2 : coupure reelle du hub (`cut_pair`), pas `p2p-block`, pas `set_link`, pas d'Internet public. Scenario 2b : gamma installe le snapshot d'index 2 puis commit l'index 3. Terme et vote restent dans le trailer du blob p2pkv. Preuve : `make qemu-p2p` (183 s) et `test_raft_snapshot_installs_for_a_lagging_follower`.
- US-067 : RPC type sur le canal chiffre (identifiant de requete, type, code de statut, echeance). Ce n'est pas gRPC HTTP/2.
- US-074 : toujours simule (STUN virtuel, full cone, punch). Pas de NAT reel.
- Partiel : US-063 routage (un relais choisi via les voisins, QEMU). US-069 tolerance aux pannes (coupure hub du scenario 2, detection, relais, resync ; pas un systeme complet). US-070 cache distribue (`p2p_cache_invalidate` envoie `P2P_I_CACHE_INV`, lecture distante deja en QEMU ; pas un cache coherent). US-068 bande passante et US-073 QoS (fenetre qui demarre a 16, moitiee sur perte d'append, +1 sur succes ; les trames de controle ne sont pas capees ; preuve unitaire ; pas un controleur adaptatif complet). US-075 analyse de trafic (regle fixe `congestion` si `loss_count` > 0, plus les regles deja en QEMU ; pas d'apprentissage).

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

Etat Phase 6 (17 octobre 2026, lot 9). US-083 ferme : `devsync-pull` entre deux invites par le canal P2P scelle (vecteur de versions, delta, dernier ecrivain gagne, empreintes egales, rien en clair ; `make qemu-p2p` scenario 1b). US-077 ferme : `mobile-ui` tape dans le bureau OS-UI VBE peint l'ecran du telephone par la liste d'affichage du volet web, verifie pixel par pixel (`test_mobile_ui`, `make qemu-osui-gui`) ; entree tactile simule. US-076, US-080, US-081 partiels ; US-078 simule.

Etat Phase 6 (17 octobre 2026, lot 8). US-076 partiel : noyau de base ARMv7-A (`arch/arm`, QEMU `-M virt` cortex-a15) avec MMU (section noyau PL1 seule), GIC, minuterie generique, taches en mode USR par SVC, preemption, IPC `kernel/ipc.c` et `kernel/sha256.c` compiles tels quels ; pilotes, FS, IA, shell et OS-UI non portes ; job CI isole `arm-port` (`make qemu-arm-boot`). US-077 partiel : coquille mobile du guest (`mobile_ui.c`, commande `mobile-ui`) : disposition telephone/tablette, cibles tactiles >= 48 px, navigation, gestes, modele de pixels hache ; pas de materiel tactile (entree simule), pas affichee sur l'ecran VBE. US-083 partiel : synchronisation multi-appareils (`devsync.c`) : carte LWW (horloge de Lamport, departage par appareil, pierres tombales), vecteurs de version, deltas texte verifies par FNV ; trois appareils dans un meme invite (simule), transport entre invites non branche.

Etat Phase 6 (17 octobre 2026, lot 7; inventaire dans `docs/lot7_inventory.md`). US-086 livre (sous-ensemble 8086) : interpreteur reel de programmes .COM (`legacy-emul`, `dosemu.c`), le stub `ok_simulated` est retire. US-078 marque simule (`sensor-read` le dit). US-081 : profils reels, pas de batterie (`battery none`). Ouverts : US-076, US-077, US-083.

Etat Phase 6 (10 octobre 2026, socle i386 ; detail dans `docs/platform.md`, preuve `make qemu-platform`) :
- Fait (sur cette plate-forme) : US-087 compatibilite applicative (ELF), US-088 migration de donnees (ramfs, archive verifiee), US-090 deploiement automatise (manifeste local, retour arriere).
- Partiel : US-076 (socle HAL i386 seulement, aucun portage ARM), US-079 gestes (classifieur, pas de peripherique tactile), US-080 adaptation d'ecran (calcul, OS-UI non re-agence), US-081 energie (profils et choix auto, politique non appliquee, pas de batterie), US-082 notifications (file locale), US-085 peripheriques (registre a partir des sondes existantes), US-089 administration (vue agregee).
- Non livre : US-077 interface mobile, US-078 capteurs, US-083 synchronisation multi-appareils (voir P2P #117), US-084 continuite, US-086 emulation legacy.

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

Etat Phase 7 (17 octobre 2026, lot 9). US-099 ferme : langage de contrats MCL dans le registre (`collab-contract-deploy`, `collab-signoff`), evalue de facon deterministe sur chaque replique. US-104 ferme comme auto-controle verifie (six controles calcules, signatures reverifiees, effacement verifie, carte RGPD ; pas une certification). US-103 ferme : preuve Schnorr NIZK dans un groupe de 2048 bits a sous-groupe de 256 bits (`collab-zk-strong`, parametres de `scripts/gen_zk_group.py`, test a reponse connue), pas en temps constant. US-097 reste simule (aucun paiement reel).

Etat Phase 7 (17 octobre 2026, lot 8). US-103 partiel, plus fort : preuve a divulgation nulle de connaissance de Schnorr (non interactive, Fiat-Shamir SHA-256, liee au message, verification de sous-groupe) dans `collab.c` (`collab-zk-prove`) ; groupe de 62 bits de taille jouet (force simule). US-099 et US-104 inchanges (partiels).

Etat Phase 7 (17 octobre 2026, lot 7). Contrats sequestre avec arbitre, litiges tranches par l'arbitre ou la majorite des membres non impliques, encheres, plies a l'identique sur les 3 invites (`make qemu-collab`, 33 entrees, meme empreinte). US-094 et US-101 livres ; US-099 partiel (modeles fixes, pas de langage de contrat) ; US-103 partiel (engagement par hachage, pas de preuve a divulgation nulle) ; US-104 partiel (4 controles calcules) ; US-097 partiel (conversion simule, pas de paiement reel).

Etat Phase 7 (10 octobre 2026, PR #119 empilee sur #117 ; detail dans `docs/collab.md`, preuve `make qemu-collab`) :
- Fait : US-091 points, US-092 ressources partagees, US-095 reputation, US-096 taches distribuees, US-098 audit, US-100 gouvernance, US-102 donnees personnelles, US-105 support.
- Fait : US-093 authentification (cle de reseau et signatures Schnorr par noeud, #121).
- Partiel : US-094 marche (offres payees, sans recherche ni encheres), US-099 contrats (modeles fixes), US-101 conflits (re-verification deterministe), US-103 confidentialite (champs prives locaux, redaction), US-104 conformite (export et oubli seulement).
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

Etat Phase 8 (17 octobre 2026, lot 9). US-112 ferme : base de signatures hors ligne (integree ou fichier, motifs hexadecimaux et empreintes FNV, severites, `prod-sec-db`), sans flux CVE en ligne. US-115 ferme : regression logistique Q12 entrainee dans l'invite (`prod-maint-train`, verdict appris de `prod-maint-predict`). US-120 reste partiel (fichier d'etat tenu a la main).

Etat Phase 8 (17 octobre 2026, lot 8). US-120 partiel : `prod-roadmap` calcule l'avancement par phase depuis `/roadmap.txt` (initrd, une ligne par US), plus de texte fige ; le fichier est tenu a la main. US-112 et US-115 inchanges (partiels).

Etat Phase 8 (17 octobre 2026, lot 7). Sorties figees remplacees par du code invite reel et prouve en QEMU : US-112 partiel (audit ELF32 W^X/pile/entree et ligne de base taille+FNV, pas de base CVE), US-108 livre en local (`prod-cicd` : lint, compilation, tests, deploiement, verification d'un programme PromptMessage), US-106 (`prod-perf-tune` mesure), US-115 partiel (heuristique), US-116 (paquet texte verifie), US-119 (compteurs observes). US-120 partiel (feuille de route statique).

Etat Phase 8 (10 octobre 2026, PR #120 ; detail dans `docs/production.md`, preuve `make qemu-production`) :
- Fait : US-107 monitoring, US-109 retour arriere automatique, US-111 sauvegarde/restauration (RAM), US-113 analyse de journaux, US-118 retours.
- Partiel : US-106 (mesures seulement), US-108 (deploiement local), US-110 (decision seulement), US-112 (integrite de fichiers), US-114 (seuils), US-115 (tendance lineaire), US-116 (diagnostic), US-117 (lecons fixes), US-119 (compteurs de commandes), US-120 (resume statique).

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

**Etat US-050 du 17 octobre 2026 (lot 4) :** toujours partiel, plus avance. L'IDE plein ecran colore la syntaxe PromptMessage (mots-cles, chaines, nombres, commentaires ; scene version 3 avec classes par caractere dessinees par `kernel/gfx_desktop.c`), complete le mot sous le curseur avec Tab (mots-cles et mots du tampon, prefixe commun puis liste des candidats) et affiche la sortie du programme de `ESC r` dans un volet (16 lignes gardees, 4 visibles, `ESC o` bascule). Preuves : `test_osui_ide`, `test_gui_pm_ide_fullscreen`, `make qemu-osui-gui`. Manquent : debogueur, completion semantique, annulation.

**Etat US-050 du 17 octobre 2026 (lot 3) :** toujours partiel, plus avance. `pm-ide FICHIER` tape dans le bureau OS-UI (`gui`) ouvre un editeur plein ecran (`userspace/osui_ide.c`, volet `OS_FB_PANE_IDE` dessine par `kernel/gfx_desktop.c`) : 96 lignes de 72 colonnes, fleches, defilement, ESC puis `w` sauver, `c` controler, `r` executer, `q` quitter (refuse si non sauve), `Q` abandonner. Preuves : `test_osui_ide`, `test_gui_pm_ide_fullscreen`, `make qemu-osui-gui`. Manquent : coloration, completion, debogueur integre, sortie du programme dans l'editeur. Dans la console texte, `pm-ide` reste l'editeur de lignes. US-059 et US-060 inchanges.

**Etat du lot phase 4 du 10 octobre 2026 :** livres dans le shell guest QEMU (C, Ring 3), voir `docs/promptmessage.md` : US-046 langage (grammaire a mots fixes, exemples de la synthese executables), US-047 compilateur (bytecode, image PMC1 avec somme de controle), US-048 interpreteur (VM bornee, `pm-run`, `pm`, declencheurs `pm-say`), US-049 validation syntaxique (ligne et colonne), US-050 editeur de lignes integre `pm-edit` (partiel : pas d'IDE plein ecran), US-051 documentation automatique `pm-doc`, US-052 debogueur (`pm-debug` trace et point d'arret, `pm-disasm`), US-053 bibliotheques (`use`, un niveau), US-054 versions (`pm-version`, `pm-versions`, 9 instantanes), US-056 optimiseur (repli de constantes), US-057 tests automatises (`expect`, `pm-test`), US-059 partiel (catalogue local `pm-catalog`/`pm-install`, pas de marketplace reseau), US-060 partiel (`pm-certify`/`pm-verify` : enregistrement local, somme FNV-1a et tests passes, pas une signature). Non livres : US-055 compilateur croise, US-058 assistance IA (pas de modele instruit hors ligne). Preuves : `test_promptmessage` et `make qemu-promptmessage` en CI. Depuis le lot de consolidation #121 : US-060 ajoute un fichier `.sig` (signature Schnorr de la cle du noeud, verifiee par `pm-verify`), sans autorite de certification. Pull request #147 ne ferme pas US-050, US-059 ni US-060 : `pm-ide` reste l'editeur de lignes (`pm-edit`, avec `.l`, `.c` et `.r`), pas un IDE plein ecran ; `pm-catalog-search` et `pm-catalog-fetch` restent un filtre et une copie du catalogue local `/pm`, pas un marketplace reseau ; `pm-ca-init`, `pm-ca-sign` et `pm-ca-verify` posent une chaine Schnorr locale (`/etc/pm_ca.pub`, cle privee en clair), pas une autorite publique. US-055 et US-058 restent non livres. Lot 1 (US-001, US-064) n'est pas livre : IDENTIFY ATA et le PIO FAT de repli restent en Ring 0 ; le journal Raft reste a 8 entrees, sans pre-vote ni compaction, et la preuve de partition est `p2p-block` (chute de trames), pas une coupure NIC.

### Règle de progression

Une étape ne passe à l'état livré qu'après implémentation, test ciblé, test de régression (`make test-all`) et mise à jour de `docs/ETAT_REEL.md`. Les compteurs historiques de ce document sont conservés pour la traçabilité ; le compteur vivant de la baseline courante est **766/766**.

### Piste GUI parity (UX5, 25 octobre 2026)

Les fonctionnalites du terminal arrivent dans le bureau graphique sans
dupliquer le code : chaque fenetre execute les vraies commandes du shell
(meme tache, memes droits noyau). Detail par famille : `docs/GUI_PARITY.md`.
Statuts comptes par `prod-roadmap` (phase 9 de `/roadmap.txt`). Ces US
completent le backlog ; elles n'en retirent aucune.

| US | Besoin | Statut |
|---|---|---|
| US-G01 | Tool windows run any shell registry command (kernel rights, terminal-only commands refused) | done |
| US-G02 | Editor: lines on the user heap, grid follows the window | partial (72-column lines, large saves refused by the overlay store) |
| US-G03 | Viewer draws PGM/PPM as pixels | partial (no PNG/JPEG/BMP) |
| US-G04 | Network panel /net | partial (status, p2p-up; no interface configuration) |
| US-G05 | Storage panel /disk | partial (ata, mounts, persist, ls, stat, mkdir) |
| US-G06 | Services and drivers panel /services | partial (read only, no restart) |
| US-G07 | Logs panel /logs | partial |
| US-G08 | Collab ledger panel /ledger | partial (read, p2p-up; no offer/vote/escrow forms) |
| US-G09 | Production / CI panel /ci (the /prod name is the existing production pane) | partial |
| US-G10 | Security / CA panel /security | partial (local Schnorr CA) |
| US-G11 | Web devtools panel /devtools | partial |
| US-G12 | File search /search and `grep -r` | done (first 8 KiB per file, 64 hits) |
| US-G13 | Notes and archives panel /notes | partial |
| US-G14 | Generic command window /cmd | done |
| US-G15 | Window-sized grid for every app | todo |
| US-G16 | Large persistent saves (overlay node 384 bytes) | todo |
| US-G17 | Panels for vfs, task, session, mcp, media, grpc | todo |

Preuves : `test_osui_parity`, `test_osui_gui`, `test_fsops`,
`test_osui_editor`, `test_gfx_desktop`, `make qemu-gui-ux5`,
`make qemu-gui-parity` (CI, job gui-parity).

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
