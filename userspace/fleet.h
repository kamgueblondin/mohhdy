/* userspace/fleet.h - guest-to-guest services over the P2P app channel:
 * session handoff (US-084), file sync between devices (US-083), metrics
 * and log collection (US-106/US-113 fleet part), staged deploy (US-117).
 * Pure C; transport and files come from the host callbacks. Payloads ride
 * in sealed P2P_I_APP messages (tag bytes 0x40..0x45; collab uses 1..3). */
#ifndef MOHHDY_FLEET_H
#define MOHHDY_FLEET_H
#include <stdint.h>

#define FL_SESSION 0x40
#define FL_SYNC 0x41
#define FL_METRIC 0x42
#define FL_LOG 0x43
#define FL_DEPLOY 0x44
#define FL_ACK 0x45
#define FL_PAYLOAD 384
#define FL_FILES 8
#define FL_NODES 8
#define FL_LOGS 4
#define FL_APPS 4
enum { FL_STAGE_CANARY = 1, FL_STAGE_ALL = 2, FL_STAGE_ROLLBACK = 3 };

typedef struct {
    void* ctx;
    uint32_t self;
    int (*send)(void* ctx, uint32_t to, const uint8_t* d, int len); /* to 0 = all */
    void (*out)(void* ctx, const char* line);
    int (*write_file)(void* ctx, const char* path, const char* d, int len);
    const char* (*name)(void* ctx, uint32_t id);
    uint32_t (*members)(void* ctx);      /* live peers + self */
} fleet_host_t;

typedef struct { char path[40]; uint32_t ver, origin, sum; int used; } fl_file_t;
typedef struct { uint32_t id; uint32_t reports; char metric[96]; char log[FL_LOGS][80]; int nlog; int used; } fl_node_t;
typedef struct { uint32_t id, ver, sum; int ok, used; } fl_ack_t;
typedef struct {
    char name[24];
    uint32_t ver, sum, prev_ver, prev_sum;
    int stage, used;
    char content[300]; int clen;
    char prev[300]; int plen;
    uint32_t canary;
    uint32_t last_send; int resends;
    fl_ack_t ack[FL_NODES];
} fl_deploy_t;
typedef struct { char name[24]; uint32_t ver, sum; int used; } fl_applied_t;

typedef struct {
    /* pending session offered by a peer */
    uint32_t sess_from; int sess_len; char sess[FL_PAYLOAD];
    fl_file_t files[FL_FILES];
    fl_node_t nodes[FL_NODES];
    fl_deploy_t dep[FL_APPS];      /* deployments coordinated here */
    fl_applied_t app[FL_APPS];     /* deployments applied here */
    uint32_t rejected;
} fleet_t;

uint32_t fleet_sum(const char* d, int len);
void fleet_init(fleet_t* f);
void fleet_receive(fleet_t* f, const fleet_host_t* h, uint32_t from, const uint8_t* d, int len);
int fleet_session_send(fleet_t* f, const fleet_host_t* h, uint32_t to, const char* text, int len);
/* push a local file version to every peer; returns the new version or -1 */
int fleet_sync_push(fleet_t* f, const fleet_host_t* h, const char* path, const char* d, int len);
int fleet_report(fleet_t* f, const fleet_host_t* h, uint32_t to, const char* metric, const char* log);
/* coordinator: stage NAME with content to the canary node */
int fleet_deploy_stage(fleet_t* f, const fleet_host_t* h, const char* name, const char* d, int len, uint32_t canary);
/* 0 sent to all, -1 unknown, -2 canary not acknowledged ok */
int fleet_deploy_promote(fleet_t* f, const fleet_host_t* h, const char* name);
int fleet_deploy_rollback(fleet_t* f, const fleet_host_t* h, const char* name);
fl_deploy_t* fleet_deploy_find(fleet_t* f, const char* name);
/* re-send a deployment step to nodes that have not acknowledged it yet
 * (every FL_RESEND_TICKS, at most FL_RESENDS times); returns sends done */
#define FL_RESEND_TICKS 200U
#define FL_RESENDS 3
int fleet_tick(fleet_t* f, const fleet_host_t* h, uint32_t now);
#endif
