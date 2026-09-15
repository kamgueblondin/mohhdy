// Ce programme n'a accès à AUCUNE fonction du noyau directement.
// Il ne peut communiquer que via les appels système.

// Fonction "wrapper" pour l'appel système putc
void putc(char c) {
    // Syscall 1 = putc
    asm volatile("int $0x80" : : "a"(1), "b"(c));
}

// Fonction "wrapper" pour l'appel système puts
void puts(const char* str) {
    // Syscall 3 = puts
    asm volatile("int $0x80" : : "a"(3), "b"(str));
}

// Fonction "wrapper" pour l'appel système yield
void yield() {
    // Syscall 4 = yield
    asm volatile("int $0x80" : : "a"(4));
}

// Fonction "wrapper" pour l'appel système exit
void exit(int code) {
    // Syscall 0 = exit
    asm volatile("int $0x80" : : "a"(0), "b"(code));
}

// Fonction utilitaire pour calculer la longueur d'une chaîne
int strlen(const char* str) {
    int len = 0;
    while (str[len] != '\0') {
        len++;
    }
    return len;
}

// Point d'entrée du programme utilisateur
void main() {
    // Message de bienvenue
    puts("=== Programme Utilisateur MOHHDY ===\n");
    puts("Execution en Ring 3 (espace utilisateur)\n");
    puts("Communication via appels systeme uniquement\n\n");
    
    // Test des différents appels système
    puts("Test 1: Affichage caractere par caractere\n");
    char* msg1 = "H-e-l-l-o- -W-o-r-l-d-!";
    for (int i = 0; msg1[i] != '\0'; i++) {
        if (msg1[i] != '-') {
            putc(msg1[i]);
        }
        
        // Petit délai et yield pour montrer le multitâche
        for (volatile int j = 0; j < 100000; j++);
        yield();
    }
    putc('\n');
    
    puts("\nTest 2: Affichage de chaines completes\n");
    puts("Ligne 1: Ceci est un test\n");
    puts("Ligne 2: Depuis l'espace utilisateur\n");
    puts("Ligne 3: Avec protection memoire\n");
    
    puts("\nTest 3: Boucle avec yield\n");
    for (int i = 0; i < 10; i++) {
        puts("Iteration ");
        
        // Conversion simple d'entier en caractère
        putc('0' + i);
        puts(" - Cede le CPU\n");
        
        yield(); // Cède le CPU à d'autres tâches
        
        // Petit délai
        for (volatile int j = 0; j < 500000; j++);
    }
    
    puts("\nTest 4: Calcul intensif avec yields\n");
    int result = 0;
    for (int i = 0; i < 1000; i++) {
        result += i * i;
        
        // Yield périodiquement pour ne pas monopoliser le CPU
        if (i % 100 == 0) {
            puts("Calcul en cours...\n");
            yield();
        }
    }
    
    puts("Calcul termine!\n");
    
    puts("\n=== Fin du Programme Utilisateur ===\n");
    puts("Retour au noyau avec code de sortie 42\n");
    
    // Termine le programme avec un code de sortie
    exit(42);
    
    // Cette ligne ne devrait jamais être atteinte
    puts("ERREUR: Code apres exit() execute!\n");
}

