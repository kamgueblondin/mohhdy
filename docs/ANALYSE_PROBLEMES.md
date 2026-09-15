# Analyse des Problèmes - MOHHDY Mode Utilisateur

> **État réel (août 2026).** Le kernel n’est plus coincé dans une boucle shell simulée : le programme `userspace/shell.c` s’exécute en Ring 3. Ce fichier reste une analyse utile de l’ancien flux. Voir [ETAT_REEL.md](ETAT_REEL.md).

## Problèmes Identifiés

### 1. Problème Principal: Pas de Vrai Passage au Mode Utilisateur

**Localisation**: `kernel/kernel.c` lignes 377-520

**Description**: 
- Le kernel crée une tâche utilisateur avec `create_user_task()` mais n'effectue jamais le vrai passage au mode utilisateur
- Au lieu de cela, il reste dans le kernel et simule le shell avec une boucle intégrée
- La fonction `schedule()` n'est jamais appelée pour effectuer le changement de contexte

**Code problématique**:
```c
task_t* shell_task = create_user_task(shell_entry);
if (shell_task) {
    print_string("Tache shell creee ! Passage en mode utilisateur...\n");
    // PROBLÈME: Aucun vrai passage au mode utilisateur ici
    // Au lieu de cela, une boucle shell intégrée dans le kernel
    while (1) {
        // Simulation du shell dans le kernel
    }
}
```

### 2. Problème de Gestion Mémoire dans create_user_task()

**Localisation**: `kernel/task/task.c` lignes 133-180

**Description**:
- La tâche utilisateur utilise le même `page_directory` que le kernel
- Cela viole l'isolation mémoire entre kernel et userspace
- Les segments utilisateur sont configurés mais pas utilisés

**Code problématique**:
```c
new_task->page_directory = (uint32_t*)kernel_directory; // PROBLÈME: Même directory que le kernel
```

### 3. Problème dans le Changement de Contexte

**Localisation**: `boot/context_switch.s` lignes 70-80

**Description**:
- Le code de changement de contexte ne gère pas correctement le passage kernel->user
- Les segments utilisateur ne sont pas chargés
- Pas de changement de niveau de privilège (Ring 0 -> Ring 3)

**Code problématique**:
```asm
; Pour la stabilité, on garde les segments kernel pour l'instant
; (les tâches utilisateur seront gérées différemment)
```

### 4. Problème de Chargement ELF

**Localisation**: `kernel/kernel.c` ligne 363

**Description**:
- L'adresse d'entrée du shell est hardcodée à `0x40000000`
- Le fichier ELF n'est pas réellement chargé en mémoire
- Pas de parsing du header ELF pour obtenir la vraie adresse d'entrée

**Code problématique**:
```c
uint32_t shell_entry = 0x40000000; // Adresse simulée
```

### 5. Problème de Scheduler

**Localisation**: `kernel/task/task.c` fonction `schedule()`

**Description**:
- La fonction `schedule()` existe mais n'est jamais appelée
- Pas d'intégration avec le timer pour le multitâche préemptif
- Le scheduler ne gère pas le passage kernel->user

## Analyse des Fichiers Clés

### kernel/kernel.c
- Point d'entrée principal
- Gère l'initialisation mais pas le vrai passage au mode utilisateur
- Contient une simulation de shell au lieu d'un vrai passage

### kernel/task/task.c
- Système de tâches partiellement implémenté
- `create_user_task()` crée la structure mais pas l'isolation mémoire
- `schedule()` non utilisé

### boot/context_switch.s
- Changement de contexte basique
- Ne gère pas les transitions de privilège
- Segments utilisateur non chargés

### userspace/shell.c
- Shell utilisateur complet et fonctionnel
- Prêt à être exécuté en mode utilisateur
- Contient toutes les fonctionnalités nécessaires

## Prochaines Étapes

1. Corriger le chargement ELF réel
2. Implémenter l'isolation mémoire pour les tâches utilisateur
3. Corriger le changement de contexte pour supporter Ring 0 -> Ring 3
4. Intégrer le scheduler avec le timer
5. Effectuer le vrai passage au mode utilisateur



## Résultats des Tests

### Test d'Exécution QEMU

Le système démarre correctement et affiche :
```
Tache utilisateur creee
Tache shell creee ! Passage en mode utilisateur...
Initialisation de l'interface shell...

=== Shell MOHHDY v5.0 Actif ===
Bienvenue dans MOHHDY ! Tapez 'help' pour l'aide.
Shell base sur userspace/shell.c avec IA fake_ai.c

MOHHDY> 
```

**Problème confirmé** : Le message "Passage en mode utilisateur..." est trompeur. Le système reste dans le kernel et utilise une simulation de shell au lieu du vrai shell utilisateur.

### Analyse Détaillée des Problèmes

1. **Pas de vrai passage au mode utilisateur** : Confirmé par les logs
2. **Shell simulé dans le kernel** : Le prompt "MOHHDY>" vient du kernel, pas du userspace
3. **Tâche utilisateur créée mais non utilisée** : La tâche est créée mais jamais exécutée
4. **Pas d'appel au scheduler** : Aucun changement de contexte effectué

### Points de Correction Identifiés

1. **kernel/kernel.c ligne 378** : Remplacer la simulation par un vrai passage au mode utilisateur
2. **kernel/task/task.c** : Corriger l'isolation mémoire des tâches utilisateur
3. **boot/context_switch.s** : Implémenter le passage Ring 0 -> Ring 3
4. **Intégration scheduler** : Appeler schedule() après création de la tâche utilisateur

