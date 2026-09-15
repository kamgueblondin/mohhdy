/* ok.c - ELF minimal pour SYS_EXEC. Affiche exec ok et sort (start.s). */

void putc(char c) {
    asm volatile("int $0x80" : : "a"(1), "b"(c));
}

int main(void) {
    const char* msg = "exec ok\n";
    int i;
    for (i = 0; msg[i]; i++) putc(msg[i]);
    return 0;
}
