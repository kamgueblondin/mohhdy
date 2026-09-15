# MOHHDY - Phase 1 Foundation - User Stories Détaillées

> **État réel (août 2026).** Les incréments Foundation 01 à 68 ont livré une boîte aux lettres IPC locale, une capacité et un état explicites pour les propriétaires de service, un médiateur VFS Ring 3, ses métadonnées et son listage de racine ou sous-répertoire source-spécifiques, le registre nommé `vfs`, son nettoyage de cycle de vie, une corrélation requête-réponse, un transfert de nom, une conservation FIFO bornée des réponses non corrélées, des sources VFS virtuelles, un backend réservé au propriétaire de `vfs`, sa révocation par transfert, des montages bornés `initrd/ ro` et `overlay/ rw`, des alias de montage locaux, des notifications de changement de propriétaire best-effort, une écriture, une suppression et un renommage VFS corrélés, des statistiques VFS locales, ainsi que des lectures initrd/overlay source-spécifiques. Voir les notes [IPC](../docs/mohhdy_foundation_increment_01_ipc.md), [VFS](../docs/mohhdy_foundation_increment_02_vfs_service.md), [registre](../docs/mohhdy_foundation_increment_03_service_registry.md), [cycle de vie](../docs/mohhdy_foundation_increment_04_service_lifecycle.md), [corrélation](../docs/mohhdy_foundation_increment_05_ipc_correlation.md), [transfert](../docs/mohhdy_foundation_increment_06_service_grant.md), [différé](../docs/mohhdy_foundation_increment_07_ipc_deferred.md), [politique virtuelle](../docs/mohhdy_foundation_increment_08_vfs_virtual_policy.md), [backend réservé](../docs/mohhdy_foundation_increment_09_vfs_backend.md), [révocation](../docs/mohhdy_foundation_increment_10_vfs_revocation.md), [montages](../docs/mohhdy_foundation_increment_11_vfs_mounts.md), [notifications](../docs/mohhdy_foundation_increment_12_service_notifications.md), [écriture](../docs/mohhdy_foundation_increment_13_vfs_write.md), [lectures source-spécifiques](../docs/mohhdy_foundation_increment_14_vfs_source_reads.md), [suppression](../docs/mohhdy_foundation_increment_15_vfs_remove.md), [renommage](../docs/mohhdy_foundation_increment_16_vfs_rename.md), [statistiques](../docs/mohhdy_foundation_increment_17_vfs_stats.md), [montages dynamiques](../docs/mohhdy_foundation_increment_18_vfs_dynamic_mounts.md), [capacité IPC](../docs/mohhdy_foundation_increment_19_service_capacity.md), [état de service](../docs/mohhdy_foundation_increment_20_service_status.md) et [métadonnées VFS](../docs/mohhdy_foundation_increment_21_vfs_stat.md), ainsi que le [listage VFS](../docs/mohhdy_foundation_increment_22_vfs_list.md) et son [extension aux sous-répertoires](../docs/mohhdy_foundation_increment_23_vfs_subdir_list.md), les profils backend lecture et mutation, la [consultation unitaire de masque](../docs/mohhdy_foundation_increment_32_vfs_backend_status.md), l’[inventaire borné des délégations](../docs/mohhdy_foundation_increment_33_vfs_backend_list.md), l’observation générationnelle et la vérification de l’émetteur des réponses VFS, la [télémétrie par tâche](../docs/mohhdy_foundation_increment_38_task_metrics.md), la [politique CPU locale](../docs/mohhdy_foundation_increment_39_task_priority.md), la [filiation avec autorité locale](../docs/mohhdy_foundation_increment_40_task_authority.md), le [cycle de vie parent-enfant](../docs/mohhdy_foundation_increment_41_task_lifecycle_authority.md), la [réattribution des enfants](../docs/mohhdy_foundation_increment_42_task_reparenting.md), la [supervision d’enfant](../docs/mohhdy_foundation_increment_43_task_wait.md), la [capacité locale d’enfants](../docs/mohhdy_foundation_increment_44_child_capacity.md), la [gouvernance observable des tâches](../docs/mohhdy_foundation_increment_45_task_governance.md), le [résultat de terminaison d’enfant](../docs/mohhdy_foundation_increment_46_child_exit_result.md), l’[historique borné des résultats enfants](../docs/mohhdy_foundation_increment_47_child_result_history.md), la [supervision post-mortem](../docs/mohhdy_foundation_increment_48_postmortem_supervision.md), la [supervision sélective](../docs/mohhdy_foundation_increment_49_selective_supervision.md), la [suspension locale d’enfant](../docs/mohhdy_foundation_increment_50_task_suspension.md), la [terminaison groupée locale](../docs/mohhdy_foundation_increment_51_group_termination.md), ainsi que la [supervision multi-enfant locale](../docs/mohhdy_foundation_increment_52_multi_child_supervision.md), le [compteur cumulatif de terminaisons enfants](../docs/mohhdy_foundation_increment_53_child_exit_count.md), la [délégation locale de supervision](../docs/mohhdy_foundation_increment_54_supervision_delegation.md), le [journal borné des transitions de supervision](../docs/mohhdy_foundation_increment_55_supervision_events.md), sa [consommation post-mortem](../docs/mohhdy_foundation_increment_56_supervision_events_lifecycle.md), la [consultation sélective de ses événements](../docs/mohhdy_foundation_increment_57_selective_supervision_events.md) et leur [instantané consolidé](../docs/mohhdy_foundation_increment_58_supervision_summary.md), ainsi que leurs [notifications IPC locales](../docs/mohhdy_foundation_increment_59_supervision_notifications.md) et leur [politique de filtrage locale](../docs/mohhdy_foundation_increment_60_supervision_notification_policy.md), ainsi que leur [watchlist locale](../docs/mohhdy_foundation_increment_61_supervision_watchlist.md) et leur [télémétrie de livraison](../docs/mohhdy_foundation_increment_62_supervision_delivery_stats.md), ainsi que la [rediffusion d’événements retenus](../docs/mohhdy_foundation_increment_63_supervision_event_replay.md) et la [supervision prioritaire locale](../docs/mohhdy_foundation_increment_64_supervision_priority.md), ainsi que leur [budget local de notifications](../docs/mohhdy_foundation_increment_65_supervision_notification_budget.md). Le lot 66 ajoute les kernels freestanding GGML Q3_K, Q4_K et Q6_K, le comptage structurel correspondant dans le rapport GGUF et des comparaisons numériques Unity ; il ne fournit pas encore la table de tenseurs ni la génération GPT-2 complète depuis un checkpoint GGUF. Voir la [note des kernels GGUF](../docs/mohhdy_foundation_increment_66_gguf_kernels.md) et la [vue de tenseurs bornée](../docs/mohhdy_foundation_increment_67_gguf_tensor_view.md). Le lot 68 ajoute un volume FAT16 lecture seule sur ATA PIO à partir du LBA 64, sans toucher aux 64 secteurs AIOV, avec listage racine 8.3, suivi borné de la FAT et lecture de fichiers par `fat16-list`/`fat16-cat`. Écriture, LFN, FAT32 et sous-répertoires restent hors périmètre ; voir la [note FAT16](../docs/mohhdy_foundation_increment_68_fat16_volume.md). Le lot 52 ajoute un inventaire borné des enfants directs actifs et une attente prospective sans PID, tous deux limités au parent appelant et à la capacité locale existante. Le lot 53 ajoute un compteur local de sorties d’enfants directs, conservé au-delà des acquittements et de la fenêtre historique, mais volatil, non atomique et borné à 32 bits. Le lot 54 transfère une filiation directe vers un superviseur utilisateur valide, sans consentement, capability, persistance, historique transféré ni délégation récursive. Le lot 55 expose une fenêtre circulaire de transitions locales de supervision, utile au diagnostic mais volatile, non atomique et dépourvue de valeur d’audit de sécurité. Le lot 56 ajoute une observation conditionnelle par génération et un acquittement global de cette fenêtre, sans verrou, réservation, lecteur multiple, suppression sélective ni persistance. Le lot 57 ajoute la recherche et l’oubli d’une entrée retenue par sa séquence locale, avec compaction ordonnée, mais sans recherche globale, restauration, transaction, persistance ni audit sécurisé. Le lot 58 ajoute un instantané diagnostique consolidant enfants actifs/suspendus, départs cumulés et journal retenu, sans atomicité, verrou, réservation, quota ni action de contrôle. Le lot 59 ajoute une souscription IPC opt-in et locale, qui livre best-effort les transitions retenues de sortie, suspension, reprise et délégation, mais sans attente, accusé de réception, lecteur multiple, persistance, capability, identité vérifiée ni audit sécurisé. Le lot 60 ajoute un masque local de filtrage par type de transition et un instantané de souscription, sans masquer le journal ni les résultats ; il reste sans sélection par PID, priorité, quota, garantie, retransmission, persistance, capability, identité vérifiée ni audit sécurisé. Le lot 61 ajoute une watchlist de quatre enfants directs au plus, qui ne cible que les notifications détaillées et se nettoie à la sortie ou à la délégation sortante ; elle reste sans filtre combiné dans le shell, priorité, durée, quota, garantie, retransmission, héritage, persistance, capability, identité vérifiée ni audit sécurisé. Le lot 62 ajoute des compteurs locaux de tentatives, livraisons et pertes des messages détaillés après souscription, filtre et watchlist, ainsi que leur acquittement ; ils restent sans détail par PID/action/séquence, seuil, alerte, garantie, retransmission, persistance, capability, identité vérifiée ni audit sécurisé. Le lot 63 ajoute la rediffusion best-effort d’un événement retenu par séquence, sans repasser par les politiques de la première tentative et en exposant la saturation ; elle reste sans restauration après écrasement/oubli, attente, retransmission automatique, persistance, capability, identité vérifiée ni audit sécurisé. Le lot 64 ajoute un enfant direct prioritaire, qui contourne filtre et watchlist lorsque la souscription est active et se nettoie à la sortie ou à la délégation ; cette priorité reste sans ordonnanceur CPU, niveau multiple, réservation, garantie, persistance, capability, identité vérifiée ni audit sécurisé. Le lot 65 ajoute une limite locale configurable de tentatives de notifications détaillées, avec `0` pour le mode illimité et un compteur d’usage remis à zéro à chaque reconfiguration. Après les politiques existantes de souscription, filtre, watchlist et priorité, une tentative admise consomme une unité même si la FIFO IPC est saturée ; une limite atteinte ne bloque ni journal, ni résultats enfants, ni rediffusion explicite. Ce budget reste local, volatile, non atomique, sans réservation, fenêtre glissante, règle par PID/action, garantie, persistance, capability, identité vérifiée ni audit sécurisé. Le noyau reste monolithique (`kernel/kernel.c`) : le backend VFS, les pilotes et le réseau n’ont pas encore été déplacés dans des services isolés ; le registre ne délivre ni capability ni identité vérifiée, les événements ne sont pas accusés et les mutations VFS ne sont pas transactionnelles. Les compteurs VFS et les aliases de montage sont volatils, non atomiques et non persistants ; la capacité IPC est volatile, globale au PID propriétaire et sans priorité ni réservation par nom ; son état est un instantané non atomique. Les métadonnées VFS sont également des instantanés non atomiques, sans verrouillage ni attributs POSIX complets. Le listage VFS couvre la racine ou un sous-répertoire sûr d’un montage, mais demeure limité à une page de quatre noms au plus, sans curseur, ordre contractuel, instantané atomique ni fusion entre sources. Les critères complets de microkernel ci-dessous restent à réaliser. Runtime : [../docs/ETAT_REEL.md](../docs/ETAT_REEL.md).

## Vue d'Ensemble de la Phase 1

La Phase Foundation constitue la base technique fondamentale pour transformer MOHHDY v5.0 en MOHHDY. Cette phase se concentre sur la modernisation de l'architecture existante, l'implémentation d'un microkernel modulaire, et la mise en place des infrastructures nécessaires pour supporter les fonctionnalités avancées des phases suivantes.

L'objectif principal est de migrer l'architecture monolithique actuelle de MOHHDY vers une architecture microkernel moderne, sécurisée et extensible, tout en préservant la compatibilité avec les fonctionnalités existantes et en préparant le terrain pour l'intégration de l'IA au niveau système.

---

## US-001 : Migration du noyau MOHHDY vers architecture microkernel

### Description
**En tant que** architecte système  
**Je veux** migrer le noyau monolithique MOHHDY vers une architecture microkernel modulaire  
**Afin de** créer une base technique flexible, sécurisée et extensible pour MOHHDY

### Contexte Technique
Le noyau actuel de MOHHDY (kernel.c - 528 lignes) est implémenté dans une architecture monolithique où tous les services système s'exécutent en mode kernel (Ring 0). Cette approche, bien que fonctionnelle pour un prototype, présente des limitations importantes pour un système d'exploitation moderne destiné à héberger de l'intelligence artificielle.

### Critères d'Acceptation
1. **Séparation des Services** : Les services système (gestion mémoire, système de fichiers, réseau) sont isolés dans des processus séparés
2. **Communication IPC** : Implémentation d'un système de communication inter-processus performant
3. **Isolation Sécurisée** : Chaque service s'exécute dans son propre espace d'adressage protégé
4. **API Unifiée** : Interface de programmation cohérente pour tous les services
5. **Compatibilité** : Les applications existantes (shell, fake_ai) continuent de fonctionner
6. **Performance** : Overhead de communication IPC < 5% par rapport à l'architecture actuelle
7. **Stabilité** : Le système reste stable même en cas de crash d'un service non-critique

### Spécifications Techniques

#### Architecture Microkernel Proposée
```
┌─────────────────────────────────────────────────────────────┐
│                    User Space (Ring 3)                     │
├─────────────────────────────────────────────────────────────┤
│  Shell    │  AI Engine  │  Web Runtime  │  Applications   │
├─────────────────────────────────────────────────────────────┤
│                    System Services                          │
│  ┌─────────┐ ┌─────────┐ ┌─────────┐ ┌─────────────────┐   │
│  │   VFS   │ │   Net   │ │  Audio  │ │   Device Mgr    │   │
│  │ Service │ │ Service │ │ Service │ │     Service     │   │
│  └─────────┘ └─────────┘ └─────────┘ └─────────────────┘   │
├─────────────────────────────────────────────────────────────┤
│                    Microkernel (Ring 0)                    │
│  ┌─────────────────────────────────────────────────────┐   │
│  │  IPC Manager │ Memory Manager │ Thread Scheduler   │   │
│  │  Interrupt Handler │ Security Manager │ Timer      │   │
│  └─────────────────────────────────────────────────────┘   │
├─────────────────────────────────────────────────────────────┤
│                    Hardware Layer                           │
└─────────────────────────────────────────────────────────────┘
```

#### Composants du Microkernel
1. **IPC Manager** : Gestion des communications inter-processus
   - Message passing synchrone et asynchrone
   - Shared memory pour les gros volumes de données
   - Capabilities-based security model

2. **Memory Manager** : Gestion mémoire minimale
   - Allocation/libération de pages physiques
   - Gestion des espaces d'adressage virtuels
   - Protection mémoire entre processus

3. **Thread Scheduler** : Ordonnanceur de threads minimal
   - Scheduling préemptif round-robin
   - Gestion des priorités
   - Support SMP (préparation multi-cœur)

4. **Interrupt Handler** : Gestionnaire d'interruptions
   - Routage des interruptions vers les services appropriés
   - Gestion des IRQ et exceptions
   - Interface avec les drivers

5. **Security Manager** : Gestionnaire de sécurité
   - Contrôle d'accès basé sur les capabilities
   - Isolation des processus
   - Audit des opérations sensibles

#### Services Système Externalisés
1. **VFS Service** : Système de fichiers virtuel
   - Abstraction des systèmes de fichiers
   - Cache de métadonnées
   - Support multi-FS (initrd, web, P2P)

2. **Network Service** : Gestionnaire réseau
   - Stack TCP/IP
   - Interface P2P
   - Gestion des connexions

3. **Device Manager Service** : Gestionnaire de périphériques
   - Drivers en espace utilisateur
   - Plug-and-play
   - Gestion des ressources hardware

#### Implémentation IPC
```c
// Structure de message IPC
typedef struct {
    uint32_t sender_id;
    uint32_t receiver_id;
    uint32_t message_type;
    uint32_t data_size;
    uint8_t data[IPC_MAX_DATA_SIZE];
} ipc_message_t;

// API IPC
int ipc_send(uint32_t target_service, ipc_message_t* msg);
int ipc_receive(ipc_message_t* msg, uint32_t timeout);
int ipc_reply(uint32_t sender, ipc_message_t* reply);
```

#### Migration Progressive
1. **Phase 1a** : Extraction du gestionnaire de fichiers
2. **Phase 1b** : Extraction du gestionnaire réseau
3. **Phase 1c** : Extraction des drivers de périphériques
4. **Phase 1d** : Optimisation et tests de performance

### Dépendances
- Aucune (première US de la phase)

### Tests d'Acceptation
1. **Test de Stabilité** : Le système fonctionne 24h sans crash
2. **Test de Performance** : Benchmark des opérations système vs version monolithique
3. **Test de Sécurité** : Isolation vérifiée entre services
4. **Test de Compatibilité** : Applications existantes fonctionnent sans modification

### Estimation
**Complexité** : Très Élevée  
**Effort** : 25 jours-homme  
**Risque** : Élevé (architecture fondamentale)

---

## US-002 : Implémentation du gestionnaire de ressources intelligent

### Description
**En tant que** système MOHHDY  
**Je veux** un gestionnaire de ressources intelligent qui optimise automatiquement l'allocation CPU, mémoire et I/O  
**Afin de** maximiser les performances et l'efficacité énergétique du système

### Contexte Technique
Le gestionnaire de ressources actuel de MOHHDY est basique et ne prend pas en compte les patterns d'utilisation ou les besoins prédictifs des applications. Pour MOHHDY, nous avons besoin d'un système intelligent qui peut apprendre des habitudes utilisateur et optimiser l'allocation des ressources en conséquence.

### Critères d'Acceptation
1. **Monitoring Temps Réel** : Surveillance continue de l'utilisation CPU, mémoire, I/O
2. **Prédiction de Charge** : Algorithmes ML pour prédire les besoins futurs
3. **Allocation Dynamique** : Réallocation automatique des ressources selon les besoins
4. **Optimisation Énergétique** : Gestion intelligente de la consommation d'énergie
5. **API de Contrôle** : Interface pour que les applications déclarent leurs besoins
6. **Métriques** : Tableau de bord des performances et optimisations
7. **Apprentissage** : Le système s'améliore avec l'usage

### Spécifications Techniques

#### Architecture du Gestionnaire de Ressources
```
┌─────────────────────────────────────────────────────────────┐
│                    Resource Manager                         │
├─────────────────────────────────────────────────────────────┤
│  ┌─────────────┐ ┌─────────────┐ ┌─────────────────────┐   │
│  │   Monitor   │ │  Predictor  │ │     Allocator       │   │
│  │   Engine    │ │   Engine    │ │      Engine         │   │
│  └─────────────┘ └─────────────┘ └─────────────────────┘   │
├─────────────────────────────────────────────────────────────┤
│  ┌─────────────┐ ┌─────────────┐ ┌─────────────────────┐   │
│  │   Policy    │ │   Learning  │ │     Metrics         │   │
│  │   Engine    │ │   Engine    │ │      Engine         │   │
│  └─────────────┘ └─────────────┘ └─────────────────────┘   │
├─────────────────────────────────────────────────────────────┤
│                    Resource Pools                          │
│  ┌─────────┐ ┌─────────┐ ┌─────────┐ ┌─────────────────┐   │
│  │   CPU   │ │ Memory  │ │   I/O   │ │     Network     │   │
│  │  Pool   │ │  Pool   │ │  Pool   │ │      Pool       │   │
│  └─────────┘ └─────────┘ └─────────┘ └─────────────────┘   │
└─────────────────────────────────────────────────────────────┘
```

#### Algorithmes d'Optimisation
1. **CPU Scheduling Intelligent**
   - Algorithme CFS (Completely Fair Scheduler) amélioré
   - Prise en compte des patterns d'utilisation
   - Optimisation pour les charges IA

2. **Memory Management Prédictif**
   - Prédiction des besoins mémoire
   - Pre-loading intelligent
   - Compression mémoire adaptative

3. **I/O Optimization**
   - Réordonnancement des requêtes I/O
   - Cache adaptatif
   - Prédiction des accès fichiers

#### Implémentation des Métriques
```c
typedef struct {
    uint64_t timestamp;
    float cpu_usage;
    uint64_t memory_used;
    uint64_t memory_available;
    uint32_t io_operations;
    uint32_t network_bandwidth;
    float energy_consumption;
} resource_metrics_t;

typedef struct {
    uint32_t process_id;
    resource_metrics_t current;
    resource_metrics_t predicted;
    resource_metrics_t allocated;
} process_resources_t;
```

#### API de Gestion des Ressources
```c
// Déclaration des besoins d'une application
int resource_declare_needs(uint32_t process_id, resource_requirements_t* req);

// Demande d'allocation de ressources
int resource_request(uint32_t process_id, resource_request_t* req);

// Libération de ressources
int resource_release(uint32_t process_id, resource_type_t type, uint64_t amount);

// Obtention des métriques
int resource_get_metrics(uint32_t process_id, resource_metrics_t* metrics);
```

### Dépendances
- US-001 (Architecture microkernel)

### Tests d'Acceptation
1. **Test d'Optimisation** : Amélioration mesurable des performances (>15%)
2. **Test d'Apprentissage** : Le système s'améliore après 7 jours d'usage
3. **Test d'Efficacité Énergétique** : Réduction consommation d'énergie (>10%)
4. **Test de Réactivité** : Adaptation aux changements de charge < 100ms

### Estimation
**Complexité** : Élevée  
**Effort** : 15 jours-homme  
**Risque** : Moyen

---

## US-003 : Développement du système de sécurité adaptatif

### Description
**En tant que** utilisateur MOHHDY  
**Je veux** un système de sécurité qui s'adapte automatiquement aux menaces et apprend de mon comportement  
**Afin de** bénéficier d'une protection maximale sans friction utilisateur

### Contexte Technique
La sécurité dans MOHHDY doit être révolutionnaire, intégrant l'IA pour détecter les menaces en temps réel et s'adapter aux nouveaux patterns d'attaque. Le système doit apprendre du comportement utilisateur pour distinguer les activités légitimes des activités suspectes.

### Critères d'Acceptation
1. **Détection Comportementale** : Analyse des patterns utilisateur normaux
2. **Détection d'Anomalies** : Identification automatique des comportements suspects
3. **Réponse Adaptative** : Actions automatiques proportionnelles aux menaces
4. **Apprentissage Continu** : Amélioration des modèles de détection
5. **Zero Trust Architecture** : Vérification continue de tous les accès
6. **Chiffrement Adaptatif** : Niveau de chiffrement selon la sensibilité des données
7. **Audit Complet** : Traçabilité de toutes les opérations de sécurité

### Spécifications Techniques

#### Architecture de Sécurité Adaptative
```
┌─────────────────────────────────────────────────────────────┐
│                 Adaptive Security System                   │
├─────────────────────────────────────────────────────────────┤
│  ┌─────────────┐ ┌─────────────┐ ┌─────────────────────┐   │
│  │ Behavioral  │ │   Threat    │ │     Response        │   │
│  │  Analysis   │ │  Detection  │ │     Engine          │   │
│  └─────────────┘ └─────────────┘ └─────────────────────┘   │
├─────────────────────────────────────────────────────────────┤
│  ┌─────────────┐ ┌─────────────┐ ┌─────────────────────┐   │
│  │   Access    │ │ Encryption  │ │      Audit          │   │
│  │  Control    │ │   Manager   │ │     Logger          │   │
│  └─────────────┘ └─────────────┘ └─────────────────────┘   │
├─────────────────────────────────────────────────────────────┤
│                    Security Policies                       │
│  ┌─────────┐ ┌─────────┐ ┌─────────┐ ┌─────────────────┐   │
│  │  User   │ │ Process │ │  Data   │ │     Network     │   │
│  │Policies │ │Policies │ │Policies │ │    Policies     │   │
│  └─────────┘ └─────────┘ └─────────┘ └─────────────────┘   │
└─────────────────────────────────────────────────────────────┘
```

#### Modèles d'Apprentissage de Sécurité
1. **Behavioral Profiling**
   - Analyse des patterns de navigation
   - Reconnaissance des habitudes de travail
   - Détection des déviations comportementales

2. **Threat Intelligence**
   - Base de données de signatures de menaces
   - Corrélation avec les feeds de threat intelligence
   - Apprentissage des nouvelles techniques d'attaque

3. **Risk Scoring**
   - Score de risque dynamique par utilisateur/processus
   - Ajustement des politiques selon le score
   - Escalade automatique des mesures de sécurité

#### Implémentation Zero Trust
```c
typedef struct {
    uint32_t user_id;
    uint32_t process_id;
    uint32_t resource_id;
    security_context_t context;
    trust_level_t trust_level;
    uint64_t timestamp;
} access_request_t;

typedef enum {
    TRUST_NONE = 0,
    TRUST_LOW = 1,
    TRUST_MEDIUM = 2,
    TRUST_HIGH = 3,
    TRUST_VERIFIED = 4
} trust_level_t;

// API de contrôle d'accès
int security_check_access(access_request_t* request);
int security_update_trust(uint32_t entity_id, trust_level_t new_level);
int security_log_event(security_event_t* event);
```

#### Système de Chiffrement Adaptatif
```c
typedef enum {
    ENCRYPTION_NONE = 0,
    ENCRYPTION_BASIC = 1,    // AES-128
    ENCRYPTION_STANDARD = 2, // AES-256
    ENCRYPTION_HIGH = 3,     // AES-256 + RSA
    ENCRYPTION_QUANTUM = 4   // Post-quantum crypto
} encryption_level_t;

encryption_level_t determine_encryption_level(data_sensitivity_t sensitivity,
                                             threat_level_t threat,
                                             performance_requirements_t perf);
```

### Dépendances
- US-001 (Architecture microkernel)
- US-002 (Gestionnaire de ressources)

### Tests d'Acceptation
1. **Test de Détection** : Identification de 95% des comportements anormaux
2. **Test de Faux Positifs** : Taux de faux positifs < 2%
3. **Test d'Apprentissage** : Amélioration de la précision sur 30 jours
4. **Test de Performance** : Impact sur les performances < 3%

### Estimation
**Complexité** : Très Élevée  
**Effort** : 20 jours-homme  
**Risque** : Élevé

---

## US-004 : Création du framework de plugins modulaires

### Description
**En tant que** développeur d'extensions MOHHDY  
**Je veux** un framework de plugins sécurisé et performant  
**Afin de** étendre facilement les fonctionnalités du système sans compromettre la stabilité

### Contexte Technique
MOHHDY doit supporter un écosystème riche d'extensions et de plugins pour permettre à la communauté de développeurs de contribuer au système. Le framework doit garantir l'isolation, la sécurité et la performance tout en offrant une API riche et flexible.

### Critères d'Acceptation
1. **Isolation Sécurisée** : Chaque plugin s'exécute dans un sandbox isolé
2. **API Riche** : Interface complète pour accéder aux services système
3. **Hot Loading** : Chargement/déchargement de plugins sans redémarrage
4. **Gestion des Dépendances** : Résolution automatique des dépendances
5. **Versioning** : Support des versions multiples de plugins
6. **Marketplace Integration** : Interface avec le store de plugins
7. **Performance Monitoring** : Surveillance de l'impact des plugins

### Spécifications Techniques

#### Architecture du Framework de Plugins
```
┌─────────────────────────────────────────────────────────────┐
│                    Plugin Framework                         │
├─────────────────────────────────────────────────────────────┤
│  ┌─────────────┐ ┌─────────────┐ ┌─────────────────────┐   │
│  │   Plugin    │ │  Security   │ │     Lifecycle       │   │
│  │   Loader    │ │  Manager    │ │     Manager         │   │
│  └─────────────┘ └─────────────┘ └─────────────────────┘   │
├─────────────────────────────────────────────────────────────┤
│  ┌─────────────┐ ┌─────────────┐ ┌─────────────────────┐   │
│  │ Dependency  │ │    API      │ │     Registry        │   │
│  │  Resolver   │ │  Gateway    │ │     Service         │   │
│  └─────────────┘ └─────────────┘ └─────────────────────┘   │
├─────────────────────────────────────────────────────────────┤
│                    Plugin Sandboxes                        │
│  ┌─────────┐ ┌─────────┐ ┌─────────┐ ┌─────────────────┐   │
│  │Plugin A │ │Plugin B │ │Plugin C │ │    Plugin D     │   │
│  │(WebExt) │ │(Native) │ │(WASM)   │ │  (PromptMsg)    │   │
│  └─────────┘ └─────────┘ └─────────┘ └─────────────────┘   │
└─────────────────────────────────────────────────────────────┘
```

#### Types de Plugins Supportés
1. **Web Extensions** : Plugins JavaScript/HTML/CSS
2. **Native Plugins** : Plugins compilés (C/C++/Rust)
3. **WebAssembly Plugins** : Plugins WASM pour performance
4. **PromptMessage Plugins** : Plugins en langage MOHHDY natif

#### Manifest de Plugin
```json
{
  "name": "example-plugin",
  "version": "1.0.0",
  "description": "Plugin d'exemple pour MOHHDY",
  "author": "Développeur MOHHDY",
  "type": "web-extension",
  "permissions": [
    "filesystem.read",
    "network.http",
    "ui.notifications"
  ],
  "dependencies": {
    "mohhdy-api": "^2.0.0",
    "ui-framework": "^1.5.0"
  },
  "entry_point": "main.js",
  "sandbox_config": {
    "memory_limit": "64MB",
    "cpu_limit": "10%",
    "network_access": true,
    "filesystem_access": "restricted"
  }
}
```

#### API Plugin Framework
```c
// Structure de plugin
typedef struct {
    char name[64];
    char version[16];
    plugin_type_t type;
    permission_set_t permissions;
    sandbox_config_t sandbox;
    void* handle;
} plugin_t;

// API de gestion des plugins
int plugin_load(const char* plugin_path, plugin_t** plugin);
int plugin_unload(plugin_t* plugin);
int plugin_execute(plugin_t* plugin, const char* function, void* args);
int plugin_get_info(plugin_t* plugin, plugin_info_t* info);
```

#### Système de Permissions
```c
typedef enum {
    PERM_FILESYSTEM_READ = 1 << 0,
    PERM_FILESYSTEM_WRITE = 1 << 1,
    PERM_NETWORK_HTTP = 1 << 2,
    PERM_NETWORK_SOCKET = 1 << 3,
    PERM_UI_NOTIFICATIONS = 1 << 4,
    PERM_SYSTEM_INFO = 1 << 5,
    PERM_HARDWARE_ACCESS = 1 << 6
} plugin_permission_t;

typedef uint32_t permission_set_t;

bool plugin_has_permission(plugin_t* plugin, plugin_permission_t perm);
int plugin_request_permission(plugin_t* plugin, plugin_permission_t perm);
```

### Dépendances
- US-001 (Architecture microkernel)
- US-003 (Système de sécurité)

### Tests d'Acceptation
1. **Test d'Isolation** : Crash d'un plugin n'affecte pas le système
2. **Test de Performance** : Overhead du framework < 5%
3. **Test de Sécurité** : Respect strict des permissions
4. **Test de Compatibilité** : Support des différents types de plugins

### Estimation
**Complexité** : Élevée  
**Effort** : 18 jours-homme  
**Risque** : Moyen

---

## US-005 : Intégration du système de logging distribué

### Description
**En tant que** administrateur système MOHHDY  
**Je veux** un système de logging distribué intelligent qui collecte, analyse et corrèle les logs de tous les composants  
**Afin de** diagnostiquer rapidement les problèmes et optimiser les performances du système

### Contexte Technique
MOHHDY étant un système distribué avec de nombreux services et composants, il est crucial d'avoir un système de logging centralisé qui peut gérer des volumes importants de logs, les analyser en temps réel, et fournir des insights actionnables pour la maintenance et l'optimisation du système.

### Critères d'Acceptation
1. **Collecte Distribuée** : Collecte des logs de tous les composants système
2. **Analyse Temps Réel** : Traitement et analyse des logs en temps réel
3. **Corrélation d'Événements** : Liaison des événements liés entre composants
4. **Alertes Intelligentes** : Génération d'alertes basées sur des patterns
5. **Recherche Avancée** : Interface de recherche et filtrage des logs
6. **Archivage Intelligent** : Compression et archivage automatique
7. **Visualisation** : Dashboards et graphiques de monitoring

### Spécifications Techniques

#### Architecture du Système de Logging
```
┌─────────────────────────────────────────────────────────────┐
│                 Distributed Logging System                 │
├─────────────────────────────────────────────────────────────┤
│  ┌─────────────┐ ┌─────────────┐ ┌─────────────────────┐   │
│  │    Log      │ │    Event    │ │      Alert          │   │
│  │ Collector   │ │ Correlator  │ │     Engine          │   │
│  └─────────────┘ └─────────────┘ └─────────────────────┘   │
├─────────────────────────────────────────────────────────────┤
│  ┌─────────────┐ ┌─────────────┐ ┌─────────────────────┐   │
│  │   Search    │ │  Analytics  │ │     Storage         │   │
│  │   Engine    │ │   Engine    │ │     Manager         │   │
│  └─────────────┘ └─────────────┘ └─────────────────────┘   │
├─────────────────────────────────────────────────────────────┤
│                      Log Sources                           │
│  ┌─────────┐ ┌─────────┐ ┌─────────┐ ┌─────────────────┐   │
│  │ Kernel  │ │Services │ │ Plugins │ │   Applications  │   │
│  │  Logs   │ │  Logs   │ │  Logs   │ │      Logs       │   │
│  └─────────┘ └─────────┘ └─────────┘ └─────────────────┘   │
└─────────────────────────────────────────────────────────────┘
```

#### Format de Log Structuré
```c
typedef struct {
    uint64_t timestamp;
    log_level_t level;
    char component[32];
    char category[32];
    uint32_t thread_id;
    uint32_t process_id;
    char message[512];
    key_value_pair_t metadata[16];
    uint32_t metadata_count;
} log_entry_t;

typedef enum {
    LOG_TRACE = 0,
    LOG_DEBUG = 1,
    LOG_INFO = 2,
    LOG_WARN = 3,
    LOG_ERROR = 4,
    LOG_FATAL = 5
} log_level_t;
```

#### API de Logging
```c
// Macros de logging
#define LOG_TRACE(component, format, ...) \
    log_write(LOG_TRACE, component, __FILE__, __LINE__, format, ##__VA_ARGS__)

#define LOG_DEBUG(component, format, ...) \
    log_write(LOG_DEBUG, component, __FILE__, __LINE__, format, ##__VA_ARGS__)

#define LOG_INFO(component, format, ...) \
    log_write(LOG_INFO, component, __FILE__, __LINE__, format, ##__VA_ARGS__)

// Fonctions de logging
int log_write(log_level_t level, const char* component, 
              const char* file, int line, const char* format, ...);
int log_write_structured(log_entry_t* entry);
int log_set_level(const char* component, log_level_t level);
```

#### Système d'Alertes
```c
typedef struct {
    char name[64];
    char condition[256];
    alert_severity_t severity;
    uint32_t threshold;
    uint32_t time_window;
    char action[128];
} alert_rule_t;

typedef enum {
    ALERT_INFO = 0,
    ALERT_WARNING = 1,
    ALERT_CRITICAL = 2,
    ALERT_EMERGENCY = 3
} alert_severity_t;

// API d'alertes
int alert_create_rule(alert_rule_t* rule);
int alert_delete_rule(const char* rule_name);
int alert_trigger(const char* rule_name, log_entry_t* trigger_event);
```

#### Corrélation d'Événements
```c
typedef struct {
    char correlation_id[64];
    uint64_t start_time;
    uint64_t end_time;
    log_entry_t* events;
    uint32_t event_count;
    correlation_type_t type;
} event_correlation_t;

typedef enum {
    CORRELATION_REQUEST_TRACE = 0,
    CORRELATION_ERROR_CASCADE = 1,
    CORRELATION_PERFORMANCE_ISSUE = 2,
    CORRELATION_SECURITY_INCIDENT = 3
} correlation_type_t;
```

### Dépendances
- US-001 (Architecture microkernel)
- US-002 (Gestionnaire de ressources)

### Tests d'Acceptation
1. **Test de Volume** : Gestion de 10,000 logs/seconde sans perte
2. **Test de Corrélation** : Identification correcte des événements liés
3. **Test d'Alertes** : Génération d'alertes en < 1 seconde
4. **Test de Recherche** : Recherche dans 1M de logs en < 5 secondes

### Estimation
**Complexité** : Élevée  
**Effort** : 12 jours-homme  
**Risque** : Moyen

---

## Résumé de la Phase 1

La Phase Foundation établit les bases techniques solides pour MOHHDY avec 15 User Stories couvrant :

### Composants Architecturaux Critiques
- **Microkernel modulaire** pour flexibilité et sécurité
- **Gestionnaire de ressources intelligent** pour optimisation automatique
- **Système de sécurité adaptatif** pour protection proactive
- **Framework de plugins** pour extensibilité
- **Logging distribué** pour observabilité

### Métriques de Succès
- **Performance** : Amélioration de 15% par rapport à MOHHDY v5.0
- **Sécurité** : Détection de 95% des anomalies avec <2% de faux positifs
- **Stabilité** : Fonctionnement 24h/24 sans crash
- **Extensibilité** : Support de 100+ plugins simultanés

### Préparation pour les Phases Suivantes
Cette phase prépare l'infrastructure nécessaire pour :
- L'intégration de l'IA au niveau système (Phase 2)
- Le développement du navigateur-OS (Phase 3)
- L'implémentation du langage PromptMessage (Phase 4)
- Le déploiement du réseau P2P (Phase 5)

La réussite de cette phase est critique pour le succès global du projet MOHHDY, car elle établit les fondations sur lesquelles toutes les innovations futures seront construites.

