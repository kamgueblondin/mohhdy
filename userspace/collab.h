/* userspace/collab.h - Phase 7 collaborative layer (US-091..US-105).
 *
 * A replicated, append-only ledger of entries shared by the members of a
 * P2P network (phase 5 transport). Every node folds the same entry set in
 * the same canonical order (lamport, origin, seq), so balances, bookings,
 * tasks, reputation, votes and profiles converge; invalid entries are
 * rejected identically everywhere. Points are local accounting units, not
 * money. Transport trust is membership-level (network key + per-pair AEAD);
 * with collab_keys_set() every entry is also signed by its origin (Schnorr,
 * userspace/csig.c) and verified on receipt, so a member can no longer forge
 * another member's entry. Pure C, no allocation.
 */
#ifndef MOHHDY_COLLAB_H
#define MOHHDY_COLLAB_H
#include <stdint.h>

#define COLLAB_ENTRIES 96
#define COLLAB_TEXT 40
#define COLLAB_NODES 8
#define COLLAB_FIELDS 6
#define COLLAB_GRANT 100U
#define COLLAB_WIRE 72

enum {
    CE_JOIN = 1, CE_TRANSFER, CE_OFFER, CE_RESERVE, CE_TASK, CE_CLAIM, CE_DONE, CE_ACCEPT, CE_REJECT,
    CE_RATE, CE_PROPOSE, CE_VOTE, CE_PROFILE, CE_REDACT, CE_TICKET, CE_ANSWER, CE_COUNT
};

typedef struct {
    uint8_t type;
    uint32_t origin, seq, lamport;
    uint32_t peer;     /* counterparty node id */
    uint32_t ref;      /* seq of the referenced entry of `peer` (or of origin) */
    uint32_t amount;
    uint32_t aux;
    char text[COLLAB_TEXT];
} collab_entry_t;

typedef struct {
    void* ctx;
    /* broadcast an encoded message to every member (returns peers reached) */
    int (*send)(void* ctx, uint32_t to, const uint8_t* data, int length);
    void (*out)(void* ctx, const char* line);
    const char* (*name)(void* ctx, uint32_t id);
    uint32_t (*members)(void* ctx);
} collab_host_t;

typedef struct { char key[12]; char value[24]; uint8_t shared; } collab_field_t;

typedef struct {
    uint32_t self;
    uint32_t seq, lamport;
    collab_entry_t e[COLLAB_ENTRIES];
    uint8_t status[COLLAB_ENTRIES]; /* 0 applied, else reject reason (after fold) */
    int n;
    uint32_t dropped, duplicates;
    collab_field_t profile[COLLAB_FIELDS]; /* local personal data (US-102) */
    /* per-node signatures */
    uint8_t signing;                         /* own key set: sign, require signed input */
    uint8_t sk[20], pk[128];
    uint8_t sig[COLLAB_ENTRIES][40];
    uint8_t th[COLLAB_ENTRIES][32];          /* SHA-256 of the original text (redaction-proof) */
    uint8_t has_sig[COLLAB_ENTRIES];
    uint32_t key_origin[COLLAB_NODES];
    uint8_t key_pk[COLLAB_NODES][128];
    int nkeys;
    uint32_t forged;                         /* entries rejected: unsigned, bad signature, key mismatch */
} collab_t;

/* derived state after a canonical fold */
typedef struct {
    uint32_t id; int32_t balance; uint32_t rating_sum, ratings; int joined;
    char shared[COLLAB_TEXT * 2];
} collab_account_t;
typedef struct {
    collab_account_t acc[COLLAB_NODES]; int nacc;
    uint32_t applied, rejected;
    uint8_t digest[32];
} collab_state_t;

void collab_init(collab_t* c, uint32_t self);
/* Local action: builds, stores and broadcasts one entry. Returns its seq or
 * a negative error (-1 bad args, -2 ledger full). Validity is decided by the
 * fold, the same way on every node. */
int collab_emit(collab_t* c, const collab_host_t* h, uint8_t type, uint32_t peer, uint32_t ref,
                uint32_t amount, uint32_t aux, const char* text);
/* Incoming message from the transport. */
void collab_receive(collab_t* c, const collab_host_t* h, uint32_t from, const uint8_t* data, int length);
/* Anti-entropy: ask every member for entries newer than ours. */
int collab_sync(collab_t* c, const collab_host_t* h);
void collab_fold(collab_t* c, collab_state_t* s);
const collab_account_t* collab_account(const collab_state_t* s, uint32_t id);
const char* collab_type_name(int type);
const char* collab_reason(int code);
/* deterministic task functions: "sum N", "primes N", "fnv TEXT" */
int collab_task_eval(const char* spec, uint32_t* result);
int collab_encode(const collab_entry_t* e, uint8_t* out);
int collab_decode(const uint8_t* in, int length, collab_entry_t* e);
/* report helpers */
int collab_report(collab_t* c, const collab_host_t* h, const char* what, uint32_t arg, char* out, int cap);
/* personal data (US-102/103/104) */
int collab_profile_set(collab_t* c, const char* key, const char* value, int shared);
int collab_forget(collab_t* c, const collab_host_t* h);
/* per-node keys: derive the keypair from a 32-byte seed, sign from now on */
int collab_keys_set(collab_t* c, const uint8_t seed[32]);
/* fingerprint (first 8 bytes of SHA-256 of the public key) as 16 hex chars */
void collab_key_fpr(const uint8_t pk[128], char out[17]);
const uint8_t* collab_key_of(const collab_t* c, uint32_t origin);
/* test hook (US-093 contract): broadcast a TRANSFER claiming `victim` as origin.
 * mode 0: signed with our key but carrying the victim's public key,
 * mode 1: carrying our own key, mode 2: unsigned. Not stored locally. */
int collab_forge(collab_t* c, const collab_host_t* h, uint32_t victim, uint32_t amount, int mode);
/* persistence: serialize / restore the whole ledger, keys included */
int collab_save(const collab_t* c, uint8_t* out, int cap);
int collab_load(collab_t* c, const uint8_t* in, int len);
#endif
