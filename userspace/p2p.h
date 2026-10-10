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
#define P2P_DOWN_TICKS (10U * P2P_HZ)
#define P2P_PROPOSE_TICKS (15U * P2P_HZ)

enum {
    P2P_T_HELLO = 1, P2P_T_SEALED = 2,
    /* inner (sealed) types */
    P2P_I_PING = 10, P2P_I_PONG, P2P_I_MSG, P2P_I_PUT, P2P_I_SYNC_REQ, P2P_I_SYNC_ITEMS,
    P2P_I_GET_REQ, P2P_I_GET_RESP, P2P_I_PROPOSE, P2P_I_VOTE, P2P_I_COMMIT, P2P_I_COUNT
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
    uint32_t id;
    char name[P2P_NAME_MAX];
    uint8_t ip[4];
    uint8_t mac[6];
    uint8_t pub[32];
    uint8_t key[16];
    uint8_t iv[4];
    uint32_t tx_counter, rx_counter;
    uint32_t last_seen, ping_sent_at, rtt_ticks;
    uint32_t via;            /* 0: direct, else id of the relay node */
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
} p2p_proposal_t;

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
#endif
