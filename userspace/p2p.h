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
/* Distance vector in HELLO: a destination is advertised up to this many
 * hops away. The frame TTL (header byte 5) starts at the same value. */
#define P2P_MAX_HOPS 3U
#define P2P_ANALYZE_WARMUP 5U
#define P2P_PING_TICKS (3U * P2P_HZ)
#define P2P_DOWN_TICKS (10U * P2P_HZ)
#define P2P_PROPOSE_TICKS (15U * P2P_HZ)

enum {
    P2P_T_HELLO = 1, P2P_T_SEALED = 2,
    /* inner (sealed) types */
    P2P_I_PING = 10, P2P_I_PONG, P2P_I_MSG, P2P_I_PUT, P2P_I_SYNC_REQ, P2P_I_SYNC_ITEMS,
    P2P_I_GET_REQ, P2P_I_GET_RESP, P2P_I_PROPOSE, P2P_I_VOTE, P2P_I_COMMIT, P2P_I_APP,
    P2P_I_RAFT_VOTE_REQ, P2P_I_RAFT_VOTE_RESP, P2P_I_RAFT_HEARTBEAT, P2P_I_CACHE_INV,
    P2P_I_GRPC_REQ, P2P_I_GRPC_RESP, P2P_I_STUN_REQ, P2P_I_STUN_RESP, P2P_I_NAT_PUNCH,
    P2P_I_RAFT_APPEND, P2P_I_RAFT_APPEND_RESP,
    P2P_I_RAFT_PREVOTE_REQ, P2P_I_RAFT_PREVOTE_RESP,
    P2P_I_RAFT_SNAPSHOT, P2P_I_RAFT_SNAPSHOT_RESP, P2P_I_CACHE_INV_ACK, P2P_I_COUNT
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
    uint32_t tx_counter, rx_counter, rx_window;
    uint32_t last_seen, ping_sent_at, rtt_ticks;
    uint32_t rtt_ewma_ms;  /* EWMA sliding window smoothed RTT in ms */
    uint8_t ping_wait;     /* 0 none, 1 PING outstanding, 2 outstanding and loss counted */
    uint32_t sync_at;      /* last SYNC_REQ sent by p2p_sync */
    uint8_t sync_retries;  /* re-sends left until SYNC_ITEMS arrives */
    uint32_t via;            /* 0: direct, else id of the relay node */
    uint32_t neighbors[P2P_PEERS];
    uint8_t nb_hops[P2P_PEERS];  /* hops from that neighbor to neighbors[i] (1..P2P_MAX_HOPS) */
    uint32_t sent, received, auth_fail, replay, throttled, down_events;
    uint32_t bucket_tick, bucket_used;
    uint32_t rx_win_last, rx_base16; /* Lot 6 (US-075): per-peer frames/s baseline */
    uint32_t anomalies;              /* windows above 4x that baseline + 8 */
    uint8_t loss_percent;            /* US-063: estimated packet loss percentage (0..100) */
    uint32_t link_cost;              /* US-063: dynamic link cost metric derived from RTT & loss */
    uint32_t raft_next;   /* next log index to send (1-based, 0 = unset) */
    uint32_t raft_match;  /* highest index known replicated */
    uint32_t raft_commit_sent; /* leader_commit last sent to this peer */
    uint32_t raft_snap_gen; /* snapshot_index this peer is receiving */
    uint8_t raft_snap_off;  /* next kv item index to send */
    /* Lot 29: US-064 Dynamic Re-keying & US-075 Zero-Trust */
    uint32_t key_gen;
    uint32_t pkts_sent_with_key;
    uint32_t key_created_tick;
    uint32_t rekey_interval_ticks;
    uint32_t rekey_pkt_limit;
    uint64_t last_seen_seq;
    uint8_t zero_trust_revoked;
    char revoke_reason[64];
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

typedef enum {
    P2P_RAFT_FOLLOWER = 0, P2P_RAFT_CANDIDATE = 1, P2P_RAFT_LEADER = 2,
    P2P_RAFT_PRE_CANDIDATE = 3
} p2p_raft_role_t;

/* Physical window. Logical indexes continue past this after compaction.
 * One uncompacted burst still fits 16 entries. A follower whose match is
 * behind snapshot_index receives InstallSnapshot (the kv image taken at
 * compact time), then ordinary appends. */
#define P2P_RAFT_LOG 16
#define P2P_RAFT_ELECTION_TICKS (30U * P2P_HZ)
/* Followers that heard the leader refuse votes and campaigns until this
 * window ends. Shorter than the election timeout. */
#define P2P_RAFT_LEASE_TICKS (10U * P2P_HZ)
#define P2P_QOS_WINDOW_MAX 16U
typedef struct {
    uint32_t term;
    char key[P2P_KEY_MAX];
    char value[P2P_VAL_MAX];
} p2p_raft_entry_t;

typedef struct {
    uint8_t active;           /* 1 if Raft consensus mode is active */
    uint8_t role;             /* follower, candidate, leader, or pre-candidate */
    uint32_t term;            /* current Raft term */
    uint32_t voted_for;       /* candidate ID voted for in current term (0 if none) */
    uint32_t leader_id;       /* current leader ID */
    uint32_t votes_granted;   /* votes received in current election */
    uint32_t election_tick;   /* election timeout tick */
    uint32_t heartbeat_tick;  /* next heartbeat tick if leader */
    uint32_t term_changes;    /* term change counter */
    p2p_raft_entry_t log[P2P_RAFT_LOG];
    uint32_t log_count;       /* highest logical index, 1-based, 0 = empty */
    uint32_t commit_index;    /* highest committed index, 0 = none */
    uint32_t last_applied;    /* highest applied index */
    uint32_t snapshot_index;  /* discarded applied prefix, 0 = none */
    uint32_t snapshot_term;   /* term of snapshot_index */
    uint32_t lease_until;     /* ticks: votes and campaigns wait until then */
    uint8_t snap_kv_n;        /* kv image captured at the last compact */
    uint8_t snap_in_n;        /* snapshot chunks received so far */
    uint32_t snap_in_index;   /* snapshot_index those chunks belong to */
    p2p_item_t snap_kv[P2P_KV];
    p2p_item_t snap_in[P2P_KV];
} p2p_raft_state_t;

#define P2P_CACHE_MAX 8
/* Lot 6 (US-070): a line written through Raft stays until replaced (about 124 days at 100 Hz). */
#define P2P_CACHE_RAFT_TTL 0x40000000U
typedef struct {
    uint8_t used;
    char key[P2P_KEY_MAX];
    char value[P2P_VAL_MAX];
    uint32_t expire_tick;
    uint32_t hits;
    uint32_t version;       /* version of the value cached (0: unversioned) */
} p2p_cache_entry_t;

/* Invalidation fence: a fill at or below this version is refused. */
#define P2P_CACHE_FENCES 8
/* Invalidation re-sent to peers that have not acked, every second, at
 * most this many times. */
#define P2P_CACHE_INV_RETRIES 5
typedef struct {
    p2p_cache_entry_t entries[P2P_CACHE_MAX];
    uint32_t total_hits;
    uint32_t total_misses;
    uint32_t invalidations;
    struct { char key[P2P_KEY_MAX]; uint32_t version; } fence[P2P_CACHE_FENCES];
    uint32_t fence_next;
    uint32_t stale_refused;  /* fills refused by a fence */
    /* Outstanding invalidation (one at a time). */
    char inv_key[P2P_KEY_MAX];
    uint32_t inv_version;
    uint32_t inv_waiting[P2P_PEERS];  /* peer ids that have not acked */
    uint32_t inv_tick;
    uint8_t inv_retries, inv_active;
    uint32_t inv_acked, inv_resent, inv_completed;
} p2p_cache_t;

typedef struct {
    uint32_t window_size;     /* dynamic congestion window (1..16) */
    uint32_t rtt_avg_ms;      /* smoothed RTT in ms (EWMA 1/8 of PONG samples) */
    uint32_t rtt_min_ms;      /* lowest RTT sample seen (0: none yet) */
    uint32_t delay_backoffs;  /* window cut by 1/4 on a queueing delay sample */
    uint32_t ping_losses;     /* PINGs never answered before the next round */
    uint32_t rate_est_bps;    /* window * datagram / srtt, bytes per second */
    uint32_t loss_count;      /* total detected losses / retransmissions */
    uint32_t priority_high_tx;/* high-priority control frames sent */
    uint32_t priority_low_tx; /* low-priority data frames sent */
    /* Lot 6: shaping instead of policing. A data message above the budget
     * waits in a small queue drained by p2p_tick when tokens come back. */
    uint32_t shaped_queued, shaped_sent, shaped_dropped;
    /* Lot 6: measured byte rates (EWMA 1/4 of 1 s windows), not a model. */
    uint32_t meas_tx_bps, meas_rx_bps;
    uint32_t meas_tick, meas_tx_last, meas_rx_last;
} p2p_qos_t;
#define P2P_SHAPE_Q 8
#define P2P_SHAPE_BODY 404

typedef struct {
    uint8_t nat_type; /* STUN_NAT_* (stun.h): mapping behaviour seen so far */
    uint8_t mapped_ip[4];
    uint16_t mapped_port;
    uint32_t stun_probes;
    uint32_t nat_punches;
    /* US-074: real STUN (RFC 5389) Binding exchange inside the sealed link.
     * One outstanding transaction per server, two servers kept. */
    struct { uint32_t server; uint8_t txid[12]; uint8_t pending; uint8_t valid;
             uint8_t ip[4]; uint16_t port; } srv[2];
    uint32_t stun_bad;
} p2p_nat_state_t;

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
    p2p_raft_state_t raft;
    p2p_cache_t cache;
    p2p_qos_t qos;
    p2p_nat_state_t nat;
    struct { uint32_t src, id; } seen[P2P_SEEN];
    uint32_t seen_next;
    /* traffic analysis (US-075) */
    uint32_t tx_type[P2P_I_COUNT], rx_type[P2P_I_COUNT];
    uint32_t tx_bytes, rx_bytes, relayed, bad_hello, foreign, dup;
    uint32_t relayed_multi; /* forwarded to a further relay, not to the destination */
    uint32_t rejoins;       /* US-069: a known name came back under a new id after going down */
    uint8_t last_src_ip[4]; /* source IPv4 of the datagram being handled (STUN server side) */
    /* Learned per-type receive baseline (US-075): EWMA of frames per
     * second, x16 fixed point, after P2P_ANALYZE_WARMUP windows. */
    uint32_t base_rate16[P2P_I_COUNT];
    uint32_t last_rx[P2P_I_COUNT];
    uint32_t analyze_tick, analyze_windows, anomalies, last_anomaly_type, last_anomaly_rate;
    uint32_t last_anomaly_peer;   /* Lot 6: id of the peer above its own baseline */
    uint32_t solicited_until;     /* Lot 6: a reply burst we asked for (sync, snapshot) is expected until then */
    uint32_t solicited_skipped;   /* anomaly windows not counted because solicited */
    uint32_t unshaped_tx;         /* messages sent by p2p_send_unshaped (flood proof) */
    /* Lot 6 (US-068/073): data messages waiting for budget */
    struct { uint8_t used; uint32_t peer; uint8_t inner; uint16_t len; uint8_t body[P2P_SHAPE_BODY]; } shape_q[P2P_SHAPE_Q];
    /* Lot 6 (US-070): cache lines written through the Raft log */
    uint32_t cache_raft_applied;
    char last_msg[P2P_TEXT_MAX];
    /* Typed RPC over the sealed channel (not HTTP/2 gRPC). */
#define P2P_RPC_SLOTS 4
#define P2P_GRPC_DEADLINE_TICKS (10U * P2P_HZ)
    uint32_t rpc_seq;
    uint32_t rpc_replays;
    struct { uint8_t used; uint32_t id; uint32_t deadline; } rpc_wait[P2P_RPC_SLOTS];
    struct { uint8_t used; uint32_t peer; uint32_t id; int len; uint8_t bytes[128]; } rpc_cache[P2P_RPC_SLOTS];
    /* Application payloads (Phase 7 collab) carried in sealed P2P_I_APP. */
    void (*app)(void* app_ctx, uint32_t from_id, const uint8_t* data, int length);
    void* app_ctx;
    /* Lot 26: Multi-Region P2P routing */
    uint32_t region_id;
} p2p_node_t;

int p2p_up_step(p2p_node_t* n, int step, const char* name, const uint8_t ip[4], const char* netkey,
                const uint8_t seed[32], const p2p_host_t* h);
int p2p_up(p2p_node_t* n, const char* name, const uint8_t ip[4], const char* netkey,
           const uint8_t seed[32], const p2p_host_t* h);
void p2p_down(p2p_node_t* n);
/* One pump: receive up to `budget` datagrams, then timers. */
void p2p_tick(p2p_node_t* n, const p2p_host_t* h, int budget);
p2p_peer_t* p2p_find(p2p_node_t* n, const char* name);
int p2p_send_text(p2p_node_t* n, const p2p_host_t* h, const char* peer, const char* text);
/* Lot 6: bypasses admit/shaping (flood proof only, see p2p.c) */
int p2p_send_unshaped(p2p_node_t* n, const p2p_host_t* h, const char* peer, const char* text);
int p2p_put(p2p_node_t* n, const p2p_host_t* h, const char* key, const char* value);
const p2p_item_t* p2p_get_local(const p2p_node_t* n, const char* key);
int p2p_get_remote(p2p_node_t* n, const p2p_host_t* h, const char* key);
int p2p_sync(p2p_node_t* n, const p2p_host_t* h, const char* peer);
int p2p_propose(p2p_node_t* n, const p2p_host_t* h, const char* key, const char* value);
int p2p_block(p2p_node_t* n, const char* peer, int blocked);
int p2p_region_set(p2p_node_t* n, uint32_t region_id);
int p2p_cluster_route_forward(p2p_node_t* n, const p2p_host_t* h, uint32_t target_cluster_id, const uint8_t* data, int length);
int p2p_raft_fast_recovery(p2p_node_t* n, const p2p_host_t* h, uint32_t highest_known_term);
int raft_stream_snapshot_chunk(p2p_node_t* n, const p2p_host_t* h, const char* peer, uint32_t chunk_idx, uint32_t total_chunks, const uint8_t* chunk_data, int chunk_len);
int p2p_qos_bbr_step(p2p_node_t* n, uint32_t current_rtt_ms, uint32_t bytes_delivered, uint32_t delta_ms);
int p2p_raft_elect(p2p_node_t* n, const p2p_host_t* h);
/* Lot 23 (US-070 / US-076): Raft compaction and anti-entropy snapshot management */
int p2p_raft_compact(p2p_node_t* n);
/* Lot 25 (US-070 / US-078): Partition healing and dynamic leader lease adjustment */
int p2p_raft_reconcile_partition(p2p_node_t* n, const p2p_host_t* h, uint32_t peer_id, uint32_t remote_term, uint32_t remote_commit_index);
int p2p_raft_update_lease(p2p_node_t* n, uint32_t rtt_ms, uint32_t loss_pct, uint32_t current_tick);
/* Append one log entry. Returns the 1-based index, or -1 if not leader. */
int p2p_raft_append(p2p_node_t* n, const p2p_host_t* h, const char* key, const char* value);
/* 12-byte term/vote trailer ('R','F', version 1). Returns 12 or -1. */
int p2p_raft_vote_export(const p2p_node_t* n, uint8_t* out, int cap);
int p2p_raft_vote_import(p2p_node_t* n, const uint8_t* in, int len);
const p2p_item_t* p2p_cache_get(p2p_node_t* n, const char* key, uint32_t current_tick);
int p2p_cache_put(p2p_node_t* n, const char* key, const char* value, uint32_t ttl_ticks, uint32_t current_tick);
/* Drop a local cache line and tell every up keyed peer. Returns how many were told. */
void p2p_qos_note_rtt(p2p_node_t* n, uint32_t sample_ms);
int p2p_cache_invalidate_v(p2p_node_t* n, const p2p_host_t* h, const char* key, uint32_t version);
int p2p_cache_put_v(p2p_node_t* n, const char* key, const char* value, uint32_t version, uint32_t ttl_ticks, uint32_t current_tick);
int p2p_cache_invalidate(p2p_node_t* n, const p2p_host_t* h, const char* key);
/* Lot 23 (US-070 / US-076): Automatic cache invalidation upon peer disconnects */
void p2p_cache_on_peer_down(p2p_node_t* n, uint32_t peer_id);
/* Lot 23 (US-070 / US-076): EWMA link cost calculation */
uint32_t p2p_calc_link_cost(const p2p_peer_t* p);
/* Lot 6 (US-070): cache write through the Raft log (leader only). Every
 * node applies it at commit: line value, version = log index, fence below.
 * Returns the log index or -1. */
int p2p_cache_raft_put(p2p_node_t* n, const p2p_host_t* h, const char* key, const char* value);
/* AIMD on the data window. Control frames do not pass through it. */
void p2p_qos_note_loss(p2p_node_t* n);
void p2p_qos_note_ack(p2p_node_t* n);
int p2p_route_find(const p2p_node_t* n, uint32_t dest_id, uint32_t* next_hop, uint32_t* total_cost);
int p2p_grpc_call(p2p_node_t* n, const p2p_host_t* h, const char* peer, const char* service, const char* method, const char* payload);
/* timeout_ticks 0 is already expired. fixed_id 0 allocates a new id. */
int p2p_grpc_call_deadline(p2p_node_t* n, const p2p_host_t* h, const char* peer, const char* service,
                           const char* method, const char* payload, uint32_t timeout_ticks, uint32_t fixed_id);
int p2p_stun_probe(p2p_node_t* n, const p2p_host_t* h, const char* stun_server_peer);
int p2p_nat_punch(p2p_node_t* n, const p2p_host_t* h, const char* peer);
/* Report lines into out (peers, health, stats, kv). */
int p2p_report(const p2p_node_t* n, const p2p_host_t* h, const char* what, char* out, int cap);
const char* p2p_type_name(int type);
/* Sealed application payload to one peer id, or to every keyed peer (0).
 * Returns the number of peers it was sent to. */
int p2p_send_app(p2p_node_t* n, const p2p_host_t* h, uint32_t peer_id, const uint8_t* data, int length);
const char* p2p_peer_name(const p2p_node_t* n, uint32_t id);
uint32_t p2p_member_count(const p2p_node_t* n);

/* Lot 29: Dynamic Re-keying, Zero-Trust Verification & Raft 2PC (US-064 / US-075) */
#define P2P_2PC_STATE_NONE      0
#define P2P_2PC_STATE_PREPARED  1
#define P2P_2PC_STATE_COMMITTED 2
#define P2P_2PC_STATE_ABORTED   3
#define P2P_2PC_MAX_TX          8

typedef struct {
    uint32_t tx_id;
    uint8_t state;
    char mutation_cmd[128];
    uint32_t participant_mask;
    uint32_t votes_mask;
    uint32_t created_tick;
    uint8_t active;
} p2p_2pc_tx_t;

int p2p_rekey_init_peer(p2p_peer_t *p, const uint8_t *initial_key, uint32_t rekey_interval_ticks, uint32_t rekey_pkt_limit);
int p2p_rekey_check_and_rotate(p2p_node_t *n, uint32_t peer_id, uint32_t current_tick);

int p2p_peer_revoke(p2p_node_t *n, uint32_t peer_id, const char *reason);
int p2p_zero_trust_verify_pkt(p2p_node_t *n, uint32_t peer_id, uint64_t seq_num, uint32_t pkt_len, uint32_t current_tick);

int p2p_raft_2pc_prepare(p2p_node_t *n, uint32_t tx_id, const char *mutation_cmd, uint32_t participant_mask);
int p2p_raft_2pc_commit(p2p_node_t *n, uint32_t tx_id);
int p2p_raft_2pc_abort(p2p_node_t *n, uint32_t tx_id);
int p2p_raft_2pc_status(p2p_node_t *n, uint32_t tx_id, int *out_state);
/* ---- Lot 28: P2P WAN Multi-Hop Mesh, Epidemic Gossip & NAT Hole Punching (US-061 / US-063 / US-074) ---- */
#define P2P_GOSSIP_ITEMS_MAX 16
typedef struct {
    char key[P2P_KEY_MAX];
    char value[P2P_VAL_MAX];
    uint32_t version;
    uint32_t origin;
    uint32_t ttl_hops;
} p2p_gossip_item_t;

typedef struct {
    p2p_gossip_item_t items[P2P_GOSSIP_ITEMS_MAX];
    int count;
    uint32_t rounds_executed;
} p2p_gossip_table_t;

#define P2P_NAT_SESSIONS_MAX 8
typedef struct {
    uint32_t peer_id;
    uint8_t ip[4];
    uint16_t port;
    uint32_t last_keepalive_tick;
    uint8_t session_active;
    uint8_t hole_punched;
} p2p_nat_session_t;

typedef struct {
    p2p_nat_session_t sessions[P2P_NAT_SESSIONS_MAX];
    int count;
    uint32_t total_keepalives_sent;
} p2p_nat_keepalive_table_t;

int p2p_gossip_broadcast(p2p_node_t* n, const p2p_host_t* h, const char* key, const char* value, uint32_t version);
int p2p_gossip_tick(p2p_node_t* n, const p2p_host_t* h, p2p_gossip_table_t* table);
int p2p_nat_keepalive_step(p2p_node_t* n, const p2p_host_t* h, p2p_nat_keepalive_table_t* table, uint32_t now_tick);
int p2p_multipath_route_select(const p2p_node_t* n, uint32_t dest_id, uint32_t* path1_next, uint32_t* path2_next);

/* Lot 30: P2P WAN Multi-Region Mesh & Distributed Consensus Partitioning (US-065 / US-077) */
#define P2P_MULTI_RAFT_GROUPS_MAX 4
#define P2P_MERKLE_NODES_MAX 32

typedef struct {
    uint32_t group_id;
    uint32_t term;
    uint32_t leader_id;
    uint32_t commit_index;
    uint32_t last_log_index;
    uint8_t active;
} p2p_multi_raft_group_t;

typedef struct {
    p2p_multi_raft_group_t groups[P2P_MULTI_RAFT_GROUPS_MAX];
    int group_count;
} p2p_multi_raft_mgr_t;

typedef struct {
    uint32_t node_id;
    uint32_t hash;
    char key[P2P_KEY_MAX];
    char value[P2P_VAL_MAX];
} p2p_merkle_entry_t;

typedef struct {
    p2p_merkle_entry_t entries[P2P_MERKLE_NODES_MAX];
    int count;
    uint32_t root_hash;
} p2p_merkle_tree_t;

int p2p_region_route_calculate(const p2p_node_t *n, uint32_t src_region, uint32_t dst_region, uint32_t dest_id, uint32_t *out_next_hop, uint32_t *out_cost);

void p2p_multi_raft_init(p2p_multi_raft_mgr_t *mgr);
int p2p_multi_raft_propose(p2p_multi_raft_mgr_t *mgr, uint32_t group_id, const char *cmd);
int p2p_multi_raft_commit(p2p_multi_raft_mgr_t *mgr, uint32_t group_id, uint32_t index);

void p2p_merkle_init(p2p_merkle_tree_t *tree);
int p2p_merkle_insert(p2p_merkle_tree_t *tree, uint32_t node_id, const char *key, const char *value);
uint32_t p2p_merkle_sync_tree(p2p_merkle_tree_t *tree);
int p2p_merkle_diff_sync(const p2p_merkle_tree_t *local_tree, const p2p_merkle_tree_t *remote_tree, uint32_t *out_diff_ids, int max_diffs);

/* Lot 31: P2P WAN Mesh Multi-Topology & Dynamic Byzantine Fault Tolerance (BFT) (US-066 / US-078) */
#define P2P_TOPOLOGY_RING 1
#define P2P_TOPOLOGY_STAR 2
#define P2P_TOPOLOGY_MESH 3

#define P2P_BFT_STAGE_IDLE       0
#define P2P_BFT_STAGE_PREPREPARE 1
#define P2P_BFT_STAGE_PREPARE    2
#define P2P_BFT_STAGE_COMMIT     3

typedef struct {
    uint32_t view_id;
    uint32_t sequence_num;
    uint8_t stage;
    uint32_t preprepare_votes;
    uint32_t prepare_votes;
    uint32_t commit_votes;
    uint32_t f_max_faulty;
    char proposal_hash[32];
    uint8_t committed;
} p2p_bft_state_t;

#define P2P_BLOOM_FILTER_SIZE_BITS 256
typedef struct {
    uint8_t filter[P2P_BLOOM_FILTER_SIZE_BITS / 8];
    uint32_t items_added;
} p2p_bloom_filter_t;

int p2p_topology_select_route(const p2p_node_t *n, uint32_t topology_type, uint32_t dest_id, uint32_t *out_next_hop);

int p2p_bft_preprepare(p2p_bft_state_t *bft, uint32_t view_id, uint32_t seq, const char *proposal_data, uint32_t f_max);
int p2p_bft_prepare(p2p_bft_state_t *bft, uint32_t voter_id, uint32_t view_id, uint32_t seq);
int p2p_bft_commit(p2p_bft_state_t *bft, uint32_t voter_id, uint32_t view_id, uint32_t seq);

void p2p_bloom_sync_init(p2p_bloom_filter_t *filter);
int p2p_bloom_sync_diff(const p2p_bloom_filter_t *local_bf, const p2p_bloom_filter_t *remote_bf, int *out_mismatch_count);

/* Lot 32: P2P WAN Mesh Dynamic Topology Reconfiguration & Zero-Knowledge Consensus (US-067 / US-079) */
#define P2P_ZK_PROOF_SIZE 64

typedef struct {
    uint32_t peer_id;
    uint32_t rtt_ms;
    uint32_t ewma_rtt;
    uint32_t flap_count;
    uint8_t active;
} p2p_topo_peer_info_t;

typedef struct {
    char statement[32];
    uint8_t proof_bytes[P2P_ZK_PROOF_SIZE];
    uint32_t proof_len;
} p2p_zk_proof_t;

int p2p_topology_reconfigure(p2p_node_t *n, const p2p_topo_peer_info_t *peers, int peer_count, uint32_t *out_active_topology);
int p2p_zk_consensus_verify(const p2p_zk_proof_t *proof, const char *expected_statement, int *out_valid);
int p2p_bbr_anti_entropy_sync(p2p_node_t *n, uint32_t target_peer_id, uint32_t bw_est_kbps, int *out_items_synced);

/* Lot 33: P2P Cross-WAN Geo-Sharded Multi-Region Consensus & Zero-Knowledge Bulletproof Validation (US-068 / US-080) */
#define P2P_GEO_SHARD_ENTRIES_MAX 16
#define P2P_BULLETPROOF_BYTES_MAX 64

typedef struct {
    uint32_t entry_id;
    uint32_t region_origin;
    char payload[P2P_VAL_MAX];
    uint8_t synced;
} p2p_geo_entry_t;

typedef struct {
    uint32_t shard_id;
    uint32_t region_mask;
    p2p_geo_entry_t entries[P2P_GEO_SHARD_ENTRIES_MAX];
    int count;
    uint32_t next_consensus_id;
    uint8_t active;
} p2p_geo_shard_t;

typedef struct {
    uint32_t value_commitment;
    uint32_t blinding_factor;
    uint8_t proof_bytes[P2P_BULLETPROOF_BYTES_MAX];
    uint32_t proof_len;
} p2p_zk_bulletproof_t;

void p2p_geo_shard_init(p2p_geo_shard_t *shard, uint32_t shard_id, uint32_t region_mask);
int p2p_geo_consensus_propose(p2p_geo_shard_t *shard, uint32_t proposer_id, const char *data, uint32_t *out_consensus_id);
int p2p_geo_cross_region_sync(p2p_geo_shard_t *shard, uint32_t target_region_id, int *out_synced_entries);

int p2p_zk_bulletproof_create_commitment(uint32_t value, uint32_t blinding_factor, p2p_zk_bulletproof_t *out_proof);
int p2p_zk_bulletproof_verify(const p2p_zk_bulletproof_t *proof, uint32_t min_val, uint32_t max_val, int *out_valid);

/* Lot 35: P2P Cross-WAN Resilient Raft Group Sharding & Zero-Knowledge Bulletproof Inner-Product Proofs (US-071 / US-083) */
#define P2P_CROSS_WAN_GROUPS_MAX 4
#define P2P_BULLETPROOF_INNER_PRODUCT_SIZE 64

typedef struct {
    uint32_t group_id;
    uint32_t wan_region_mask;
    uint32_t term;
    uint32_t leader_id;
    uint8_t active;
} p2p_cross_wan_raft_group_t;

typedef struct {
    p2p_cross_wan_raft_group_t groups[P2P_CROSS_WAN_GROUPS_MAX];
    int group_count;
    uint32_t total_sharded_proposals;
} p2p_cross_wan_raft_shard_mgr_t;

typedef struct {
    uint32_t vector_a_commitment;
    uint32_t vector_b_commitment;
    uint32_t inner_product_value;
    uint8_t proof_bytes[P2P_BULLETPROOF_INNER_PRODUCT_SIZE];
    uint32_t proof_len;
} p2p_zk_bulletproof_ip_t;

void p2p_cross_wan_raft_shard_init(p2p_cross_wan_raft_shard_mgr_t *mgr);
int p2p_cross_wan_raft_propose_sharded(p2p_cross_wan_raft_shard_mgr_t *mgr, uint32_t group_id, const char *command);
int p2p_cross_wan_raft_sync_groups(p2p_cross_wan_raft_shard_mgr_t *mgr, uint32_t target_wan_region, int *out_synced_groups);

int p2p_zk_bulletproof_inner_product_create(const uint32_t *vec_a, const uint32_t *vec_b, uint32_t count, p2p_zk_bulletproof_ip_t *out_proof);
int p2p_zk_bulletproof_inner_product_verify(const p2p_zk_bulletproof_ip_t *proof, uint32_t expected_inner_product, int *out_valid);

/* Lot 37: P2P Mesh Network Cross-WAN Resilient Raft Group Leader Election & Zero-Knowledge Bulletproof Batch Verification Engine (US-072 / US-084) */
#define P2P_WAN_RAFT_QUORUM_MIN 2
#define P2P_ZK_BULLETPROOF_BATCH_MAX 16

typedef struct {
    uint32_t group_id;
    uint32_t term;
    uint32_t leader_id;
    uint32_t votes_granted;
    uint32_t total_voters;
    uint32_t lease_ticks;
    uint32_t leader_lease_expire_tick;
    uint8_t state; /* 0=follower, 1=candidate, 2=leader */
} p2p_wan_raft_elec_t;

typedef struct {
    p2p_zk_bulletproof_ip_t proofs[P2P_ZK_BULLETPROOF_BATCH_MAX];
    uint32_t expected_inner_products[P2P_ZK_BULLETPROOF_BATCH_MAX];
    int count;
    uint32_t verified_count;
} p2p_zk_bulletproof_batch_t;

void p2p_wan_raft_elec_init(p2p_wan_raft_elec_t *elec, uint32_t group_id, uint32_t total_voters, uint32_t lease_ticks);
int p2p_wan_raft_leader_elect(p2p_wan_raft_elec_t *elec, uint32_t candidate_id, uint32_t current_term, uint32_t votes_received, uint32_t current_tick);
int p2p_wan_raft_check_leader_lease(const p2p_wan_raft_elec_t *elec, uint32_t current_tick, int *out_lease_valid);

void p2p_zk_bulletproof_batch_init(p2p_zk_bulletproof_batch_t *batch);
int p2p_zk_bulletproof_batch_add(p2p_zk_bulletproof_batch_t *batch, const p2p_zk_bulletproof_ip_t *proof, uint32_t expected_inner_product);
int p2p_zk_bulletproof_batch_verify(p2p_zk_bulletproof_batch_t *batch, int *out_all_valid);

/* Lot 38: P2P Cross-WAN Multi-Raft Consensus Group Sharding & Zero-Knowledge Bulletproof Range Proof Verification Engine (US-073 / US-085) */
#define P2P_MULTI_RAFT_WAN_SHARDS_MAX 8
#define P2P_ZK_RANGE_BATCH_MAX 16

typedef struct {
    uint32_t shard_id;
    uint32_t wan_region_id;
    uint32_t leader_id;
    uint32_t term;
    uint32_t lease_ticks;
    uint32_t lease_expire_tick;
    uint8_t healthy;
} p2p_multi_raft_wan_shard_t;

typedef struct {
    p2p_multi_raft_wan_shard_t shards[P2P_MULTI_RAFT_WAN_SHARDS_MAX];
    int shard_count;
    uint32_t total_failovers;
    uint32_t rebalance_count;
} p2p_multi_raft_wan_shard_mgr_t;

typedef struct {
    uint64_t min_value;
    uint64_t max_value;
    uint64_t committed_value;
    uint32_t commitment_hash;
    uint8_t proof_bytes[P2P_BULLETPROOF_INNER_PRODUCT_SIZE];
    uint32_t proof_len;
} p2p_zk_bulletproof_range_proof_t;

typedef struct {
    p2p_zk_bulletproof_range_proof_t proofs[P2P_ZK_RANGE_BATCH_MAX];
    int count;
    uint32_t verified_count;
} p2p_zk_range_batch_t;

void p2p_multi_raft_wan_shard_init(p2p_multi_raft_wan_shard_mgr_t *mgr);
int p2p_multi_raft_wan_shard_add(p2p_multi_raft_wan_shard_mgr_t *mgr, uint32_t shard_id, uint32_t wan_region_id, uint32_t leader_id);
int p2p_multi_raft_wan_shard_failover(p2p_multi_raft_wan_shard_mgr_t *mgr, uint32_t shard_id, uint32_t new_leader_id, uint32_t current_tick);
int p2p_multi_raft_wan_shard_rebalance_leases(p2p_multi_raft_wan_shard_mgr_t *mgr, uint32_t current_tick, int *out_rebalanced);

void p2p_zk_range_batch_init(p2p_zk_range_batch_t *batch);
int p2p_zk_range_proof_create(uint64_t value, uint64_t min_val, uint64_t max_val, p2p_zk_bulletproof_range_proof_t *out_proof);
int p2p_zk_range_batch_add(p2p_zk_range_batch_t *batch, const p2p_zk_bulletproof_range_proof_t *proof);
int p2p_zk_range_batch_verify(p2p_zk_range_batch_t *batch, int *out_all_valid);

/* Lot 39: P2P Mesh Network Cross-WAN Asynchronous Consensus Replication & Zero-Knowledge PLONK Verification Engine (US-074 / US-086) */
#define P2P_WAN_SLIDING_WINDOW_SZ 16
#define P2P_RAFT_LOG_DELTA_MAX 128
#define P2P_ZK_PLONK_WIRES_MAX 8
#define P2P_ZK_PLONK_CIRCUIT_GATES_MAX 16

typedef struct {
    uint32_t index;
    uint32_t term;
    uint8_t data[64];
    uint32_t len;
    uint8_t acked;
} p2p_wan_raft_log_entry_t;

typedef struct {
    p2p_wan_raft_log_entry_t window[P2P_WAN_SLIDING_WINDOW_SZ];
    uint32_t window_start;
    uint32_t window_end;
    uint32_t ack_watermark;
    uint32_t compressed_bytes;
    uint32_t uncompressed_bytes;
} p2p_wan_async_repl_t;

typedef struct {
    int32_t q_l;
    int32_t q_r;
    int32_t q_o;
    int32_t q_m;
    int32_t q_c;
} p2p_zk_plonk_gate_t;

typedef struct {
    uint32_t wire_a;
    uint32_t wire_b;
    uint32_t wire_c;
    p2p_zk_plonk_gate_t gate;
} p2p_zk_plonk_circuit_entry_t;

typedef struct {
    p2p_zk_plonk_circuit_entry_t gates[P2P_ZK_PLONK_CIRCUIT_GATES_MAX];
    int gate_count;
    uint32_t wire_values[P2P_ZK_PLONK_WIRES_MAX];
    uint8_t proof_bytes[128];
    uint32_t proof_len;
} p2p_zk_plonk_proof_t;

typedef struct {
    uint32_t verified_proofs;
    uint32_t failed_proofs;
} p2p_zk_plonk_verifier_t;

void p2p_wan_async_repl_init(p2p_wan_async_repl_t *repl, uint32_t start_index);
int p2p_wan_async_repl_push(p2p_wan_async_repl_t *repl, uint32_t term, const uint8_t *data, uint32_t len);
int p2p_wan_async_repl_ack(p2p_wan_async_repl_t *repl, uint32_t ack_index);
int p2p_wan_async_repl_compress_delta(const uint8_t *prev, uint32_t prev_len, const uint8_t *curr, uint32_t curr_len, uint8_t *out_delta, uint32_t *out_delta_len);
int p2p_wan_async_repl_decompress_delta(const uint8_t *prev, uint32_t prev_len, const uint8_t *delta, uint32_t delta_len, uint8_t *out_curr, uint32_t *out_curr_len);

void p2p_zk_plonk_verifier_init(p2p_zk_plonk_verifier_t *v);
int p2p_zk_plonk_proof_create(const p2p_zk_plonk_circuit_entry_t *gates, int gate_cnt, const uint32_t *wires, p2p_zk_plonk_proof_t *out_proof);
int p2p_zk_plonk_verify(p2p_zk_plonk_verifier_t *v, const p2p_zk_plonk_proof_t *proof, int *out_valid);

#endif
