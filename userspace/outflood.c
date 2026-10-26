/* outflood.c - Lot 46 proof program: prints 300 numbered lines (about
 * 21 KB, more than the 16 KiB window capture OS_OUTCAP_MAX) through
 * SYS_PUTS, then a last line. Run from a desktop window (/cmd
 * "spawn outflood") the window shows the head of the output and a note
 * with the number of bytes dropped; the last line never reaches it. */

static void puts_line(const char* s) {
    /* SYS_PUTS adds the newline itself */
    asm volatile("int $0x80" : : "a"(3), "b"(s) : "memory");
}

int main(void) {
    static const char pad[] = " abcdefghijklmnopqrstuvwxyz0123456789abcdefghijklmnop";
    char line[80];
    int i, k, o;
    for (i = 1; i <= 300; i++) {
        o = 0;
        line[o++] = 'o'; line[o++] = 'u'; line[o++] = 't'; line[o++] = 'f';
        line[o++] = 'l'; line[o++] = 'o'; line[o++] = 'o'; line[o++] = 'd';
        line[o++] = ' ';
        line[o++] = (char)('0' + (i / 100) % 10);
        line[o++] = (char)('0' + (i / 10) % 10);
        line[o++] = (char)('0' + i % 10);
        for (k = 0; pad[k] && o < (int)sizeof(line) - 1; k++) line[o++] = pad[k];
        line[o] = 0;
        puts_line(line);
    }
    puts_line("outflood end (300 lines)");
    return 0;
}
