/* idle.c - tache userspace de fond. spawn lui cede le CPU une fois ;
 * ensuite elle boucle sur SYS_YIELD jusqu'a kill. Ne pas la lancer via exec
 * (bloquant). */

void putc(char c) {
    asm volatile("int $0x80" : : "a"(1), "b"(c));
}

void yield(void) {
    asm volatile("int $0x80" : : "a"(4));
}

void main(void) {
    const char* msg = "idle ok\n";
    int i;
    for (i = 0; msg[i]; i++) putc(msg[i]);
    for (;;) {
        yield();
    }
}
