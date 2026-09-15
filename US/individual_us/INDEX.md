# Index MOHHDY — fichiers de spécification

> **Légende (août 2026)**  
> - **Spec** = fichier rédigé, **pas** livré dans le noyau.  
> - **Chevauchement** = le prototype MOHHDY a un voisinage technique (souvent une fraction du besoin).  
> - **Livraison partielle** = mécanisme réellement compilé et testé, sans satisfaire tous les critères de la spec.
> - Backlog du code réel : [../mohhdy_us.md](../mohhdy_us.md). Runtime : [../../docs/ETAT_REEL.md](../../docs/ETAT_REEL.md).

Les titres du [document maître](../mohhdy_user_stories_master.md) **ne correspondent pas toujours** aux noms de fichiers ci-dessous (ex. maître US-008 = « mise à jour automatique », fichier = tests automatisés). **Le fichier individuel fait foi** pour le texte de la spec. Les IDs **US-023, US-024 et US-025 existent en double**.

Il n'y a **pas** 120 fichiers : environ 78 specs détaillées + des phases décrites seulement dans les documents `mohhdy_*.md`.

## Chevauchement avec MOHHDY

| US fichier | Spec MOHHDY | Dans le prototype |
|---|---|---|
| US-001 | Microkernel + IPC | **Livraison partielle :** IPC avec capacité de deux messages clients et instantané de profondeur pour un propriétaire publié, VFS Ring 3 lecture-écriture-suppression-renommage avec lectures, métadonnées et listage de racine ou sous-répertoire initrd/overlay distincts, statistiques locales, trois alias de montage dynamiques au plus, découverte `vfs`, cycle de vie, corrélation, conservation différée, transfert, révocation et notifications best-effort ; noyau monolithique, backend non externalisé |
| US-002 | Gestionnaire de ressources IA | PMM / VMM / heap / `SYS_MEMINFO` seulement |
| US-003 | Sécurité adaptative IA | Isolation Ring 0/3 et PID d’émetteur IPC attribué par le noyau ; pas de capabilities ni de détection de menaces |
| US-007 | Monitoring temps réel | `ps` / `mem` / `uptime` / `SYS_TICKS`, pas de télémétrie |
| US-008 | Framework de tests IA | Unity 490 + contrats QEMU (coeur, IRQ0, fournisseur IA, NE2000, `ai-acquire`, IPC, VFS FAT16/FAT32 mutate, cycle de vie, transfert, revocation et notifications) + GitHub Actions ; pas de framework distribue |
| US-010 | Pilotes modulaires | PIC, PIT, PS/2, ATA PIO, NE2000 ISA ; pas de framework de drivers |
| US-012 | APIs unifiees | `include/os_syscalls.h` (syscalls 0-121, `MAX_SYSCALLS = 122`), IPC avec `request_id` opaque, VFS lecture-ecriture-suppression-renommage, FAT16 et FAT32 8.3 racine mutables, sockets 99-108, session LLM 90-98, `SYS_NET_STATUS` |
| US-013 | Communication inter-services | **Livraison partielle :** IPC avec saturation et instantané de file par propriétaire de service, VFS local de lecture-écriture-suppression-renommage et listage de racine ou sous-répertoire avec lectures et métadonnées de sources distinctes, statistiques volatiles et alias de montage bornés, registre, cycle de vie, corrélation, conservation bornée des réponses, transfert, révocation et notifications best-effort ; pas de capabilities, d’identité vérifiée, de persistance, de priorité ni de garantie de livraison |
| US-016 | Moteur TensorFlow Lite | GPT-2 124M freestanding (`SYS_GPT2_GENERATE`), pas TFLite |
| US-017 | NLU 90 % d'intentions | BPE + complétion 12 jetons, pas d'analyse d'intention |
| US-021 | Assistant IA proactif | Builtin `ai <texte>` synchrone et borné |
| US-066 | Gestion mémoire avancée | Même voisinage que US-002 (PMM/VMM) |
| Autres | — | **Spec uniquement** |

## Fichiers présents (`individual_us/`)

Chaque entrée : `US-XXX` + nom de fichier. Statut implicite **Spec**, sauf mention dans le tableau ci-dessus.

### Phase 1 — Foundation (fichiers)

- US-001 `US-001_Architecture_Microkernel.md`
- US-002 `US-002_Gestionnaire_Ressources_Intelligent.md` (chevauchement mémoire)
- US-003 `US-003_Systeme_Securite_Adaptatif.md`
- US-004 `US-004_Framework_Plugins_Modulaires.md`
- US-005 `US-005_Systeme_Logging_Distribue.md`
- US-006 `US-006_Gestionnaire_Configuration_Dynamique.md`
- US-007 `US-007_Systeme_Monitoring_Temps_Reel.md` (chevauchement `ps`/`mem`/`uptime`)
- US-008 `US-008_Framework_Tests_Automatises.md` (chevauchement Unity + CI)
- US-009 `US-009_Systeme_Mise_A_Jour_Incrementale.md`
- US-010 `US-010_Gestionnaire_Pilotes_Modulaires.md` (chevauchement pilotes QEMU)
- US-011 `US-011_Systeme_Virtualisation_Leger.md`
- US-012 `US-012_Framework_APIs_Unifiees.md` (chevauchement ABI syscalls)
- US-013 `US-013_Communication_Inter_Services.md`
- US-014 `US-014_Gestionnaire_Performance_Metriques.md`
- US-015 `US-015_Framework_Deploiement_Orchestration.md`

### Phase 2 — AI Core (fichiers)

- US-016 `US-016_Moteur_IA_Local.md` (chevauchement GPT-2, **pas** TFLite)
- US-017 `US-017_Systeme_Comprehension_Langage_Naturel.md`
- US-018 `US-018_Gestionnaire_Modeles_IA_Distribues.md`
- US-019 `US-019_Systeme_Apprentissage_Federe.md`
- US-020 `US-020_Orchestrateur_Cloud_Edge.md`
- US-021 `US-021_Assistant_IA_Integre.md` (chevauchement commande `ai`)
- US-022 `US-022_Systeme_Personnalisation_IA.md`
- US-023 **doublon** `US-023_Systeme_Apprentissage_Machine_Adaptatif.md` et `US-023_Moteur_Recommandations_Intelligent.md`
- US-024 **doublon** `US-024_Systeme_Optimisation_Automatique.md` et `US-024_Framework_Integration_IA_Tiers.md`
- US-025 **doublon** `US-025_Systeme_Prediction_Maintenance.md` et `US-025_Framework_Explicabilite_IA.md`
- US-026 `US-026_Orchestrateur_Workflows_IA.md`
- US-027 `US-027_Systeme_Analyse_Comportementale.md`
- US-028 `US-028_Module_Intelligence_Conversationnelle.md`
- US-029 `US-029_Framework_Optimisation_Automatique.md`
- US-030 `US-030_Plateforme_IA_Ethique_Explicable.md`

### Phases 3+ (fichiers, IDs ≠ découpage maître 031=navigateur)

Les numéros 031-075 des **fichiers** parlent surtout écosystème, sécurité et performance (marketplace, RGPD, zero-trust, etc.). Ce n'est **pas** la liste « navigateur-OS / PromptMessage / P2P » du document maître. Les phases 4-8 du maître (US-046 à US-120 : PromptMessage, P2P, mobile, points, production) n'ont **pas** de fichier individuel homonyme.

- US-031 `US-031_Centre_Distribution_Applications.md`
- US-032 `US-032_SDK_Developpeur_Multi_Plateforme.md`
- US-033 `US-033_Marketplace_Ecosysteme_Communautaire.md`
- US-034 `US-034_Connecteurs_Systemes_Entreprise.md`
- US-035 `US-035_Plateforme_Integration_Cloud_Hybride.md`
- US-036 `US-036_Systeme_Federation_Identites.md`
- US-037 `US-037_Hub_Collaboration_Etendue.md`
- US-038 `US-038_Passerelle_IoT_Edge_Computing.md`
- US-039 `US-039_Ecosysteme_Plugins_Modulaires.md`
- US-040 `US-040_Interface_Programmation_Universelle.md`
- US-041 `US-041_Reseau_Partenaires_Certifies.md`
- US-042 `US-042_Plateforme_Innovation_Ouverte.md`
- US-043 `US-043_Systeme_Metriques_Analytics_Ecosysteme.md`
- US-044 `US-044_Programme_Gouvernance_Ecosysteme.md`
- US-045 `US-045_Initiatives_Durabilite_Impact_Social.md`
- US-046 `US-046_Systeme_Securite_Multi_Couches.md`
- US-047 `US-047_Gestion_Avancee_Identites_Acces.md`
- US-048 `US-048_Conformite_RGPD_Protection_Donnees.md`
- US-049 `US-049_Audit_Compliance_Automatiques.md`
- US-050 `US-050_Systeme_Gestion_Incidents_Securite.md`
- US-051 `US-051_Chiffrement_Avance_Gestion_Cles.md`
- US-052 `US-052_Tests_Penetration_Vulnerabilites.md`
- US-053 `US-053_Surveillance_Comportementale_Avancee.md`
- US-054 `US-054_Backup_Disaster_Recovery_Securises.md`
- US-055 `US-055_Securite_Chaines_Approvisionnement.md`
- US-056 `US-056_Formation_Sensibilisation_Securite.md`
- US-057 `US-057_Gestion_Menaces_Threat_Intelligence.md`
- US-058 `US-058_Zero_Trust_Architecture.md`
- US-059 `US-059_Securite_DevSecOps_Integree.md`
- US-060 `US-060_Resilience_Continuite_Securite.md`
- US-061 `US-061_Optimisation_Performance_Multi_Niveaux.md`
- US-062 `US-062_Auto_Scaling_Intelligent_Adapte.md`
- US-063 `US-063_Optimisation_Base_Donnees_Avancee.md`
- US-064 `US-064_Gestionnaire_Charge_Avance.md`
- US-065 `US-065_Optimisation_Reseau_Latence.md`
- US-066 `US-066_Gestion_Memoire_Ressources.md` (chevauchement PMM/VMM)
- US-067 `US-067_Systeme_Cache_Distribue_Intelligent.md`
- US-068 `US-068_Optimisation_Algorithmes_Traitements.md`
- US-069 `US-069_Monitoring_Performance_Temps_Reel.md`
- US-070 `US-070_Optimisation_Stockage_IO.md`
- US-071 `US-071_Test_Charge_Stress_Automatise.md`
- US-072 `US-072_Optimisation_Energetique_Green_IT.md`
- US-073 `US-073_Orchestration_Workload_Intelligente.md`
- US-074 `US-074_Optimisation_Continue_Adaptee.md`
- US-075 `US-075_Benchmark_Comparaison_Performance.md`

Pas de fichiers `US-076` … `US-120`.

## Structure d'un fichier US

Métadonnées (id, phase, effort) → « En tant que / Je veux / Afin de » → spec technique → critères d'acceptation → tests imaginés. Les critères (NLU 90 %, 1000 nœuds P2P, TFLite, etc.) **ne s'appliquent pas** au prototype.

## Ancienne légende (obsolète)

« Complétées : 4 / Restantes : 116 » comptait mal les fichiers et prenait ✅ pour « implémenté ». Remplacé par le tableau de chevauchement et l'inventaire ci-dessus.
