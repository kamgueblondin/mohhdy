// fake_ai.c - Simulateur d'Intelligence Artificielle pour MOHHDY
// Ce programme simule un moteur d'IA en analysant les mots-cles
// et en retournant des reponses preprogrammees.

// Wrappers pour les appels systeme
void putc(char c) { 
    asm volatile("int $0x80" : : "a"(1), "b"(c)); 
}

void exit_program() { 
    asm volatile("int $0x80" : : "a"(0)); 
}

// Fonctions utilitaires C basiques (pas de libc disponible)
int strlen(const char* str) {
    int len = 0;
    while (str[len] != '\0') len++;
    return len;
}

int strcmp(const char* s1, const char* s2) {
    int i = 0;
    while (s1[i] != '\0' && s2[i] != '\0') {
        if (s1[i] != s2[i]) {
            return s1[i] - s2[i];
        }
        i++;
    }
    return s1[i] - s2[i];
}

char* strstr(const char* haystack, const char* needle) {
    if (*needle == '\0') return (char*)haystack;
    
    for (int i = 0; haystack[i] != '\0'; i++) {
        int j = 0;
        while (haystack[i + j] == needle[j] && needle[j] != '\0') {
            j++;
        }
        if (needle[j] == '\0') {
            return (char*)&haystack[i];
        }
    }
    return 0; // NULL
}

// Convertir en minuscules pour la comparaison
void to_lowercase(char* str) {
    for (int i = 0; str[i] != '\0'; i++) {
        if (str[i] >= 'A' && str[i] <= 'Z') {
            str[i] = str[i] + 32;
        }
    }
}

// Fonction pour afficher une chaine de caracteres
void print_string(const char* str) {
    for (int i = 0; str[i] != '\0'; i++) {
        putc(str[i]);
    }
}

// Logique principale du simulateur d'IA
void process_query(const char* query) {
    // Créer une copie modifiable pour la conversion en minuscules
    char query_lower[256];
    int i = 0;
    while (query[i] != '\0' && i < 255) {
        query_lower[i] = query[i];
        i++;
    }
    query_lower[i] = '\0';
    to_lowercase(query_lower);
    
    // Healthcheck explicite
    if (strcmp(query_lower, "healthcheck") == 0) {
        print_string("AI HEALTH: OK\n");
        return;
    }
    
    // Analyse des mots-cles et generation de reponses (ASCII only)
    if (strstr(query_lower, "bonjour") || strstr(query_lower, "salut") || strstr(query_lower, "hello")) {
        print_string("Bonjour ! Je suis l'IA de MOHHDY. Comment puis-je vous aider aujourd'hui ?\n");
    }
    else if (strstr(query_lower, "heure") || strstr(query_lower, "temps")) {
        print_string("Il est l'heure de developper un systeme d'exploitation revolutionnaire !\n");
    }
    else if (strstr(query_lower, "aide") || strstr(query_lower, "help") || strcmp(query_lower, "?") == 0) {
        print_string("Commandes disponibles :\n");
        print_string("- 'bonjour' : Salutation\n");
        print_string("- 'heure' : Information sur l'heure\n");
        print_string("- 'nom' : Mon identite\n");
        print_string("- 'calcul' : Operations mathematiques\n");
        print_string("- 'systeme' : Informations sur MOHHDY\n");
        print_string("- 'aide' : Cette aide\n");
    }
    else if (strstr(query_lower, "nom") || strstr(query_lower, "qui es-tu") || strstr(query_lower, "identite")) {
        print_string("Je suis MOHHDY Assistant, l'intelligence artificielle integree de votre systeme d'exploitation.\n");
        print_string("Je suis actuellement en mode simulation pour tester l'infrastructure.\n");
    }
    else if (strstr(query_lower, "calcul") || strstr(query_lower, "mathematique") || strstr(query_lower, "2+2")) {
        if (strstr(query_lower, "2+2")) {
            print_string("2 + 2 = 4\n");
        } else {
            print_string("Je peux effectuer des calculs simples. Essayez '2+2' par exemple.\n");
        }
    }
    else if (strstr(query_lower, "systeme") || strstr(query_lower, "os") || strstr(query_lower, "mohhdy")) {
        print_string("MOHHDY est un systeme d'exploitation experimental concu pour l'IA.\n");
        print_string("Version actuelle : 6.0 avec shell interactif et simulation IA.\n");
        print_string("Fonctionnalites : multitache, gestion memoire, systeme de fichiers.\n");
    }
    else if (strstr(query_lower, "merci") || strstr(query_lower, "thank")) {
        print_string("De rien ! C'est un plaisir de vous assister dans MOHHDY.\n");
    }
    else if (strstr(query_lower, "au revoir") || strstr(query_lower, "bye") || strstr(query_lower, "quit")) {
        print_string("Au revoir ! Merci d'avoir utilise MOHHDY Assistant.\n");
    }
    else if (strlen(query_lower) == 0) {
        print_string("Veuillez poser une question ou tapez 'aide' pour voir les commandes disponibles.\n");
    }
    else {
        print_string("Je ne comprends pas encore cette question.\n");
        print_string("Mon intelligence artificielle est en cours de developpement.\n");
        print_string("Tapez 'aide' pour voir les commandes que je connais.\n");
    }
}

// Point d'entree principal
// Note: argc et argv seront fournis par le noyau via SYS_EXEC
void main(int argc, char* argv[]) {
    if (argc < 2) {
        // Pas d'arguments: repondre directement pour tester l'executable
        print_string("AI: received\n");
        exit_program();
    } else {
        // Traiter la question fournie en argument (argv non garanti par le noyau)
        process_query(argv[1]);
    }
    
    // Terminer proprement
    exit_program();
}

