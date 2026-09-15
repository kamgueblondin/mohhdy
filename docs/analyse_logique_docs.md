# Analyse de la Logique du Projet MOHHDY

> **État réel (août 2026).** Ce fichier décrit la logique des étapes 1–7 d’origine. L’ABI courante, GPT-2 local et l’overlay AIOV sont dans [ETAT_REEL.md](ETAT_REEL.md). MOHHDY n’est pas une distribution Linux. Lexique : [vocabulaire.md](vocabulaire.md).

## Vue d'Ensemble du Projet

MOHHDY est un système d'exploitation spécialement conçu pour héberger et exécuter des applications d'intelligence artificielle de manière sécurisée et efficace. Le projet suit une approche progressive par étapes, chaque version ajoutant des fonctionnalités essentielles.

## Architecture Évolutive

### Étape 1 : Socle de Base (Démarrage et Noyau)
- **Objectif** : Créer un noyau minimaliste qui démarre
- **Composants** :
  - `boot/boot.s` : Point d'entrée assembleur avec en-tête Multiboot
  - `kernel/kernel.c` : Noyau basique avec affichage VGA
  - `linker.ld` : Script de liaison pour organiser la mémoire
  - `Makefile` : Automatisation de la compilation

### Étapes 3-4 : Gestion Mémoire et Système de Fichiers
- **Physical Memory Manager (PMM)** : Gestion de la RAM avec bitmap
- **Virtual Memory Manager (VMM)** : Paging complet avec protection
- **Système de fichiers Initrd** : Parser TAR pour fichiers en mémoire
- **Support Multiboot** : Récupération des informations système

### Étapes 5-6 : Multitâche et Espace Utilisateur (Version 4.0)
- **Système de tâches** : Multitâche préemptif avec ordonnanceur Round-Robin
- **Changement de contexte** : Optimisé en assembleur
- **Séparation Ring 0/3** : Isolation kernel/utilisateur
- **Appels système** : Interface sécurisée (**15** syscalls : exit, putc, getc, puts, yield, gets, exec, spawn, listdir, readfile, getpid, ps, kill, ticks, meminfo ; le chiffre 5 ci-dessous est l’état à l’étape 5–6 d’origine)
- **Chargeur ELF** : Exécution de programmes externes

## Logique Technique Détaillée

### 1. Architecture Mémoire
```
Adresses Virtuelles    Adresses Physiques
0x00000000-0x003FFFFF → 0x00000000-0x003FFFFF (4 Mo, mapping 1:1)
0x00400000+           → Gestion dynamique
```

### 2. Structure des Tâches
- **États** : RUNNING, READY, WAITING, TERMINATED
- **Types** : Kernel et User
- **Ordonnancement** : Round-Robin avec quantum de 10ms (100Hz)
- **Isolation** : Espaces mémoire séparés via paging

### 3. Appels Système
- **Mécanisme** : Interruption INT 0x80
- **Transition** : Ring 3 → Ring 0 automatique
- **Syscalls disponibles** :
  - SYS_EXIT (0) : Termine le processus
  - SYS_PUTC (1) : Affiche un caractère
  - SYS_GETC (2) : Lit un caractère
  - SYS_PUTS (3) : Affiche une chaîne
  - SYS_YIELD (4) : Cède le CPU

### 4. Système de Fichiers
- **Format** : archive TAR (ustar) pour simplicité — ce n’est pas un volume disque ni une identité POSIX
- **Localisation** : Initrd chargé par Multiboot
- **Fonctionnalités** : Lecture, listage, validation checksums

## Logique de Développement

### Approche Progressive
1. **Démarrage minimal** → Affichage de base
2. **Gestion mémoire** → Allocation dynamique
3. **Système de fichiers** → Accès aux données
4. **Multitâche** → Exécution parallèle
5. **Espace utilisateur** → Sécurité et isolation

### Modularité
- **Séparation claire** : Chaque module a une responsabilité précise
- **Interfaces définies** : APIs claires entre composants
- **Extensibilité** : Architecture prête pour nouvelles fonctionnalités

### Sécurité
- **Isolation mémoire** : Protection via paging
- **Séparation des privilèges** : Ring 0/3
- **Validation** : Contrôle des paramètres syscalls
- **Gestion d'erreurs** : Prévention des crashes

## Préparation pour l'IA

### Infrastructure Nécessaire
- **Mémoire dynamique** : Pour charger modèles et données
- **Isolation des processus** : Sécurité pour l'exécution d'IA
- **Communication contrôlée** : Via syscalls uniquement
- **Gestion de ressources** : Ordonnancement équitable

### Prochaines Étapes Planifiées
- **Version 5.0** : Shell interactif et commandes avancées
- **Version 6.0** : Stack TCP/IP et communication réseau
- **Version 7.0** : Moteur d'inférence et API IA

## Points Forts de l'Architecture

### Performance
- **Changement de contexte optimisé** : Assembleur pur
- **Ordonnanceur O(1)** : Efficacité constante
- **Timer haute fréquence** : Réactivité 100Hz

### Robustesse
- **Gestion d'erreurs complète** : Validation systématique
- **Tests automatisés** : Validation à chaque étape
- **Compatibilité standard** : Multiboot, ELF, TAR

### Extensibilité
- **Architecture modulaire** : Ajout facile de fonctionnalités
- **Interfaces standardisées** : Intégration simplifiée
- **Code documenté** : Maintenance facilitée

## Conclusion

La logique du projet MOHHDY suit une approche méthodique et progressive, construisant couche par couche un système d'exploitation moderne et sécurisé. L'architecture modulaire et les choix techniques (Multiboot, ELF, TAR, Ring 0/3) démontrent une compréhension approfondie des systèmes d'exploitation et préparent efficacement le terrain pour l'intégration future d'intelligence artificielle.

Le projet atteint actuellement la version 4.0 avec un système multitâche complet et un espace utilisateur sécurisé, constituant une base solide pour les développements futurs.

