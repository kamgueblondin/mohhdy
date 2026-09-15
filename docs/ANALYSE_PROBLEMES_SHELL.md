# Analyse des Problèmes - Shell Utilisateur MOHHDY

> **État réel (août 2026).** L’interface n’est plus figée au passage userspace ; le shell ELF démarre. Contenu historique conservé. Voir [ETAT_REEL.md](ETAT_REEL.md).

## Problème Principal Identifié

**Symptôme**: L'interface reste figée lors du passage au mode utilisateur et le système redémarre en boucle.

**Localisation**: Le problème survient après "Transfert vers l'espace utilisateur..." dans les logs.

## Analyse Détaillée

### 1. Problème de Transition Kernel → Userspace

**Code problématique dans kernel.c (lignes 370-405)**:
- Le système crée correctement la tâche shell avec `create_task_from_initrd_file("shell")`
- Mais au lieu de faire la transition vers l'espace utilisateur, il reste en mode simulation
- Le commentaire indique: "Pour éviter les crashes, on simule le shell dans le kernel"

### 2. Gestion des Tâches Incomplète

**Dans task.c**:
- La fonction `create_task_from_initrd_file()` charge correctement l'ELF
- Mais la transition vers l'exécution de la tâche utilisateur n'est pas implémentée
- Le scheduler existe mais n'est pas utilisé pour passer au shell utilisateur

### 3. Problèmes dans les Appels Système

**Dans syscall.c**:
- `sys_gets()` utilise `hlt` pour attendre les entrées, ce qui peut causer des blocages
- La gestion des interruptions clavier n'est pas synchronisée avec le shell utilisateur
- Le buffer d'entrée est géré au niveau kernel mais pas correctement exposé au userspace

### 4. Configuration du Timer

**Problème de timing**:
- Le timer est désactivé pour la stabilité mais réactivé après la création du shell
- Cela peut causer des conflits d'interruptions lors de la transition

## Problèmes Spécifiques Identifiés

### A. Absence de Context Switch vers Userspace
```c
// Dans kernel.c - Le code reste en simulation
print_string("Mode simulation shell (pour stabilite)...");
// Au lieu de faire: schedule_to_user_task(shell_task);
```

### B. Gestion des Interruptions Clavier
```c
// Dans syscall.c - sys_gets() peut bloquer indéfiniment
while (!line_ready) {
    asm volatile("hlt");  // Problématique si pas d'interruptions
}
```

### C. Configuration Mémoire Userspace
- Le shell est chargé à l'adresse 0x40000000
- Mais la transition vers cette adresse n'est pas effectuée
- Le système reste en mode kernel

## Solutions Proposées

### 1. Implémenter la Transition Userspace
- Ajouter une fonction `switch_to_userspace()` 
- Configurer correctement les registres de segment
- Effectuer un `iret` vers l'espace utilisateur

### 2. Corriger la Gestion des Interruptions
- Synchroniser les interruptions clavier avec le shell utilisateur
- Améliorer `sys_gets()` pour éviter les blocages
- Gérer correctement les timeouts

### 3. Refactoriser le Code de Lancement
- Supprimer le mode simulation
- Implémenter le vrai passage au shell utilisateur
- Ajouter des vérifications de sécurité

### 4. Améliorer le Scheduler
- Utiliser le scheduler existant pour la transition
- Gérer correctement les changements de contexte
- Ajouter des logs de debug pour le suivi

## Prochaines Étapes

1. Tester le système actuel avec `make run`
2. Implémenter les corrections identifiées
3. Tester progressivement chaque correction
4. Valider le fonctionnement complet du shell utilisateur

