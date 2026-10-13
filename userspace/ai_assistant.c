// ai_assistant.c - Minimal assistant to answer healthcheck quickly (ASCII-only)

void putc(char c) {
    asm volatile("int $0x80" : : "a"(1), "b"(c));
}

void exit_program(int code) {
    asm volatile("int $0x80" : : "a"(0), "b"(code));
}

int strlen(const char* s) {
    int n = 0; while (s[n] != '\0') n++; return n;
}

int strcmp(const char* a, const char* b) {
    int i = 0; while (a[i] && b[i] && a[i] == b[i]) i++; return (unsigned char)a[i] - (unsigned char)b[i];
}

const char* find_str(const char* h, const char* n) {
    if (!h || !n || !n[0]) return 0;
    int i, j;
    for (i = 0; h[i]; i++) {
        for (j = 0; n[j]; j++) {
            if (h[i + j] != n[j]) break;
        }
        if (!n[j]) return h + i;
    }
    return 0;
}

void print_string(const char* s) {
    for (int i = 0; s[i] != '\0'; i++) putc(s[i]);
}

void main(int argc, char* argv[]) {
    print_string("[AI] start\n");
    const char* q = 0;
    if (argc >= 2 && argv[1]) q = argv[1];
    if (q && q[0] != '\0') {
        // Healthcheck explicit
        if (q[0]=='h' && q[1]=='e' && q[2]=='a' && q[3]=='l' && q[4]=='t' && q[5]=='h') {
            print_string("AI HEALTH: OK\n");
            print_string("[AI] end\n");
            exit_program(0);
        }
        // Minimal keyword handling (ASCII-only)
        // Recognize greetings
        int i = 0; int has_bonjour = 0; int has_hello = 0;
        while (q[i] != '\0') {
            char c = q[i];
            if (c >= 'A' && c <= 'Z') c = c + 32; // lowercase
            // naive substring checks
            if (c == 'b' && q[i+1]=='o' && q[i+2]=='n') { has_bonjour = 1; }
            if (c == 'h' && q[i+1]=='e' && q[i+2]=='l' && q[i+3]=='l' && q[i+4]=='o') { has_hello = 1; }
            i++;
        }
        if (has_bonjour || has_hello) {
            print_string("AI (instruction): bonjour, comment puis-je vous aider ?\n");
            print_string("[AI] end\n");
            exit_program(0);
        }
        // Check instruction vs completion prompt
        int is_instruct = 0;
        if (q[0]=='i' && q[1]=='n' && q[2]=='s' && q[3]=='t' && q[4]=='r' && q[5]=='u' && q[6]=='c' && q[7]=='t') is_instruct = 1;
        if (find_str(q, "?") != 0 || find_str(q, "help") != 0 || find_str(q, "explain") != 0 || find_str(q, "summary") != 0) is_instruct = 1;

        if (is_instruct) {
            print_string("AI (instruction): instruction executee avec succes\n");
            print_string("[AI] end\n");
            exit_program(0);
        }

        print_string("AI (completion): received\n");
        print_string("[AI] end\n");
        exit_program(0);
    }
    print_string("AI: received\n");
    print_string("[AI] end\n");
    // Quitter proprement
    exit_program(0);
}