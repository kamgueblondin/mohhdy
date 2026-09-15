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
            print_string("AI: bonjour\n");
            print_string("[AI] end\n");
            exit_program(0);
        }
        print_string("AI: received\n");
        print_string("[AI] end\n");
        exit_program(0);
    }
    print_string("AI: received\n");
    print_string("[AI] end\n");
    // Quitter proprement
    exit_program(0);
}