/* wait_child.c - enfant de preuve pour SYS_TASK_WAIT.
 * La série bornée de yields laisse au parent le temps d’installer une attente
 * sous QEMU ; une fois le parent WAITING, l’enfant reprend immédiatement puis
 * sort normalement via start.s. */

#define WAIT_CHILD_YIELDS 64

void putc(char c) {
    asm volatile("int $0x80" : : "a"(1), "b"(c));
}

void yield(void) {
    asm volatile("int $0x80" : : "a"(4));
}

static void puts_local(const char* text) {
    int i = 0;
    while (text[i]) putc(text[i++]);
}

void main(void) {
    int i;
    puts_local("wait-child ready\n");
    for (i = 0; i < WAIT_CHILD_YIELDS; i++) yield();
    puts_local("wait-child done\n");
}
