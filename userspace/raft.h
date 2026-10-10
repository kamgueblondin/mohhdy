/* userspace/raft.h - US-064 multi-round consensus (Raft-like) over P2P.
 *
 * Leader election with terms and randomized timeouts, log replication
 * with consistency check (prev index/term), commit on a majority of the
 * fixed cluster, and a small replicated key/value state machine. Messages
 * ride on the sealed P2P application channel (tags 0x60-0x63). Pure C,
 * no allocation, host-testable. Not persisted across reboot (documented). */
#ifndef MOHHDY_RAFT_H
#define MOHHDY_RAFT_H
#include <stdint.h>

#define RAFT_MAX_NODES 5
#define RAFT_LOG 32
#define RAFT_KEY 12
#define RAFT_VAL 20
#define RAFT_KV 16
#define RAFT_HEARTBEAT 50U        /* ticks between leader appends */
#define RAFT_ELECTION_MIN 300U    /* ticks; + (id % 200) per node */
#define RAFT_BATCH 4

enum { RAFT_FOLLOWER = 0, RAFT_CANDIDATE, RAFT_LEADER };

typedef struct { uint32_t term; char key[RAFT_KEY]; char val[RAFT_VAL]; } raft_entry_t;
typedef struct { char key[RAFT_KEY]; char val[RAFT_VAL]; uint32_t index; int used; } raft_kv_t;

typedef struct {
    void* ctx;
    int (*send)(void* ctx, uint32_t to, const uint8_t* data, int length);
    void (*out)(void* ctx, const char* line);
    const char* (*name)(void* ctx, uint32_t id);
} raft_host_t;

typedef struct {
    uint32_t self, members[RAFT_MAX_NODES]; int nmembers;   /* members excludes self */
    int role;
    uint32_t term, voted_for, leader;
    raft_entry_t log[RAFT_LOG]; uint32_t nlog;              /* 1-based indices */
    uint32_t commit, applied;
    uint32_t next[RAFT_MAX_NODES], match[RAFT_MAX_NODES];
    uint32_t votes;
    uint32_t last_heard, last_sent, timeout;
    raft_kv_t kv[RAFT_KV];
    uint32_t elections, step_downs, truncated, rejected_appends;
    int on;
} raft_t;

int raft_start(raft_t* r, uint32_t self, const uint32_t* members, int n, uint32_t now);
void raft_tick(raft_t* r, const raft_host_t* h, uint32_t now);
void raft_receive(raft_t* r, const raft_host_t* h, uint32_t from, const uint8_t* d, int len, uint32_t now);
/* leader only: append; returns the index (>0), -1 not leader, -2 log full, -3 bad args */
int raft_propose(raft_t* r, const raft_host_t* h, const char* key, const char* val, uint32_t now);
const char* raft_get(const raft_t* r, const char* key);
uint32_t raft_digest(const raft_t* r); /* FNV-1a over committed entries */
#endif
