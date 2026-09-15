# MOHHDY - Phase 3 Web Runtime - User Stories Détaillées

> **État réel (août 2026).** Phase **non implémentée** (pas de navigateur-OS). Hors suite proche du prototype ([mohhdy_us.md](mohhdy_us.md)). Specs conservées. Runtime : [../docs/ETAT_REEL.md](../docs/ETAT_REEL.md).

## Vue d'Ensemble de la Phase 3

La Phase Web Runtime transforme MOHHDY en un système d'exploitation basé sur un navigateur amélioré, réalisant la vision d'un OS où toutes les applications sont des applications web et où le navigateur devient le système de fichiers et l'explorateur principal. Cette phase implémente l'idée révolutionnaire que "le web c'est l'avenir" et que MOHHDY doit exploiter l'écosystème existant d'applications web.

L'objectif principal est de créer un navigateur-OS intégré qui peut gérer les fichiers, exécuter des applications web comme des applications natives, et fournir une interface responsive qui s'adapte automatiquement à toutes les plateformes. Cette phase établit MOHHDY comme un concurrent direct des OS traditionnels en exploitant la puissance et la flexibilité du web.

---

## US-031 : Développement du navigateur-OS intégré

### Description
**En tant que** utilisateur MOHHDY  
**Je veux** un navigateur intégré qui fonctionne comme mon système d'exploitation principal  
**Afin de** accéder à toutes mes applications et fichiers via une interface web unifiée

### Contexte Technique
Le navigateur-OS est le cœur de l'expérience utilisateur MOHHDY. Il doit combiner les fonctionnalités d'un navigateur web moderne avec celles d'un système d'exploitation, permettant de naviguer sur internet, gérer les fichiers locaux, et exécuter des applications web comme des applications natives.

### Critères d'Acceptation
1. **Moteur Web Moderne** : Basé sur Chromium/WebKit avec support des standards récents
2. **Interface OS Intégrée** : Barre de tâches, gestionnaire de fenêtres, notifications
3. **Gestion Multi-Onglets** : Onglets comme processus système avec isolation
4. **Extensions Natives** : Support des extensions web comme plugins système
5. **Performance Optimisée** : Démarrage rapide et consommation mémoire optimisée
6. **Sécurité Renforcée** : Sandboxing avancé et protection contre les menaces
7. **Intégration IA** : Assistant IA intégré pour navigation et recherche

### Spécifications Techniques

#### Architecture du Navigateur-OS
```
┌─────────────────────────────────────────────────────────────┐
│                    Browser-OS Core                          │
├─────────────────────────────────────────────────────────────┤
│  ┌─────────────┐ ┌─────────────┐ ┌─────────────────────┐   │
│  │   Render    │ │   Process   │ │      Window         │   │
│  │   Engine    │ │   Manager   │ │     Manager         │   │
│  └─────────────┘ └─────────────┘ └─────────────────────┘   │
├─────────────────────────────────────────────────────────────┤
│  ┌─────────────┐ ┌─────────────┐ ┌─────────────────────┐   │
│  │ Extension   │ │  Security   │ │       AI            │   │
│  │  Manager    │ │  Sandbox    │ │   Assistant         │   │
│  └─────────────┘ └─────────────┘ └─────────────────────┘   │
├─────────────────────────────────────────────────────────────┤
│                    Web Standards                           │
│  ┌─────────┐ ┌─────────┐ ┌─────────┐ ┌─────────────────┐   │
│  │  HTML5  │ │  CSS3   │ │   JS    │ │   WebAssembly   │   │
│  │   DOM   │ │ Flexbox │ │  ES2023 │ │      WASM       │   │
│  └─────────┘ └─────────┘ └─────────┘ └─────────────────┘   │
└─────────────────────────────────────────────────────────────┘
```

#### Intégration Système d'Exploitation
```c
typedef struct {
    char title[256];
    char url[512];
    process_id_t process_id;
    window_handle_t window;
    tab_state_t state;
    security_context_t security;
    resource_usage_t resources;
} browser_tab_t;

typedef enum {
    TAB_LOADING = 0,
    TAB_LOADED = 1,
    TAB_ERROR = 2,
    TAB_SUSPENDED = 3,
    TAB_BACKGROUND = 4
} tab_state_t;

// API de gestion des onglets
int browser_create_tab(const char* url, browser_tab_t** tab);
int browser_close_tab(browser_tab_t* tab);
int browser_navigate_tab(browser_tab_t* tab, const char* url);
int browser_suspend_tab(browser_tab_t* tab);
int browser_resume_tab(browser_tab_t* tab);
```

#### Interface Système Intégrée
```c
typedef struct {
    char name[64];
    char icon_path[256];
    char url[512];
    bool pinned_to_taskbar;
    bool auto_start;
    window_type_t window_type;
} web_app_t;

typedef enum {
    WINDOW_NORMAL = 0,
    WINDOW_POPUP = 1,
    WINDOW_FULLSCREEN = 2,
    WINDOW_KIOSK = 3
} window_type_t;

// API d'applications web
int browser_register_web_app(web_app_t* app);
int browser_launch_web_app(const char* app_name);
int browser_unregister_web_app(const char* app_name);
int browser_get_installed_apps(web_app_t* apps, int max_count);
```

### Dépendances
- US-001 (Architecture microkernel)
- US-016 (Moteur IA local)

### Tests d'Acceptation
1. **Test de Performance** : Démarrage en < 3 secondes
2. **Test de Mémoire** : < 512MB pour 10 onglets ouverts
3. **Test de Compatibilité** : Support de 95% des sites web modernes
4. **Test de Sécurité** : Isolation complète entre onglets

### Estimation
**Complexité** : Très Élevée  
**Effort** : 30 jours-homme  
**Risque** : Élevé

---

## US-032 : Création du gestionnaire d'applications web natives

### Description
**En tant que** utilisateur MOHHDY  
**Je veux** que les applications web se comportent comme des applications natives  
**Afin de** avoir une expérience utilisateur fluide et intégrée

### Contexte Technique
Le gestionnaire d'applications web natives transforme les Progressive Web Apps (PWA) et applications web en applications système complètes, avec intégration dans la barre de tâches, notifications, accès aux APIs système, et comportement natif.

### Critères d'Acceptation
1. **Installation PWA** : Installation automatique des Progressive Web Apps
2. **Intégration Système** : Applications dans le menu démarrer et barre de tâches
3. **Notifications Natives** : Support des notifications système
4. **Accès APIs** : Accès sécurisé aux APIs système (fichiers, caméra, etc.)
5. **Mode Hors Ligne** : Fonctionnement sans connexion internet
6. **Mise à Jour Automatique** : Mise à jour transparente des applications
7. **Gestion des Permissions** : Contrôle granulaire des permissions

### Spécifications Techniques

#### Architecture du Gestionnaire PWA
```
┌─────────────────────────────────────────────────────────────┐
│              Native Web App Manager                        │
├─────────────────────────────────────────────────────────────┤
│  ┌─────────────┐ ┌─────────────┐ ┌─────────────────────┐   │
│  │     PWA     │ │ Manifest    │ │     Service         │   │
│  │  Installer  │ │  Parser     │ │     Worker          │   │
│  └─────────────┘ └─────────────┘ └─────────────────────┘   │
├─────────────────────────────────────────────────────────────┤
│  ┌─────────────┐ ┌─────────────┐ ┌─────────────────────┐   │
│  │ Permission  │ │ Notification│ │      Update         │   │
│  │  Manager    │ │   Manager   │ │     Manager         │   │
│  └─────────────┘ └─────────────┘ └─────────────────────┘   │
├─────────────────────────────────────────────────────────────┤
│                    System Integration                      │
│  ┌─────────┐ ┌─────────┐ ┌─────────┐ ┌─────────────────┐   │
│  │Taskbar  │ │ Start   │ │ File    │ │   Shortcuts     │   │
│  │ Icons   │ │  Menu   │ │ Assoc   │ │   Desktop       │   │
│  └─────────┘ └─────────┘ └─────────┘ └─────────────────┘   │
└─────────────────────────────────────────────────────────────┘
```

### Dépendances
- US-031 (Navigateur-OS intégré)

### Tests d'Acceptation
1. **Test d'Installation** : Installation PWA en < 10 secondes
2. **Test d'Intégration** : Applications visibles dans le système
3. **Test Hors Ligne** : Fonctionnement sans internet
4. **Test de Permissions** : Contrôle sécurisé des accès

### Estimation
**Complexité** : Élevée  
**Effort** : 18 jours-homme  
**Risque** : Moyen

---

## US-033 : Implémentation du système de fichiers web

### Description
**En tant que** utilisateur MOHHDY  
**Je veux** gérer mes fichiers via une interface web moderne et intuitive  
**Afin de** avoir une expérience unifiée pour tous mes contenus

### Contexte Technique
Le système de fichiers web remplace l'explorateur de fichiers traditionnel par une interface web moderne qui peut gérer les fichiers locaux, cloud, et distribués via une interface unifiée. Il doit supporter tous les types de médias avec prévisualisation et édition intégrées.

### Critères d'Acceptation
1. **Interface Web Moderne** : Interface responsive et intuitive
2. **Support Multi-Sources** : Fichiers locaux, cloud, P2P
3. **Prévisualisation Intégrée** : Aperçu de tous types de fichiers
4. **Édition en Ligne** : Édition directe des documents
5. **Recherche Intelligente** : Recherche avec IA intégrée
6. **Synchronisation** : Sync automatique multi-appareils
7. **Partage Avancé** : Partage sécurisé avec permissions

### Spécifications Techniques

#### Architecture du Système de Fichiers Web
```
┌─────────────────────────────────────────────────────────────┐
│                  Web File System                           │
├─────────────────────────────────────────────────────────────┤
│  ┌─────────────┐ ┌─────────────┐ ┌─────────────────────┐   │
│  │    File     │ │   Preview   │ │      Search         │   │
│  │  Browser    │ │   Engine    │ │      Engine         │   │
│  └─────────────┘ └─────────────┘ └─────────────────────┘   │
├─────────────────────────────────────────────────────────────┤
│  ┌─────────────┐ ┌─────────────┐ ┌─────────────────────┐   │
│  │   Editor    │ │    Sync     │ │      Share          │   │
│  │  Manager    │ │   Manager   │ │     Manager         │   │
│  └─────────────┘ └─────────────┘ └─────────────────────┘   │
├─────────────────────────────────────────────────────────────┤
│                   Storage Backends                         │
│  ┌─────────┐ ┌─────────┐ ┌─────────┐ ┌─────────────────┐   │
│  │  Local  │ │  Cloud  │ │   P2P   │ │    Network      │   │
│  │ Storage │ │ Storage │ │ Storage │ │    Drives       │   │
│  └─────────┘ └─────────┘ └─────────┘ └─────────────────┘   │
└─────────────────────────────────────────────────────────────┘
```

### Dépendances
- US-031 (Navigateur-OS intégré)
- US-016 (Moteur IA local)

### Tests d'Acceptation
1. **Test de Performance** : Navigation fluide avec 10,000+ fichiers
2. **Test de Prévisualisation** : Support de 20+ formats de fichiers
3. **Test de Recherche** : Résultats pertinents en < 1 seconde
4. **Test de Synchronisation** : Sync en temps réel

### Estimation
**Complexité** : Élevée  
**Effort** : 22 jours-homme  
**Risque** : Moyen

---

## Résumé des Phases Restantes

### Phase 4 - PromptMessage (US-046 à US-060)
Création du langage universel PromptMessage pour programmer et communiquer avec MOHHDY.

### Phase 5 - P2P Network (US-061 à US-075)  
Implémentation du réseau distribué P2P pour l'interconnexion des instances MOHHDY.

### Phase 6 - Multi-Platform (US-076 à US-090)
Adaptation du système pour fonctionner sur toutes les plateformes (desktop, mobile, IoT).

### Phase 7 - Collaborative (US-091 à US-105)
Système de points et économie collaborative pour partager les ressources communautaires.

### Phase 8 - Production (US-106 à US-120)
Optimisation, monitoring et déploiement en production du système complet.

La Phase 3 Web Runtime établit MOHHDY comme un véritable navigateur-OS, préparant l'infrastructure web nécessaire pour les phases suivantes qui exploiteront cette base pour créer un écosystème complet d'applications et de services distribués.

