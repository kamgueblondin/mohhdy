/* userspace/p2p.h - Phase 5 P2P node (US-061..US-075) for the guest shell.
 *
 * Datagrams travel as Ethernet broadcast UDP frames on the shared segment
 * (SYS_PEER_DATA OS_PEER_P2P_*, Ring 3 networker). Membership is a network
 * key (PSK): HELLO frames carry an HMAC-SHA256 tag under it. Each pair of
 * nodes derives an AES-128-GCM key from X25519 + SHA-256 (the guest TLS
 * primitives, not a TLS session). Pure C, no allocation; the host provides
 * the datagram I/O, the clock (100 Hz ticks) and the output sink.
 */
#ifndef MOHHDY_P2P_H
#define MOHHDY_P2P_H
#include <stdint.h>
#include "../kernel/x25519.h"

#define P2P_PORT 7700U
#define P2P_PEERS 6
#define P2P_NAME_MAX 12
#define P2P_KV 16
#define P2P_KEY_MAX 20
#define P2P_VAL_MAX 48
#define P2P_TEXT_MAX 96
#define P2P_SEEN 32
#define P2P_DATAGRAM_MAX 600
#define P2P_HZ 100U
#define P2P_HELLO_TICKS (2U * P2P_HZ)
#define P2P_PING_TICKS (3U * P2P_HZ)
/* longest relay chain for hellos and sealed frames (header hop byte) */
#define P2P_HOPS 3
#define P2P_STALL_TICKS 300U
#define P2P_KX_BUDGET 48      /* ~11 slices per peer, each well under 0.5 s on CI */
#define P2P_DOWN_TICKS (10U * P2P_HZ)
#define P2P_PROPOSE_TICKS (15U * P2P_HZ)

enum {
    P2P_T_HELLO = 1, P2P_T_SEALED = 2,
    /* inner (sealed) types */
    P2P_I_PING = 10, P2P_I_PONG, P2P_I_MSG, P2P_I_PUT, P2P_I_SYNC_REQ, P2P_I_SYNC_ITEMS,
    P2P_I_GET_REQ, P2P_I_GET_RESP, P2P_I_PROPOSE, P2P_I_VOTE, P2P_I_COMMIT, P2P_I_APP, P2P_I_COUNT
};

typedef struct {
    void* ctx;
    int (*send)(void* ctx, const uint8_t dst_ip[4], const uint8_t* data, uint16_t length);
    /* Returns the datagram length (> 0) or 0 when nothing is pending. */
    int (*recv)(void* ctx, uint8_t src_ip[4], uint8_t src_mac[6], uint8_t* data, uint16_t capacity);
    uint32_t (*ticks)(void* ctx);
    void (*out)(void* ctx, const char* line);
} p2p_host_t;

typedef struct {
    uint8_t used, up, blocked, keyed;
    uint8_t kx;              /* key agreement pending (step-wise X25519) */
    uint32_t id;
    char name[P2P_NAME_MAX];
    uint8_t ip[4];
    uint8_t mac[6];
    uint8_t pub[32];
    uint8_t key[16];
    uint8_t iv[4];
    uint32_t tx_counter, rx_counter;
    uint32_t last_seen, ping_sent_at, rtt_ticks;
    uint32_t sync_at;      /* last SYNC_REQ sent by p2p_sync */
    uint8_t sync_retries;  /* re-sends left until SYNC_ITEMS arrives */
    uint32_t via;            /* 0: direct, else id of the relay node */
    /* US-063 multi-hop: hellos relayed across partial links (hop count in
     * the header); routes are shortest paths over the neighbor lists. */
    uint32_t last_heard;     /* last hello, direct or relayed */
    uint32_t reach_said;     /* announced route: next hop id ^ hops (0: none) */
    uint32_t neighbors[P2P_PEERS];
    uint32_t sent, received, auth_fail, replay, throttled, down_events;
    uint32_t bucket_tick, bucket_used;
} p2p_peer_t;

typedef struct {
    uint8_t used;
    char key[P2P_KEY_MAX];
    char value[P2P_VAL_MAX];
    uint32_t version;
    uint32_t origin;
} p2p_item_t;

typedef struct {
    uint8_t active;
    uint32_t round, started;
    char key[P2P_KEY_MAX];
    char value[P2P_VAL_MAX];
    uint32_t version;
    uint32_t yes, no, needed, members;
    uint32_t voters[P2P_PEERS + 1];
    int result; /* 0 pending, 1 committed, -1 rejected, -2 timeout */
    uint32_t last_tx; /* datagrams are not retransmitted by the link: the */
    uint32_t resent;  /* proposer re-sends to peers that have not voted yet */
    uint8_t commit_resends; /* COMMIT is repeated: a lost one left a peer behind */
} p2p_proposal_t;
#define P2P_PROPOSE_RESEND_TICKS (2U * P2P_HZ)
#define P2P_SYNC_RESEND_TICKS (2U * P2P_HZ)

typedef struct {
    uint8_t up;
    char name[P2P_NAME_MAX];
    uint8_t ip[4];
    uint32_t id;
    uint8_t secret[32];
    uint8_t pub[32];
    uint8_t netkey[32];
    uint32_t msg_id;
    uint32_t last_hello, last_ping;
    uint32_t rate_limit; /* sealed messages per second and peer, 0 = none */
    p2p_peer_t peers[P2P_PEERS];
    p2p_item_t kv[P2P_KV];
    p2p_proposal_t prop;
    struct { uint32_t src, id; } seen[P2P_SEEN];
    uint32_t seen_next;
    /* traffic analysis (US-075) */
    uint32_t tx_type[P2P_I_COUNT], rx_type[P2P_I_COUNT];
    uint32_t tx_bytes, rx_bytes, relayed, bad_hello, foreign, dup;
    char last_msg[P2P_TEXT_MAX];
    /* Application payloads (Phase 7 collab) carried in sealed P2P_I_APP. */
    void (*app)(void* app_ctx, uint32_t from_id, const uint8_t* data, int length);
    void* app_ctx;
    /* Step-wise key agreement: one peer at a time, P2P_KX_BUDGET ladder
     * steps per p2p_tick, so discovery never stalls the caller's loop. */
    x25519_job_t kx_job;
    int kx_peer;             /* index in peers[] or -1 */
    uint32_t kx_done;
    /* Pump starvation monitor: a gap over P2P_STALL_TICKS between two
     * p2p_tick calls is reported ("p2p stall N ticks after HINT"). */
    uint32_t last_pump, stalls, worst_gap;
    const char* stall_hint;  /* set by the host: what ran before the gap */
    uint32_t hello_relayed;
} p2p_node_t;

int p2p_up(p2p_node_t* n, const char* name, const uint8_t ip[4], const char* netkey,
           const uint8_t seed[32], const p2p_host_t* h);
void p2p_down(p2p_node_t* n);
/* One pump: receive up to `budget` datagrams, then timers. */
void p2p_tick(p2p_node_t* n, const p2p_host_t* h, int budget);
p2p_peer_t* p2p_find(p2p_node_t* n, const char* name);
int p2p_send_text(p2p_node_t* n, const p2p_host_t* h, const char* peer, const char* text);
int p2p_put(p2p_node_t* n, const p2p_host_t* h, const char* key, const char* value);
const p2p_item_t* p2p_get_local(const p2p_node_t* n, const char* key);
int p2p_get_remote(p2p_node_t* n, const p2p_host_t* h, const char* key);
int p2p_sync(p2p_node_t* n, const p2p_host_t* h, const char* peer);
int p2p_propose(p2p_node_t* n, const p2p_host_t* h, const char* key, const char* value);
int p2p_block(p2p_node_t* n, const char* peer, int blocked);
/* Report lines into out (peers, health, stats, kv). */
int p2p_report(const p2p_node_t* n, const p2p_host_t* h, const char* what, char* out, int cap);
const char* p2p_type_name(int type);
/* Sealed application payload to one peer id, or to every keyed peer (0).
 * Returns the number of peers it was sent to. */
int p2p_send_app(p2p_node_t* n, const p2p_host_t* h, uint32_t peer_id, const uint8_t* data, int length);
const char* p2p_peer_name(const p2p_node_t* n, uint32_t id);
uint32_t p2p_member_count(const p2p_node_t* n);
/* Route to a peer: next hop id (0 direct, 0xFFFFFFFF none) and hop count. */
uint32_t p2p_route(p2p_node_t* n, uint32_t dst, int* hops);
#endif
