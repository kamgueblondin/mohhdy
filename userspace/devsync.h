/* Lot 8 (US-083): multi-device synchronisation core of the guest.
 * Each device keeps a replica: a last-writer-wins map (Lamport clock,
 * device id as tie break, tombstones for deletes) and a version vector.
 * Sync exchanges text deltas (only entries the peer has not seen),
 * checked by an FNV-1a sum. Merge is commutative, associative and
 * idempotent, so replicas converge whatever the order. */
#ifndef DEVSYNC_H
#define DEVSYNC_H

#include <stdint.h>

#define DS_DEVICES 8
#define DS_KEYS 32
#define DS_KEY_LEN 16
#define DS_VAL_LEN 32

typedef struct {
    char key[DS_KEY_LEN];
    char val[DS_VAL_LEN];
    uint32_t clock;
    uint8_t dev;
    uint8_t dead;
} ds_entry_t;

typedef struct {
    uint8_t id;
    uint32_t clock;
    uint32_t vv[DS_DEVICES];
    int n;
    ds_entry_t e[DS_KEYS];
    uint32_t applied, ignored, conflicts;
} ds_replica_t;

int ds_init(ds_replica_t* r, int id);
int ds_set(ds_replica_t* r, const char* key, const char* val);
int ds_del(ds_replica_t* r, const char* key);
const char* ds_get(const ds_replica_t* r, const char* key);
int ds_live(const ds_replica_t* r);
/* 1 applied, 0 ignored (older or same), -1 invalid or full */
int ds_apply(ds_replica_t* r, const ds_entry_t* e);
/* Delta for a peer whose version vector is peer_vv (NULL = everything). */
int ds_export(const ds_replica_t* r, const uint32_t* peer_vv, char* out, int cap);
/* Returns entries applied, -1 on a bad format, -2 on a bad sum. */
int ds_import(ds_replica_t* r, const char* in);
/* Order independent digest of the live map (convergence check). */
uint32_t ds_digest(const ds_replica_t* r);

#endif
