/* userspace/promptmessage.h - Phase 4 PromptMessage (US-046..US-057).
 *
 * A small, line oriented language that mixes plain words and structure:
 *   create file "note.txt" with content "Hello MOHHDY"
 *   when user says "bonjour" then print "salut"
 *   if system.memory < 50% then print "memoire ok"
 * Source is compiled to a compact bytecode (PMC1) and run by a bounded stack
 * VM inside the guest shell. No libc, no allocation: every buffer is static
 * and sized below. The host (shell or unit test) provides file access, the
 * memory percentage and the output sink through pm_host_t.
 */
#ifndef MOHHDY_PROMPTMESSAGE_H
#define MOHHDY_PROMPTMESSAGE_H

#define PM_SRC_MAX 4096
#define PM_CODE_MAX 4096
#define PM_POOL_MAX 2048
#define PM_VARS 16
#define PM_NAME_MAX 16
#define PM_TRIGGERS 8
#define PM_VAL_MAX 192
#define PM_STACK 12
#define PM_STEPS 20000
#define PM_BLOCKS 8
#define PM_INCLUDES 4
#define PM_IMAGE_MAX (16 + PM_TRIGGERS * 4 + PM_VARS * PM_NAME_MAX + PM_POOL_MAX + PM_CODE_MAX + 4)

/* Status codes. */
#define PM_OK 0
#define PM_ERR_SYNTAX -1
#define PM_ERR_LIMIT -2
#define PM_ERR_RUNTIME -3
#define PM_ERR_IMAGE -4
#define PM_ERR_DENIED -5
#define PM_ERR_EXPECT -6
#define PM_BREAK 1

typedef struct {
    void* ctx;
    /* Returns bytes read (>= 0) or < 0. */
    int (*read_file)(void* ctx, const char* path, char* buf, int cap);
    /* append = 0 replaces the file. Returns < 0 on failure. */
    int (*write_file)(void* ctx, const char* path, const char* data, int len, int append);
    int (*mem_used_pct)(void* ctx);
    void (*out)(void* ctx, const char* text);
} pm_host_t;

typedef struct {
    unsigned short phrase; /* pool offset */
    unsigned short addr;
} pm_trigger_t;

typedef struct {
    unsigned char code[PM_CODE_MAX];
    unsigned short code_len;
    char pool[PM_POOL_MAX];
    unsigned short pool_len;
    pm_trigger_t triggers[PM_TRIGGERS];
    unsigned char ntrig;
    char vars[PM_VARS][PM_NAME_MAX];
    unsigned char nvars;
    /* Compile report (not serialized). */
    unsigned short statements;
    unsigned short folded;
    unsigned short includes;
    int err_line;
    int err_col;
    char err[64];
} pm_program_t;

typedef struct {
    int is_str;
    int num;
    char str[PM_VAL_MAX];
} pm_value_t;

typedef struct {
    pm_value_t vars[PM_VARS];
    pm_value_t stack[PM_STACK];
    int sp;
    unsigned int steps;
    unsigned int prints;
    unsigned int writes;
    unsigned int expects;
    unsigned int expect_failed;
    int line;
    int trace;       /* 1: emit "pm-trace line N" at each statement */
    int break_line;  /* > 0: stop before this line and dump variables */
    char err[64];
} pm_vm_t;

/* Compile source text. Returns PM_OK or PM_ERR_SYNTAX / PM_ERR_LIMIT with
 * prog->err_line, err_col and err set. host may be 0 (no `use` includes). */
int pm_compile(const char* src, int len, pm_program_t* prog, const pm_host_t* host);
/* Serialize to the PMC1 image (FNV-1a 32 checksum at the end). Returns size. */
int pm_image_write(const pm_program_t* prog, unsigned char* out, int cap);
/* Load and verify a PMC1 image. Returns PM_OK or PM_ERR_IMAGE. */
int pm_image_read(const unsigned char* in, int len, pm_program_t* prog);
unsigned int pm_checksum(const unsigned char* data, int len);
/* Run the main body (from address 0 to HALT). */
int pm_run(const pm_program_t* prog, pm_vm_t* vm, const pm_host_t* host);
/* Run the trigger whose phrase matches (case-insensitive). Returns PM_OK,
 * an error, or PM_ERR_RUNTIME with vm->err "no trigger". */
int pm_say(const pm_program_t* prog, pm_vm_t* vm, const pm_host_t* host, const char* phrase);
void pm_vm_reset(pm_vm_t* vm);
/* Documentation (US-051): `##` comments, triggers, variables, includes. */
int pm_doc(const char* src, int len, const pm_program_t* prog, char* out, int cap);
/* Disassembler for the debugger view (US-052). */
int pm_disasm(const pm_program_t* prog, char* out, int cap);

#endif
