#ifndef MOHHDY_DOSEMU_H
#define MOHHDY_DOSEMU_H
/* Lot 7 (US-086): a real 8086 interpreter for DOS .COM programs, in guest
 * C, no libc. One 64 KiB segment (CS = DS = ES = SS, segment registers
 * read as 0 and writes are ignored), program loaded at 0x100 after a
 * minimal PSP (INT 20h at offset 0), SP = 0xFFFE.
 * Integer subset of the 8086: MOV/XCHG/LEA, ADD/ADC/SUB/SBB/CMP/AND/OR/XOR/
 * TEST, INC/DEC/NEG/NOT, MUL/IMUL/DIV/IDIV, shifts and rotates, PUSH/POP,
 * Jcc/JMP/CALL/RET/LOOP/JCXZ, CBW/CWD, string ops (MOVS/STOS/LODS/CMPS/
 * SCAS with REP), flags CF ZF SF OF DF (PF and AF are not computed).
 * Services: INT 21h AH=02 (print DL), 09 (print $-string at DX),
 * 30 (DOS version 5.0), 4C (exit AL); INT 20h; INT 10h AH=0E (teletype).
 * No BIOS, no VGA mode 13h, no files, no EXE format, no 286+ opcodes:
 * unsupported opcodes or services stop the program with an error. */

#define DOS_EXITED 0
#define DOS_ERR_OPCODE -1
#define DOS_ERR_INT -2
#define DOS_ERR_STEPS -3
#define DOS_ERR_DIV -4
#define DOS_ERR_SIZE -5

typedef struct {
    int status;            /* DOS_EXITED or DOS_ERR_* */
    int exit_code;         /* AL of INT 21h AH=4C */
    unsigned int steps;    /* instructions executed */
    unsigned int ints;     /* service calls */
    unsigned int fault_ip; /* IP of the unsupported instruction */
    int fault_op;          /* its opcode or interrupt number */
    int out_len;
} dos_result_t;

/* Runs `len` bytes as a .COM image. Program output (INT 21h/10h) goes to
 * out (NUL terminated, truncated at cap). max_steps bounds the run. */
int dos_run_com(const unsigned char* image, int len, char* out, int cap, unsigned int max_steps,
                dos_result_t* res);
const char* dos_status_name(int status);

#endif
