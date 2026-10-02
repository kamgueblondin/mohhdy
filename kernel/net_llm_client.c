/* kernel/net_llm_client.c - Tranche 5: LLM network session (DHCP, DNS, TCP,
 * TLS 1.2, HTTP, SSE) over an NE2000 given by net_llm_client_bind().
 * Moved out of kernel/kernel.c unchanged. Linked in the kernel (boot NIC,
 * degraded path) and, with -DMOHHDY_RING3, in the Ring 3 networker that
 * owns the card: relayed SYS_LLM_* calls then run here at CPL 3. */
#include "net_llm_client.h"
#include "tls_trust_anchor.h"
#include "tls_test_trust_anchor.h"
#include "tls_test_leaf.h"
#include "net_tls_server.h"
#ifndef MOHHDY_RING3
#include "net_nic_owner.h"
#endif

static ne2k_device_t* g_llm_dev;
static const ne2k_io_t* g_llm_io;
static uint8_t g_llm_present;

void net_llm_client_bind(ne2k_device_t* device, const ne2k_io_t* io, int present) {
    g_llm_dev = device;
    g_llm_io = io;
    g_llm_present = present ? 1U : 0U;
}

/* Contexte persistant appartenant au noyau ; le seul secret est le bearer borné et effaçable ci-dessous. */
net_dhcp_lease_t boot_llm_lease;
static ne2k_llm_socket_session_t boot_llm_socket_session;
typedef struct {
    uint8_t armed;
    uint8_t retries_used;
    uint8_t retry_limit;
    uint32_t next_retry_tick;
    os_llm_acquire_start_request_t acquire;
} kernel_llm_dhcp_maintenance_t;
#define KERNEL_LLM_DHCP_RETRY_BASE_TICKS 100U
#define KERNEL_LLM_DHCP_RETRY_MAX_TICKS 10000U
#define KERNEL_LLM_DHCP_RETRY_LIMIT 5U
static kernel_llm_dhcp_maintenance_t boot_llm_dhcp_maintenance;
/* Espaces de travail noyau fixes : aucun buffer du chemin DHCP→LLM n’est alloué. */
net_arp_cache_t boot_llm_arp_cache;
uint8_t boot_llm_dhcp_tx[KERNEL_LLM_FRAME_CAPACITY];
static uint8_t boot_llm_dhcp_rx[KERNEL_LLM_FRAME_CAPACITY];
static uint8_t boot_llm_arp_request[KERNEL_LLM_FRAME_CAPACITY];
static uint8_t boot_llm_arp_rx[KERNEL_LLM_FRAME_CAPACITY];
uint8_t boot_llm_frame[KERNEL_LLM_FRAME_CAPACITY];
/* Matériaux, client TLS et buffers réservés au noyau ; aucun n’est accessible via syscall. */
static ne2k_tls_client_t boot_llm_tls_client;
static x509_certificate_view_t boot_llm_trust_anchor;
static uint8_t boot_llm_trust_anchor_ready;
static x509_certificate_view_t boot_llm_test_trust_anchor;
uint8_t boot_llm_test_trust_anchor_ready;
static rtc_io_t boot_llm_rtc_io;
static char boot_llm_hostname[OS_LLM_HOSTNAME_MAX];
static char boot_llm_openai_bearer[OS_LLM_BEARER_MAX];
static uint8_t boot_llm_openai_bearer_ready;
static uint8_t boot_llm_client_random[NET_TLS_X25519_KEY_LENGTH];
static uint8_t boot_llm_client_private[NET_TLS_X25519_KEY_LENGTH];
uint8_t boot_llm_rdrand_supported;
static uint8_t boot_llm_tls_entropy_ready;
static uint8_t boot_llm_tls_material_ready;
static uint8_t boot_llm_tls_record[KERNEL_LLM_TLS_RECORD_CAPACITY];
static uint8_t boot_llm_tls_handshake[KERNEL_LLM_TLS_RECORD_CAPACITY];
static uint8_t boot_llm_tls_transcript[KERNEL_LLM_TLS_RECORD_CAPACITY];
static uint8_t boot_llm_tls_hello[KERNEL_LLM_TLS_HELLO_CAPACITY];
uint32_t boot_llm_rsa_workspace[KERNEL_LLM_TLS_WORKSPACE_WORDS];
uint32_t boot_llm_x25519_workspace[KERNEL_LLM_TLS_WORKSPACE_WORDS];
uint8_t boot_llm_prf_workspace[KERNEL_LLM_TLS_RECORD_CAPACITY];
static uint8_t boot_llm_tcp_segment[KERNEL_LLM_TLS_RECORD_CAPACITY];
static uint8_t boot_llm_flight_records[KERNEL_LLM_TLS_RECORD_CAPACITY];
static uint32_t boot_llm_flight_records_length;
uint8_t boot_llm_plaintext[KERNEL_LLM_TLS_RECORD_CAPACITY];
/* HTTP/LLM : buffers fixes noyau ; seul le texte extrait est copié à l’appelant. */
static uint8_t boot_llm_http_json[KERNEL_LLM_TLS_RECORD_CAPACITY];
static uint8_t boot_llm_http_request[KERNEL_LLM_TLS_RECORD_CAPACITY];
static uint8_t boot_llm_http_tls_record[KERNEL_LLM_TLS_RECORD_CAPACITY];
static uint8_t boot_llm_http_response_buffer[KERNEL_LLM_TLS_RECORD_CAPACITY];
static uint8_t boot_llm_http_text[OS_LLM_TEXT_MAX];
static net_http_response_accumulator_t boot_llm_http_accumulator;
static net_http_response_view_t boot_llm_http_response;
static uint8_t boot_llm_sse_http_buffer[KERNEL_LLM_TLS_RECORD_CAPACITY];
static uint8_t boot_llm_sse_event_buffer[KERNEL_LLM_TLS_RECORD_CAPACITY];
static net_llm_sse_response_t boot_llm_sse_response;
static uint8_t boot_llm_http_provider;
static uint8_t boot_llm_http_streaming;
typedef struct {
    uint8_t pending;
    uint8_t is_sse_resume;
    uint8_t event_id_length;
    uint8_t event_id[NET_LLM_SSE_EVENT_ID_MAX];
    os_llm_request_t request;
} kernel_llm_application_recovery_t;
static kernel_llm_application_recovery_t boot_llm_application_recovery;

static int boot_peer_listen_socket = -1;
static uint8_t boot_peer_segment[1500];
static uint8_t boot_peer_remote_ip[4];
static net_tls_server_t boot_peer_tls_server;
static uint8_t boot_peer_tls_ready;
static uint8_t boot_peer_tls_step; /* 0 idle..7 finished+app */
static uint8_t boot_peer_tls_record[1200];
static uint8_t boot_peer_server_random[32];
static uint8_t boot_peer_server_private[32];
static uint8_t boot_peer_app_seen;
static uint8_t boot_metier_client_sent;

#ifndef MOHHDY_RING3
int net_llm_client_utc(rtc_io_t* io, char* out, uint16_t capacity) {
    if (rtc_i386_io(io) != 0) return -1;
    return rtc_read_utc(io, out, capacity);
}
#endif

static void kernel_llm_clear_bytes(uint8_t* buffer, uint32_t length);
static int kernel_llm_rdrand_supported(void);
static int kernel_llm_close_internal(uint8_t preserve_provider);
int kernel_llm_close(void);

void net_llm_client_reset(void) {
    net_dhcp_lease_clear(&boot_llm_lease);
    (void)ne2k_llm_socket_session_init(&boot_llm_socket_session);
    (void)net_arp_cache_init(&boot_llm_arp_cache);
    boot_llm_rdrand_supported = kernel_llm_rdrand_supported() ? 1U : 0U;
    boot_llm_trust_anchor_ready = (x509_certificate_parse(aos_tls_isrg_root_x1_der,
        aos_tls_isrg_root_x1_der_len, &boot_llm_trust_anchor) == 0 &&
        x509_rsa_public_key_validate(&boot_llm_trust_anchor) == 0) ? 1U : 0U;
    boot_llm_test_trust_anchor_ready = (x509_certificate_parse(aos_tls_test_root_der,
        aos_tls_test_root_der_len, &boot_llm_test_trust_anchor) == 0 &&
        x509_rsa_public_key_validate(&boot_llm_test_trust_anchor) == 0) ? 1U : 0U;
    boot_llm_tls_entropy_ready = 0U;
    boot_llm_tls_material_ready = 0U;
    boot_llm_flight_records_length = 0U;
    boot_llm_http_provider = NE2K_LLM_PROVIDER_OLLAMA;
    boot_llm_http_streaming = 0U;
    boot_llm_application_recovery.pending = 0U;
    boot_llm_dhcp_maintenance.armed = 0U;
    boot_llm_dhcp_maintenance.retries_used = 0U;
    boot_llm_dhcp_maintenance.retry_limit = KERNEL_LLM_DHCP_RETRY_LIMIT;
    boot_llm_dhcp_maintenance.next_retry_tick = 0U;
    boot_peer_listen_socket = -1;
    boot_peer_tls_ready = 0U;
    boot_peer_tls_step = 0U;
    boot_peer_app_seen = 0U;
    boot_metier_client_sent = 0U;
}

static int kernel_llm_rdrand_supported(void) {
    uint32_t eax, ebx, ecx, edx;
    __asm__ volatile("cpuid" : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx) : "a"(1U), "c"(0U));
    (void)eax;
    (void)ebx;
    (void)edx;
    return (ecx & (1U << 30)) != 0U;
}

int kernel_llm_rdrand_word(uint32_t* output) {
    uint8_t success;
    uint32_t value;
    if (!output) return -1;
    __asm__ volatile("rdrand %0; setc %1" : "=&r"(value), "=qm"(success) : : "cc");
    if (!success) return -1;
    *output = value;
    return 0;
}

static void kernel_llm_clear_tls_material(void) {
    uint16_t index;
    for (index = 0U; index < NET_TLS_X25519_KEY_LENGTH; ++index) {
        boot_llm_client_random[index] = 0U;
        boot_llm_client_private[index] = 0U;
    }
    boot_llm_tls_entropy_ready = 0U;
    boot_llm_tls_material_ready = 0U;
}

static int kernel_llm_fill_tls_material(void) {
    uint8_t random[NET_TLS_X25519_KEY_LENGTH];
    uint8_t private_key[NET_TLS_X25519_KEY_LENGTH];
    uint16_t byte_index;
    uint8_t attempt;
    uint32_t word;
    if (!boot_llm_rdrand_supported) return -1;
    for (byte_index = 0U; byte_index < NET_TLS_X25519_KEY_LENGTH; byte_index += 4U) {
        for (attempt = 0U; attempt < 10U; ++attempt)
            if (kernel_llm_rdrand_word(&word) == 0) break;
        if (attempt == 10U) goto failure;
        random[byte_index] = (uint8_t)word;
        random[byte_index + 1U] = (uint8_t)(word >> 8);
        random[byte_index + 2U] = (uint8_t)(word >> 16);
        random[byte_index + 3U] = (uint8_t)(word >> 24);
        for (attempt = 0U; attempt < 10U; ++attempt)
            if (kernel_llm_rdrand_word(&word) == 0) break;
        if (attempt == 10U) goto failure;
        private_key[byte_index] = (uint8_t)word;
        private_key[byte_index + 1U] = (uint8_t)(word >> 8);
        private_key[byte_index + 2U] = (uint8_t)(word >> 16);
        private_key[byte_index + 3U] = (uint8_t)(word >> 24);
    }
    for (byte_index = 0U; byte_index < NET_TLS_X25519_KEY_LENGTH; ++byte_index) {
        boot_llm_client_random[byte_index] = random[byte_index];
        boot_llm_client_private[byte_index] = private_key[byte_index];
    }
    boot_llm_tls_entropy_ready = 1U;
    boot_llm_tls_material_ready = boot_llm_trust_anchor_ready;
    return 0;
failure:
    for (byte_index = 0U; byte_index < NET_TLS_X25519_KEY_LENGTH; ++byte_index) {
        random[byte_index] = 0U;
        private_key[byte_index] = 0U;
    }
    kernel_llm_clear_tls_material();
    return -1;
}

/* Bit 0 : NE2000 prêt ; bit 1 : bail DHCP ; bit 2 : RDRAND ; bit 3 : ancre X.509 ;
 * bit 4 : ancre de test locale ; bits 8..15 : phase LLM. */
uint32_t kernel_llm_session_status(void) {
    return (g_llm_present ? 1U : 0U) |
           (boot_llm_lease.valid ? 2U : 0U) |
           (boot_llm_rdrand_supported ? 4U : 0U) |
           (boot_llm_trust_anchor_ready ? 8U : 0U) |
           (boot_llm_test_trust_anchor_ready ? 16U : 0U) |
           ((uint32_t)boot_llm_socket_session.state.phase << 8);
}

static int kernel_llm_hostname_is_valid(const char hostname[OS_LLM_HOSTNAME_MAX]) {
    uint16_t index;
    if (!hostname || hostname[0] == '\0' || hostname[0] == '.' || hostname[0] == '-') return 0;
    for (index = 0U; index < OS_LLM_HOSTNAME_MAX; ++index) {
        char value = hostname[index];
        if (value == '\0') return hostname[index - 1U] != '.' && hostname[index - 1U] != '-';
        if (!((value >= 'a' && value <= 'z') || (value >= 'A' && value <= 'Z') ||
              (value >= '0' && value <= '9') || value == '.' || value == '-')) return 0;
    }
    return 0;
}

static int kernel_llm_ascii_lower(char value) {
    if (value >= 'A' && value <= 'Z') return (int)(value - 'A' + 'a');
    return (int)(unsigned char)value;
}

static int kernel_llm_hostname_has_suffix(const char* hostname, const char* suffix) {
    uint16_t host_length = 0U, suffix_length = 0U, index;
    if (!hostname || !suffix) return 0;
    while (hostname[host_length] != '\0' && host_length < OS_LLM_HOSTNAME_MAX) host_length++;
    while (suffix[suffix_length] != '\0') suffix_length++;
    if (host_length < suffix_length) return 0;
    for (index = 0U; index < suffix_length; ++index) {
        if (kernel_llm_ascii_lower(hostname[host_length - suffix_length + index]) !=
            kernel_llm_ascii_lower(suffix[index])) return 0;
    }
    if (host_length != suffix_length && hostname[host_length - suffix_length - 1U] != '.') return 0;
    return 1;
}

/* Ancre locale pour example.com / api.example.test ; ISRG Root X1 reste le defaut public. */
static const char* kernel_llm_session_hostname(void) {
    if (boot_llm_hostname[0] != '\0') return boot_llm_hostname;
    return boot_llm_dhcp_maintenance.acquire.hostname;
}

static const x509_certificate_view_t* kernel_llm_select_trust_anchor(void) {
    const char* hostname = kernel_llm_session_hostname();
    if (boot_llm_test_trust_anchor_ready &&
        (kernel_llm_hostname_has_suffix(hostname, "example.test") ||
         kernel_llm_hostname_has_suffix(hostname, "example.com") ||
         kernel_llm_hostname_has_suffix(hostname, "peer.local")))
        return &boot_llm_test_trust_anchor;
    return &boot_llm_trust_anchor;
}

/* Identite feuille locale example.com pour le pair guest peer.local. */
static const char* kernel_llm_identity_hostname(void) {
    const char* hostname = kernel_llm_session_hostname();
    if (kernel_llm_hostname_has_suffix(hostname, "peer.local")) return "example.com";
    return hostname;
}

static void kernel_llm_copy_hostname(const char source[OS_LLM_HOSTNAME_MAX]) {
    uint16_t index;
    for (index = 0U; index < OS_LLM_HOSTNAME_MAX; ++index) {
        boot_llm_hostname[index] = source[index];
        if (source[index] == '\0') return;
    }
    boot_llm_hostname[OS_LLM_HOSTNAME_MAX - 1U] = '\0';
}

int kernel_llm_acquire_start(const os_llm_acquire_start_request_t* request) {
    int status;
    if (!request || !kernel_llm_hostname_is_valid(request->hostname) ||
        request->dhcp_attempts == 0U || request->dns_attempts == 0U || request->arp_attempts == 0U ||
        request->dhcp_attempts > OS_LLM_ACQUIRE_MAX_ATTEMPTS ||
        request->dns_attempts > OS_LLM_ACQUIRE_MAX_ATTEMPTS ||
        request->arp_attempts > OS_LLM_ACQUIRE_MAX_ATTEMPTS ||
        request->local_port == 0U || request->remote_port == 0U) return OS_LLM_ACQUIRE_BAD_REQUEST;
    if (!g_llm_present) return OS_LLM_ACQUIRE_UNAVAILABLE;
    if (boot_llm_socket_session.state.phase != NE2K_LLM_CONNECTION_IDLE) return OS_LLM_ACQUIRE_IN_PROGRESS;
    if (kernel_llm_fill_tls_material() != 0) return OS_LLM_ACQUIRE_TLS_ENTROPY;
    if (net_arp_cache_init(&boot_llm_arp_cache) != 0 ||
        ne2k_tls_client_init(&boot_llm_tls_client, boot_llm_tls_record, sizeof(boot_llm_tls_record),
                             boot_llm_tls_handshake, sizeof(boot_llm_tls_handshake),
                             boot_llm_tls_transcript, sizeof(boot_llm_tls_transcript)) != 0) {
        kernel_llm_clear_tls_material();
        return OS_LLM_ACQUIRE_FAILED;
    }
    boot_llm_flight_records_length = 0U;
    status = ne2k_llm_socket_session_acquire_start_dhcp(
        g_llm_dev, g_llm_io, &boot_llm_arp_cache,
        boot_llm_dhcp_tx, sizeof(boot_llm_dhcp_tx), boot_llm_dhcp_rx, sizeof(boot_llm_dhcp_rx),
        request->xid, request->dhcp_attempts,
        boot_llm_arp_request, sizeof(boot_llm_arp_request), boot_llm_arp_rx, sizeof(boot_llm_arp_rx),
        boot_llm_frame, sizeof(boot_llm_frame), request->dns_id, request->hostname,
        request->dns_attempts, request->arp_attempts, request->local_port, request->remote_port,
        request->local_sequence, &boot_llm_lease, &boot_llm_socket_session);
    if (status != 0) {
        kernel_llm_clear_tls_material();
        if (status == -12) return OS_LLM_ACQUIRE_DHCP_DISCOVER_FAILED;
        if (status == -13) return OS_LLM_ACQUIRE_DHCP_OFFER_TIMEOUT;
        if (status == -14) return OS_LLM_ACQUIRE_DHCP_REQUEST_FAILED;
        if (status == -15) return OS_LLM_ACQUIRE_DHCP_ACK_TIMEOUT;
        if (status == -2) return OS_LLM_ACQUIRE_DHCP_FAILED;
        if (status == -3) return OS_LLM_ACQUIRE_BOOTSTRAP_FAILED;
        return OS_LLM_ACQUIRE_FAILED;
    }
    boot_llm_dhcp_maintenance.acquire = *request;
    boot_llm_dhcp_maintenance.armed = 1U;
    boot_llm_dhcp_maintenance.retries_used = 0U;
    boot_llm_dhcp_maintenance.retry_limit = KERNEL_LLM_DHCP_RETRY_LIMIT;
    boot_llm_dhcp_maintenance.next_retry_tick = 0U;
    kernel_llm_copy_hostname(request->hostname);
    return 0;
}

/* Appelé depuis un contexte noyau sûr ; jamais depuis le gestionnaire IRQ0. */
int kernel_llm_dhcp_maintenance(uint32_t now) {
    int status; uint32_t delay; uint8_t attempt; os_llm_acquire_start_request_t retry;
    if (!boot_llm_dhcp_maintenance.armed || !g_llm_present) return 0;
#ifndef MOHHDY_RING3
    if (!nic_owner_kernel_may_touch()) return 0; /* Tranche 5 suite: card in Ring 3 */
#endif
    if (boot_llm_lease.valid) {
        /* ne2k_dhcp_poll_ack consomme la tete du ring, DHCP ou pas. Pendant
         * SYN/TLS/HTTP ces trames ne doivent pas disparaitre. */
        if (boot_llm_socket_session.state.phase == NE2K_LLM_CONNECTION_SYN_SENT ||
            boot_llm_socket_session.state.phase == NE2K_LLM_CONNECTION_TLS_STARTED ||
            boot_llm_socket_session.state.phase == NE2K_LLM_CONNECTION_REQUEST_SENT ||
            boot_llm_socket_session.state.phase == NE2K_LLM_CONNECTION_STREAMING)
            return 0;
        status = ne2k_dhcp_renew_if_due(g_llm_dev, g_llm_io,
                                        boot_llm_dhcp_tx, sizeof(boot_llm_dhcp_tx),
                                        boot_llm_dhcp_rx, sizeof(boot_llm_dhcp_rx),
                                        boot_llm_dhcp_maintenance.acquire.xid,
                                        boot_llm_dhcp_maintenance.acquire.dhcp_attempts,
                                        now, &boot_llm_lease);
        if (status != -2) return status;
        boot_llm_application_recovery.is_sse_resume = 0U;
        boot_llm_application_recovery.event_id_length = 0U;
        if (boot_llm_application_recovery.pending && boot_llm_application_recovery.request.streaming &&
            boot_llm_sse_response.sse.event_id_valid &&
            boot_llm_sse_response.sse.event_id_length <= NET_LLM_SSE_EVENT_ID_MAX) {
            for (attempt = 0U; attempt < boot_llm_sse_response.sse.event_id_length; ++attempt)
                boot_llm_application_recovery.event_id[attempt] = boot_llm_sse_response.sse.event_id[attempt];
            boot_llm_application_recovery.event_id_length = boot_llm_sse_response.sse.event_id_length;
            boot_llm_application_recovery.is_sse_resume = 1U;
        }
        (void)kernel_llm_close_internal(1U);
        net_dhcp_lease_clear(&boot_llm_lease);
    }
    if (now < boot_llm_dhcp_maintenance.next_retry_tick) return 0;
    if (boot_llm_dhcp_maintenance.retries_used >= boot_llm_dhcp_maintenance.retry_limit) return -3;
    retry = boot_llm_dhcp_maintenance.acquire;
    retry.xid += (uint32_t)boot_llm_dhcp_maintenance.retries_used + 1U;
    status = kernel_llm_acquire_start(&retry);
    if (status == 0) return 2;
    delay = KERNEL_LLM_DHCP_RETRY_BASE_TICKS;
    for (attempt = 0U; attempt < boot_llm_dhcp_maintenance.retries_used &&
         delay < KERNEL_LLM_DHCP_RETRY_MAX_TICKS / 2U; ++attempt) delay <<= 1U;
    boot_llm_dhcp_maintenance.retries_used++;
    if (delay > KERNEL_LLM_DHCP_RETRY_MAX_TICKS) delay = KERNEL_LLM_DHCP_RETRY_MAX_TICKS;
    boot_llm_dhcp_maintenance.next_retry_tick = now + delay;
    return -2;
}

static int kernel_llm_text_field_is_valid(const char* field, uint16_t capacity) {
    uint16_t index;
    if (!field || capacity == 0U || field[0] == '\0') return 0;
    for (index = 0U; index < capacity; ++index) {
        char value = field[index];
        if (value == '\0') return 1;
        if (value < 32 || value > 126) return 0;
    }
    return 0;
}

int kernel_llm_configure_openai(const os_llm_openai_credential_request_t* request){uint16_t index;if(!request||!kernel_llm_text_field_is_valid(request->bearer,OS_LLM_BEARER_MAX))return OS_LLM_CREDENTIAL_BAD_ARGUMENT;if(boot_llm_socket_session.state.phase!=NE2K_LLM_CONNECTION_IDLE)return OS_LLM_CREDENTIAL_BAD_PHASE;kernel_llm_clear_bytes((uint8_t*)boot_llm_openai_bearer,sizeof(boot_llm_openai_bearer));for(index=0U;index<OS_LLM_BEARER_MAX;index++){boot_llm_openai_bearer[index]=request->bearer[index];if(request->bearer[index]=='\0')break;}boot_llm_openai_bearer_ready=1U;return 0;}
int kernel_llm_request(const os_llm_request_t* request) {
    int status;
    if (!request || !kernel_llm_text_field_is_valid(request->model, OS_LLM_MODEL_MAX) ||
        !kernel_llm_text_field_is_valid(request->path, OS_LLM_PATH_MAX) || request->path[0] != '/' ||
        request->prompt_length > OS_LLM_PROMPT_MAX || request->streaming > 1U ||
        (request->provider != NE2K_LLM_PROVIDER_OLLAMA && request->provider != NE2K_LLM_PROVIDER_OPENAI))
        return OS_LLM_REQUEST_BAD_REQUEST;
    if (!g_llm_present || boot_llm_socket_session.state.phase != NE2K_LLM_CONNECTION_TLS_COMPLETE)
        return OS_LLM_REQUEST_BAD_PHASE;
    if (request->provider == NE2K_LLM_PROVIDER_OPENAI && !boot_llm_openai_bearer_ready) return OS_LLM_REQUEST_UNCONFIGURED;
    if (request->streaming) {
        if (net_llm_sse_response_init(&boot_llm_sse_response, boot_llm_sse_http_buffer,
                                      sizeof(boot_llm_sse_http_buffer), boot_llm_sse_event_buffer,
                                      sizeof(boot_llm_sse_event_buffer)) != 0) return OS_LLM_REQUEST_FAILED;
        status = ne2k_llm_socket_session_request(
            g_llm_dev, g_llm_io, &boot_llm_arp_cache, boot_llm_frame, sizeof(boot_llm_frame),
            boot_llm_lease.ipv4, &boot_llm_socket_session, &boot_llm_tls_client.session,
            request->provider, 1U, boot_llm_http_json, sizeof(boot_llm_http_json),
            boot_llm_http_request, sizeof(boot_llm_http_request), boot_llm_hostname, request->path,
            request->provider == NE2K_LLM_PROVIDER_OPENAI ? boot_llm_openai_bearer : 0,
            request->model, request->prompt, request->prompt_length, boot_llm_http_tls_record,
            sizeof(boot_llm_http_tls_record), boot_llm_tcp_segment, sizeof(boot_llm_tcp_segment), 2U);
    } else {
        if (net_http_response_accumulator_init(&boot_llm_http_accumulator,
                                               boot_llm_http_response_buffer,
                                               sizeof(boot_llm_http_response_buffer)) != 0)
            return OS_LLM_REQUEST_FAILED;
        status = ne2k_llm_socket_session_request(
            g_llm_dev, g_llm_io, &boot_llm_arp_cache, boot_llm_frame, sizeof(boot_llm_frame),
            boot_llm_lease.ipv4, &boot_llm_socket_session, &boot_llm_tls_client.session,
            request->provider, 0U, boot_llm_http_json, sizeof(boot_llm_http_json),
            boot_llm_http_request, sizeof(boot_llm_http_request), boot_llm_hostname, request->path,
            request->provider == NE2K_LLM_PROVIDER_OPENAI ? boot_llm_openai_bearer : 0,
            request->model, request->prompt, request->prompt_length, boot_llm_http_tls_record,
            sizeof(boot_llm_http_tls_record), boot_llm_tcp_segment, sizeof(boot_llm_tcp_segment), 2U);
    }
    if (status < 0) return OS_LLM_REQUEST_FAILED;
    boot_llm_http_provider = request->provider;
    boot_llm_http_streaming = request->streaming;
    boot_llm_application_recovery.request = *request;
    boot_llm_application_recovery.pending = 1U;
    boot_llm_application_recovery.is_sse_resume = 0U;
    boot_llm_application_recovery.event_id_length = 0U;
    kernel_llm_clear_bytes(boot_llm_application_recovery.event_id,
                           sizeof(boot_llm_application_recovery.event_id));
    return 0;
}

int kernel_llm_poll_text(os_llm_text_result_t* result) {
    uint16_t text_length = 0U;
    uint16_t consumed = 0U;
    uint16_t index;
    int status;
    if (!result) return OS_LLM_TEXT_BAD_ARGUMENT;
    result->text_length = 0U;
    result->status_code = 0U;
    if (boot_llm_socket_session.state.phase != NE2K_LLM_CONNECTION_REQUEST_SENT || boot_llm_http_streaming)
        return OS_LLM_TEXT_BAD_PHASE;
    status = ne2k_llm_socket_session_poll_response(
        g_llm_dev, g_llm_io, &boot_llm_arp_cache, boot_llm_arp_rx, sizeof(boot_llm_arp_rx),
        boot_llm_frame, sizeof(boot_llm_frame), boot_llm_lease.ipv4, &boot_llm_socket_session,
        &boot_llm_tls_client.session, boot_llm_plaintext, sizeof(boot_llm_plaintext),
        &boot_llm_http_accumulator, &boot_llm_http_response, &consumed);
    if (status < 0) return OS_LLM_TEXT_FAILED;
    result->status_code = boot_llm_http_response.status_code;
    if (status == 0) {
        if (boot_llm_http_response.status_code < 200U || boot_llm_http_response.status_code >= 300U)
            return OS_LLM_TEXT_FAILED;
        status = boot_llm_http_provider == NE2K_LLM_PROVIDER_OLLAMA
            ? net_llm_ollama_response_extract(boot_llm_http_response.body, boot_llm_http_response.body_length,
                                              boot_llm_http_text, sizeof(boot_llm_http_text), &text_length)
            : net_llm_openai_response_extract(boot_llm_http_response.body, boot_llm_http_response.body_length,
                                              boot_llm_http_text, sizeof(boot_llm_http_text), &text_length);
        if (status < 0) return OS_LLM_TEXT_FAILED;
        status = 0;
    }
    if (text_length > OS_LLM_TEXT_MAX) return OS_LLM_TEXT_FAILED;
    for (index = 0U; index < text_length; ++index) result->text[index] = boot_llm_http_text[index];
    result->text_length = text_length;
    return status;
}

int kernel_peer_listen(const os_peer_listen_request_t* request) {
    int socket_id;
    if (!request || request->local_port == 0U) return OS_PEER_BAD_REQUEST;
    if (!g_llm_present) return OS_PEER_UNAVAILABLE;
    if (!boot_llm_lease.valid) return OS_PEER_NO_LEASE;
    if (boot_peer_listen_socket >= 0) {
        uint8_t state = 0U;
        if (net_socket_get_state(boot_peer_listen_socket, &state) == 0 &&
            (state == NET_TCP_STATE_LISTEN || state == NET_TCP_STATE_SYN_RECEIVED ||
             state == NET_TCP_STATE_ESTABLISHED))
            return OS_PEER_IN_PROGRESS;
        (void)net_socket_close(boot_peer_listen_socket);
        boot_peer_listen_socket = -1;
    }
    socket_id = net_socket_listen(request->local_port,
                                  request->local_sequence ? request->local_sequence : 0x20406080U);
    if (socket_id < 0) return OS_PEER_FAILED;
    boot_peer_listen_socket = socket_id;
    return 0;
}

int kernel_peer_accept(const os_peer_accept_request_t* request) {
    uint8_t state = 0U;
    uint16_t attempts;
    int status;
    if (!request) return OS_PEER_BAD_REQUEST;
    if (!g_llm_present) return OS_PEER_UNAVAILABLE;
    if (!boot_llm_lease.valid) return OS_PEER_NO_LEASE;
    if (boot_peer_listen_socket < 0) return OS_PEER_NOT_LISTENING;
    if (net_socket_get_state(boot_peer_listen_socket, &state) != 0) return OS_PEER_FAILED;
    if (state == NET_TCP_STATE_ESTABLISHED) return 0;
    if (state != NET_TCP_STATE_LISTEN && state != NET_TCP_STATE_SYN_RECEIVED)
        return OS_PEER_NOT_LISTENING;
    attempts = request->attempts ? request->attempts : 64U;
    status = ne2k_socket_passive_accept(
        g_llm_dev, g_llm_io, &boot_llm_arp_cache,
        boot_llm_frame, sizeof(boot_llm_frame),
        boot_llm_dhcp_tx, sizeof(boot_llm_dhcp_tx),
        boot_peer_segment, sizeof(boot_peer_segment),
        boot_llm_lease.ipv4, boot_peer_listen_socket, attempts,
        request->require_established);
    if (status == 0) return 0;
    if (status == 1) return 1; /* SYN-ACK guest emis, SYN_RECEIVED */
    if (status == -12) return OS_PEER_TIMEOUT;
    return OS_PEER_FAILED;
}

static int kernel_peer_send_record(const uint8_t* record, uint16_t record_length) {
    net_tcp_connection_t snapshot;
    uint16_t segment_length = 0U;
    int status;
    if (net_socket_connection_snapshot(boot_peer_listen_socket, &snapshot) != 0) return -1;
    status = net_socket_send_limit(boot_peer_listen_socket, record, record_length, boot_peer_segment,
                                   sizeof(boot_peer_segment), &segment_length, 2U);
    if (status != 0) return -2;
    status = ne2k_tcp_segment(g_llm_dev, g_llm_io, &boot_llm_arp_cache, boot_llm_frame,
                              sizeof(boot_llm_frame), boot_llm_lease.ipv4, boot_peer_remote_ip,
                              boot_peer_segment, segment_length);
    if (status != 0) {
        (void)net_socket_connection_restore(boot_peer_listen_socket, &snapshot);
        return -3;
    }
    return 0;
}

static int kernel_peer_capture_remote_ip(void) {
    uint16_t i;
    uint8_t candidate[4] = {10U, 32U, 0U, 15U};
    uint8_t mac[6];
    for (i = 0U; i < 4U; i++) boot_peer_remote_ip[i] = 0U;
    /* Ne pas utiliser remote_ip LLM (souvent 203.0.113.20) : viser l autre invite loue. */
    if (boot_llm_lease.ipv4[3] == 15U) candidate[3] = 16U;
    else candidate[3] = 15U;
    if (net_arp_cache_lookup(&boot_llm_arp_cache, candidate, mac) == 0) {
        for (i = 0U; i < 4U; i++) boot_peer_remote_ip[i] = candidate[i];
        return 0;
    }
    candidate[3] = (uint8_t)(candidate[3] == 16U ? 15U : 16U);
    if (net_arp_cache_lookup(&boot_llm_arp_cache, candidate, mac) == 0) {
        for (i = 0U; i < 4U; i++) boot_peer_remote_ip[i] = candidate[i];
        return 0;
    }
    return -2;
}

static int kernel_tls_app_recv(int socket_id, net_tls_aes_gcm_session_t* session,
                               uint8_t* plaintext, uint16_t plaintext_cap, uint16_t* out_len) {
    net_tcp_view_t view;
    net_tls_record_view_t opened;
    uint16_t frame_length = 0U;
    uint16_t consumed = 0U;
    int status;
    if (!plaintext || !out_len || socket_id < 0 || !session) return -1;
    *out_len = 0U;
    status = ne2k_rx_poll_tcp(g_llm_dev, g_llm_io, boot_llm_frame,
                              sizeof(boot_llm_frame), &frame_length, &view);
    if (status != 0 || view.payload_length == 0U) return -2;
    /* net_socket_receive_tls accepte deja le segment TCP. Un feed avant
     * avancerait remote_sequence et ferait rejeter ce second accept. */
    (void)frame_length;
    status = net_socket_receive_tls(socket_id, session, &view, plaintext, plaintext_cap,
                                    &opened, &consumed);
    if (status != 0 || opened.content_type != NET_TLS_CONTENT_APPLICATION_DATA) return -5;
    *out_len = opened.payload_length;
    return 0;
}

static int kernel_tls_app_send(int socket_id, net_tls_aes_gcm_session_t* session,
                               const uint8_t remote_ip[4],
                               const uint8_t* payload, uint16_t payload_length) {
    net_tcp_connection_t snapshot;
    uint16_t segment_length = 0U;
    int status;
    if (net_socket_connection_snapshot(socket_id, &snapshot) != 0) return -1;
    status = net_socket_send_tls(socket_id, session, NET_TLS_CONTENT_APPLICATION_DATA,
                                 payload, payload_length, boot_peer_tls_record,
                                 sizeof(boot_peer_tls_record), boot_peer_segment,
                                 sizeof(boot_peer_segment), &segment_length, 2U);
    if (status != 0) return -2;
    status = ne2k_tcp_segment(g_llm_dev, g_llm_io, &boot_llm_arp_cache, boot_llm_frame,
                              sizeof(boot_llm_frame), boot_llm_lease.ipv4, remote_ip,
                              boot_peer_segment, segment_length);
    if (status != 0) {
        (void)net_socket_connection_restore(socket_id, &snapshot);
        return -3;
    }
    return 0;
}

static int plaintext_starts_metier(const uint8_t* data, uint16_t length) {
    static const uint8_t mark[6] = {'M', 'E', 'T', 'I', 'E', 'R'};
    uint16_t i;
    if (!data || length < 6U) return 0;
    for (i = 0U; i < 6U; i++) if (data[i] != mark[i]) return 0;
    return 1;
}

/* 10 = METIER ok, 11 = attente, 12 = METIER facture emis. */
static int kernel_metier_exchange(void) {
    static const uint8_t facture[14] = {
        'M', 'E', 'T', 'I', 'E', 'R', ' ', 'f', 'a', 'c', 't', 'u', 'r', 'e'
    };
    static const uint8_t ok_reply[9] = {
        'M', 'E', 'T', 'I', 'E', 'R', ' ', 'o', 'k'
    };
    uint16_t rx = 0U;
    int status;
    if (!g_llm_present) return OS_PEER_UNAVAILABLE;
    if (!boot_llm_lease.valid) return OS_PEER_NO_LEASE;
    if (boot_peer_tls_step >= 7U && boot_peer_listen_socket >= 0) {
        status = kernel_tls_app_recv(boot_peer_listen_socket, &boot_peer_tls_server.session,
                                     boot_llm_plaintext, sizeof(boot_llm_plaintext), &rx);
        if (status != 0 || !plaintext_starts_metier(boot_llm_plaintext, rx)) return 11;
        if (kernel_peer_capture_remote_ip() != 0) return OS_PEER_FAILED;
        if (kernel_tls_app_send(boot_peer_listen_socket, &boot_peer_tls_server.session,
                                boot_peer_remote_ip, ok_reply, 9U) != 0)
            return OS_PEER_FAILED;
        return 10;
    }
    if (boot_llm_socket_session.state.phase != NE2K_LLM_CONNECTION_TLS_COMPLETE ||
        boot_llm_socket_session.socket_id < 0)
        return OS_PEER_NOT_LISTENING;
    if (!boot_metier_client_sent) {
        if (kernel_tls_app_send(boot_llm_socket_session.socket_id, &boot_llm_tls_client.session,
                                boot_llm_socket_session.state.remote_ip, facture, 14U) != 0)
            return OS_PEER_FAILED;
        boot_metier_client_sent = 1U;
        return 12;
    }
    status = kernel_tls_app_recv(boot_llm_socket_session.socket_id, &boot_llm_tls_client.session,
                                 boot_llm_plaintext, sizeof(boot_llm_plaintext), &rx);
    if (status != 0 || rx < 9U || !plaintext_starts_metier(boot_llm_plaintext, rx)) return 11;
    if (boot_llm_plaintext[7] == 'o' && boot_llm_plaintext[8] == 'k') return 10;
    return 11;
}

/* Retours : 1=ServerHello, 2=Certificate, 3=SKE, 4=SHD, 5=attente flight,
 * 6=CCS, 7=Finished, 8=attente applicative, 9=app echo, 0=noop.
 * request->metier : 10=METIER ok, 11=attente, 12=facture emise. */
int kernel_peer_tls_poll(const os_peer_tls_poll_request_t* request) {
    uint8_t state = 0U;
    uint16_t rx_length = 0U;
    int status;
    int built;
    uint16_t i;
    if (request && request->metier) return kernel_metier_exchange();
    if (!g_llm_present) return OS_PEER_UNAVAILABLE;
    if (!boot_llm_lease.valid) return OS_PEER_NO_LEASE;
    if (boot_peer_listen_socket < 0) return OS_PEER_NOT_LISTENING;
    if (net_socket_get_state(boot_peer_listen_socket, &state) != 0) return OS_PEER_FAILED;
    if (state != NET_TCP_STATE_ESTABLISHED) return OS_PEER_NOT_LISTENING;

    if (!boot_peer_tls_ready) {
        uint32_t word = 0U;
        if (!boot_llm_rdrand_supported) return OS_PEER_FAILED;
        for (i = 0U; i < 32U; i++) {
            if ((i & 3U) == 0U) {
                if (kernel_llm_rdrand_word(&word) != 0) return OS_PEER_FAILED;
            }
            boot_peer_server_random[i] = (uint8_t)(word >> ((i & 3U) * 8U));
            boot_peer_server_private[i] = 0U;
        }
        for (i = 0U; i < 32U; i++) {
            if ((i & 3U) == 0U) {
                if (kernel_llm_rdrand_word(&word) != 0) return OS_PEER_FAILED;
            }
            boot_peer_server_private[i] = (uint8_t)(word >> ((i & 3U) * 8U));
        }
        boot_peer_server_private[0] &= 248U;
        boot_peer_server_private[31] &= 127U;
        boot_peer_server_private[31] |= 64U;
        if (kernel_peer_capture_remote_ip() != 0) return OS_PEER_FAILED;
        if (net_tls_server_init(&boot_peer_tls_server, boot_peer_server_random,
                                boot_peer_server_private, boot_llm_x25519_workspace,
                                KERNEL_LLM_TLS_WORKSPACE_WORDS) != 0)
            return OS_PEER_FAILED;
        boot_peer_tls_ready = 1U;
        boot_peer_tls_step = 0U;
    }

    /* Drain NIC into socket while advancing. */
    for (i = 0U; i < 8U; i++) {
        status = ne2k_socket_poll_tcp(g_llm_dev, g_llm_io, boot_llm_frame,
                                      sizeof(boot_llm_frame), boot_peer_listen_socket);
        if (status != 0) break;
    }

    if (boot_peer_tls_step == 0U) {
        status = net_socket_receive(boot_peer_listen_socket, boot_peer_tls_record,
                                    sizeof(boot_peer_tls_record), &rx_length);
        if (status != 0 || rx_length == 0U) return 0;
        if (net_tls_server_accept_client_hello(&boot_peer_tls_server, boot_peer_tls_record,
                                               rx_length) != 0)
            return OS_PEER_FAILED;
        built = net_tls_server_hello_build(boot_peer_tls_record, sizeof(boot_peer_tls_record),
                                           boot_peer_server_random,
                                           NET_TLS_CIPHER_ECDHE_RSA_WITH_AES_128_GCM_SHA256);
        if (built < 0) return OS_PEER_FAILED;
        if (net_tls_server_note_handshake_message(&boot_peer_tls_server, boot_peer_tls_record,
                                                  (uint16_t)built) != 0)
            return OS_PEER_FAILED;
        if (kernel_peer_send_record(boot_peer_tls_record, (uint16_t)built) != 0) return OS_PEER_FAILED;
        boot_peer_tls_server.phase = NET_TLS_SERVER_PHASE_HELLO_SENT;
        boot_peer_tls_step = 1U;
        return 1;
    }

    if (boot_peer_tls_step == 1U) {
        built = net_tls_server_certificate_build(boot_peer_tls_record, sizeof(boot_peer_tls_record),
                                                 aos_tls_test_leaf_der, (uint16_t)AOS_TLS_TEST_LEAF_DER_LEN);
        if (built < 0) return OS_PEER_FAILED;
        if (net_tls_server_note_handshake_message(&boot_peer_tls_server, boot_peer_tls_record,
                                                  (uint16_t)built) != 0)
            return OS_PEER_FAILED;
        if (kernel_peer_send_record(boot_peer_tls_record, (uint16_t)built) != 0) return OS_PEER_FAILED;
        boot_peer_tls_step = 2U;
        return 2;
    }

    if (boot_peer_tls_step == 2U) {
        built = net_tls_server_key_exchange_ecdhe_rsa_build(
            boot_peer_tls_record, sizeof(boot_peer_tls_record),
            boot_peer_tls_server.client_random, boot_peer_server_random,
            boot_peer_tls_server.server_public, aos_tls_test_leaf_modulus,
            AOS_TLS_TEST_LEAF_MODULUS_LEN, aos_tls_test_leaf_private_exponent,
            AOS_TLS_TEST_LEAF_PRIVATE_LEN, boot_llm_rsa_workspace, KERNEL_LLM_TLS_WORKSPACE_WORDS);
        if (built < 0) return OS_PEER_FAILED;
        if (net_tls_server_note_handshake_message(&boot_peer_tls_server, boot_peer_tls_record,
                                                  (uint16_t)built) != 0)
            return OS_PEER_FAILED;
        if (kernel_peer_send_record(boot_peer_tls_record, (uint16_t)built) != 0) return OS_PEER_FAILED;
        boot_peer_tls_step = 3U;
        return 3;
    }

    if (boot_peer_tls_step == 3U) {
        built = net_tls_server_hello_done_build(boot_peer_tls_record, sizeof(boot_peer_tls_record));
        if (built < 0) return OS_PEER_FAILED;
        if (net_tls_server_note_handshake_message(&boot_peer_tls_server, boot_peer_tls_record,
                                                  (uint16_t)built) != 0)
            return OS_PEER_FAILED;
        if (kernel_peer_send_record(boot_peer_tls_record, (uint16_t)built) != 0) return OS_PEER_FAILED;
        boot_peer_tls_server.phase = NET_TLS_SERVER_PHASE_WAIT_CLIENT_FLIGHT;
        boot_peer_tls_step = 4U;
        return 4;
    }

    if (boot_peer_tls_step == 4U) {
        status = net_socket_receive(boot_peer_listen_socket, boot_peer_tls_record,
                                    sizeof(boot_peer_tls_record), &rx_length);
        if (status != 0 || rx_length == 0U) return 5;
        if (net_tls_server_accept_client_flight(
                &boot_peer_tls_server, boot_peer_tls_record, rx_length, boot_llm_x25519_workspace,
                KERNEL_LLM_TLS_WORKSPACE_WORDS, boot_llm_prf_workspace,
                sizeof(boot_llm_prf_workspace), boot_llm_plaintext,
                sizeof(boot_llm_plaintext)) != 0)
            return OS_PEER_FAILED;
        boot_peer_tls_step = 5U;
        return 5;
    }

    if (boot_peer_tls_step == 5U) {
        built = net_tls_change_cipher_spec_build(boot_peer_tls_record, sizeof(boot_peer_tls_record));
        if (built < 0) return OS_PEER_FAILED;
        if (kernel_peer_send_record(boot_peer_tls_record, (uint16_t)built) != 0) return OS_PEER_FAILED;
        boot_peer_tls_step = 6U;
        return 6;
    }

    if (boot_peer_tls_step == 6U) {
        built = net_tls_server_finished_record_build(&boot_peer_tls_server, boot_peer_tls_record,
                                                     sizeof(boot_peer_tls_record), boot_llm_prf_workspace,
                                                     sizeof(boot_llm_prf_workspace));
        if (built < 0) return OS_PEER_FAILED;
        if (kernel_peer_send_record(boot_peer_tls_record, (uint16_t)built) != 0) return OS_PEER_FAILED;
        boot_peer_tls_step = 7U;
        return 7;
    }

    if (boot_peer_tls_step == 7U) {
        /* Un record applicatif AES-GCM echo minimal si A a envoye des donnees. */
        net_tcp_view_t view;
        uint16_t frame_length = 0U;
        net_tls_record_view_t opened;
        status = ne2k_rx_poll_tcp(g_llm_dev, g_llm_io, boot_llm_frame,
                                  sizeof(boot_llm_frame), &frame_length, &view);
        if (status != 0) return 8;
        if (view.payload_length == 0U) return 8;
        if (net_socket_feed(boot_peer_listen_socket,
                            boot_llm_frame + NET_ETHERNET_HEADER_SIZE +
                                ((boot_llm_frame[NET_ETHERNET_HEADER_SIZE] & 0x0fU) * 4U),
                            (uint16_t)(frame_length - NET_ETHERNET_HEADER_SIZE -
                                       ((boot_llm_frame[NET_ETHERNET_HEADER_SIZE] & 0x0fU) * 4U))) != 0)
            return 8;
        status = net_socket_receive_tls(boot_peer_listen_socket, &boot_peer_tls_server.session, &view,
                                        boot_llm_plaintext, sizeof(boot_llm_plaintext), &opened, &rx_length);
        if (status != 0) return 8;
        if (opened.content_type != NET_TLS_CONTENT_APPLICATION_DATA) return 8;
        /* Echo "pong" */
        {
            static const uint8_t pong[4] = {'p', 'o', 'n', 'g'};
            uint16_t segment_length = 0U;
            net_tcp_connection_t snapshot;
            if (net_socket_connection_snapshot(boot_peer_listen_socket, &snapshot) != 0) return OS_PEER_FAILED;
            status = net_socket_send_tls(boot_peer_listen_socket, &boot_peer_tls_server.session,
                                         NET_TLS_CONTENT_APPLICATION_DATA, pong, 4U, boot_peer_tls_record,
                                         sizeof(boot_peer_tls_record), boot_peer_segment,
                                         sizeof(boot_peer_segment), &segment_length, 2U);
            if (status != 0) return OS_PEER_FAILED;
            if (ne2k_tcp_segment(g_llm_dev, g_llm_io, &boot_llm_arp_cache, boot_llm_frame,
                                 sizeof(boot_llm_frame), boot_llm_lease.ipv4, boot_peer_remote_ip,
                                 boot_peer_segment, segment_length) != 0) {
                (void)net_socket_connection_restore(boot_peer_listen_socket, &snapshot);
                return OS_PEER_FAILED;
            }
            boot_peer_app_seen = 1U;
            boot_peer_tls_step = 8U;
            return 9;
        }
    }
    return 8;
}

static void kernel_llm_clear_bytes(uint8_t* buffer, uint32_t length) {
    uint32_t index;
    if (!buffer) return;
    for (index = 0U; index < length; ++index) buffer[index] = 0U;
}

static void kernel_llm_clear_session_preserve_lease(uint8_t preserve_provider) {
    net_dhcp_lease_t retained_lease = boot_llm_lease;
    kernel_llm_clear_bytes(boot_llm_dhcp_tx, sizeof(boot_llm_dhcp_tx));
    kernel_llm_clear_bytes(boot_llm_dhcp_rx, sizeof(boot_llm_dhcp_rx));
    kernel_llm_clear_bytes(boot_llm_arp_request, sizeof(boot_llm_arp_request));
    kernel_llm_clear_bytes(boot_llm_arp_rx, sizeof(boot_llm_arp_rx));
    kernel_llm_clear_bytes(boot_llm_frame, sizeof(boot_llm_frame));
    kernel_llm_clear_bytes(boot_llm_tls_record, sizeof(boot_llm_tls_record));
    kernel_llm_clear_bytes(boot_llm_tls_handshake, sizeof(boot_llm_tls_handshake));
    kernel_llm_clear_bytes(boot_llm_tls_transcript, sizeof(boot_llm_tls_transcript));
    kernel_llm_clear_bytes(boot_llm_tls_hello, sizeof(boot_llm_tls_hello));
    kernel_llm_clear_bytes((uint8_t*)boot_llm_rsa_workspace, sizeof(boot_llm_rsa_workspace));
    kernel_llm_clear_bytes((uint8_t*)boot_llm_x25519_workspace, sizeof(boot_llm_x25519_workspace));
    kernel_llm_clear_bytes(boot_llm_prf_workspace, sizeof(boot_llm_prf_workspace));
    kernel_llm_clear_bytes(boot_llm_tcp_segment, sizeof(boot_llm_tcp_segment));
    kernel_llm_clear_bytes(boot_llm_flight_records, sizeof(boot_llm_flight_records));
    kernel_llm_clear_bytes(boot_llm_plaintext, sizeof(boot_llm_plaintext));
    kernel_llm_clear_bytes(boot_llm_http_json, sizeof(boot_llm_http_json));
    kernel_llm_clear_bytes(boot_llm_http_request, sizeof(boot_llm_http_request));
    kernel_llm_clear_bytes(boot_llm_http_tls_record, sizeof(boot_llm_http_tls_record));
    kernel_llm_clear_bytes(boot_llm_http_response_buffer, sizeof(boot_llm_http_response_buffer));
    kernel_llm_clear_bytes(boot_llm_http_text, sizeof(boot_llm_http_text));
    kernel_llm_clear_bytes(boot_llm_sse_http_buffer, sizeof(boot_llm_sse_http_buffer));
    kernel_llm_clear_bytes(boot_llm_sse_event_buffer, sizeof(boot_llm_sse_event_buffer));
    kernel_llm_clear_bytes((uint8_t*)&boot_llm_tls_client, sizeof(boot_llm_tls_client));
    kernel_llm_clear_bytes((uint8_t*)&boot_llm_socket_session, sizeof(boot_llm_socket_session));
    kernel_llm_clear_bytes((uint8_t*)&boot_llm_http_accumulator, sizeof(boot_llm_http_accumulator));
    kernel_llm_clear_bytes((uint8_t*)&boot_llm_http_response, sizeof(boot_llm_http_response));
    kernel_llm_clear_bytes((uint8_t*)&boot_llm_sse_response, sizeof(boot_llm_sse_response));
    kernel_llm_clear_bytes((uint8_t*)boot_llm_hostname, sizeof(boot_llm_hostname));
    if (!preserve_provider) {
        kernel_llm_clear_bytes((uint8_t*)boot_llm_openai_bearer, sizeof(boot_llm_openai_bearer));
        boot_llm_openai_bearer_ready = 0U;
        kernel_llm_clear_bytes((uint8_t*)&boot_llm_application_recovery,
                               sizeof(boot_llm_application_recovery));
    }
    kernel_llm_clear_tls_material();
    boot_llm_lease = retained_lease;
    (void)ne2k_llm_socket_session_init(&boot_llm_socket_session);
    (void)net_arp_cache_init(&boot_llm_arp_cache);
    (void)ne2k_tls_client_init(&boot_llm_tls_client, boot_llm_tls_record, sizeof(boot_llm_tls_record),
                               boot_llm_tls_handshake, sizeof(boot_llm_tls_handshake),
                               boot_llm_tls_transcript, sizeof(boot_llm_tls_transcript));
    boot_llm_flight_records_length = 0U;
    boot_llm_http_provider = NE2K_LLM_PROVIDER_OLLAMA;
    boot_llm_http_streaming = 0U;
}

static int kernel_llm_close_internal(uint8_t preserve_provider) {
    uint8_t fin_failed = 0U, socket_state = NET_TCP_STATE_CLOSED;
    if (boot_llm_socket_session.state.phase == NE2K_LLM_CONNECTION_IDLE)
        return OS_LLM_CLOSE_BAD_PHASE;
    if (boot_llm_socket_session.socket_id >= 0 &&
        net_socket_get_state(boot_llm_socket_session.socket_id, &socket_state) == 0 &&
        socket_state == NET_TCP_STATE_ESTABLISHED) {
        if (!g_llm_present || !boot_llm_lease.valid) fin_failed = 1U;
        else {
            if (boot_llm_tls_client.complete && net_tls_handshake_is_complete(&boot_llm_tls_client.handshake) &&
                ne2k_socket_tls_close_notify(g_llm_dev, g_llm_io, &boot_llm_arp_cache,
                                             boot_llm_frame, sizeof(boot_llm_frame), boot_llm_lease.ipv4,
                                             boot_llm_socket_session.state.remote_ip,
                                             boot_llm_socket_session.socket_id, &boot_llm_tls_client,
                                             boot_llm_http_tls_record, sizeof(boot_llm_http_tls_record), 2U) < 0)
                fin_failed = 1U;
            if (ne2k_socket_fin(g_llm_dev, g_llm_io, &boot_llm_arp_cache,
                                boot_llm_frame, sizeof(boot_llm_frame), boot_llm_lease.ipv4,
                                boot_llm_socket_session.state.remote_ip,
                                boot_llm_socket_session.socket_id) != 0) fin_failed = 1U;
        }
    }
    if (boot_llm_socket_session.socket_id >= 0)
        (void)net_socket_close(boot_llm_socket_session.socket_id);
    kernel_llm_clear_session_preserve_lease(preserve_provider);
    return fin_failed ? OS_LLM_CLOSE_FIN_FAILED : 0;
}

int kernel_llm_close(void) {
    return kernel_llm_close_internal(0U);
}

int kernel_llm_reset_for_request(void) {
    if (boot_llm_socket_session.state.phase != NE2K_LLM_CONNECTION_RESPONSE_READY)
        return OS_LLM_RESET_BAD_PHASE;
    if (ne2k_llm_socket_session_reset_for_request(&boot_llm_socket_session) != 0)
        return OS_LLM_RESET_FAILED;
    boot_llm_http_streaming = 0U;
    boot_llm_http_provider = NE2K_LLM_PROVIDER_OLLAMA;
    if (net_http_response_accumulator_init(&boot_llm_http_accumulator,
                                           boot_llm_http_response_buffer,
                                           sizeof(boot_llm_http_response_buffer)) != 0)
        return OS_LLM_RESET_FAILED;
    if (net_llm_sse_response_init(&boot_llm_sse_response, boot_llm_sse_http_buffer,
                                  sizeof(boot_llm_sse_http_buffer), boot_llm_sse_event_buffer,
                                  sizeof(boot_llm_sse_event_buffer)) != 0)
        return OS_LLM_RESET_FAILED;
    boot_llm_http_response.status_code = 0U;
    boot_llm_http_response.body = 0;
    boot_llm_http_response.body_length = 0U;
    boot_llm_http_response.header_length = 0U;
    kernel_llm_clear_bytes(boot_llm_http_text, sizeof(boot_llm_http_text));
    boot_llm_application_recovery.pending = 0U;
    boot_llm_application_recovery.is_sse_resume = 0U;
    boot_llm_application_recovery.event_id_length = 0U;
    kernel_llm_clear_bytes(boot_llm_application_recovery.event_id,
                           sizeof(boot_llm_application_recovery.event_id));
    return 0;
}

int kernel_llm_poll_sse(os_llm_text_result_t* result) {
    uint16_t text_length = 0U;
    uint16_t consumed = 0U;
    uint16_t index;
    int status;
    if (!result) return OS_LLM_SSE_BAD_ARGUMENT;
    result->text_length = 0U;
    result->status_code = 0U;
    if (!boot_llm_http_streaming ||
        (boot_llm_socket_session.state.phase != NE2K_LLM_CONNECTION_REQUEST_SENT &&
         boot_llm_socket_session.state.phase != NE2K_LLM_CONNECTION_STREAMING)) return OS_LLM_SSE_BAD_PHASE;
    status = ne2k_llm_socket_session_poll_sse(
        g_llm_dev, g_llm_io, &boot_llm_arp_cache, boot_llm_arp_rx, sizeof(boot_llm_arp_rx),
        boot_llm_frame, sizeof(boot_llm_frame), boot_llm_lease.ipv4, &boot_llm_socket_session,
        &boot_llm_tls_client.session, boot_llm_plaintext, sizeof(boot_llm_plaintext), &boot_llm_sse_response,
        boot_llm_http_provider, boot_llm_http_text, sizeof(boot_llm_http_text), &text_length, &consumed);
    if (status < 0) return OS_LLM_SSE_FAILED;
    result->status_code = boot_llm_sse_response.http.status_code;
    if (text_length > OS_LLM_TEXT_MAX) return OS_LLM_SSE_FAILED;
    for (index = 0U; index < text_length; ++index) result->text[index] = boot_llm_http_text[index];
    result->text_length = text_length;
    return status;
}

int kernel_llm_poll_tls(void) {
    uint16_t consumed = 0U;
    char utc_time[RTC_UTC_BUFFER_LENGTH];
    int status;
    if (!g_llm_present) return OS_LLM_ACQUIRE_UNAVAILABLE;
    if (boot_llm_socket_session.state.phase != NE2K_LLM_CONNECTION_SYN_SENT &&
        boot_llm_socket_session.state.phase != NE2K_LLM_CONNECTION_TLS_STARTED) return OS_LLM_TLS_BAD_PHASE;
    /* Aucune clé éphémère faible ni ancre vide ne doit initier un ClientHello. */
    if (!boot_llm_tls_material_ready) return OS_LLM_TLS_UNCONFIGURED;
    if (boot_llm_socket_session.state.phase == NE2K_LLM_CONNECTION_SYN_SENT) {
        status = ne2k_llm_socket_session_poll_tls_start(
            g_llm_dev, g_llm_io, &boot_llm_arp_cache,
            boot_llm_arp_rx, sizeof(boot_llm_arp_rx), boot_llm_frame, sizeof(boot_llm_frame),
            boot_llm_lease.ipv4, &boot_llm_socket_session, &boot_llm_tls_client, boot_llm_client_random,
            boot_llm_tls_hello, sizeof(boot_llm_tls_hello), boot_llm_tcp_segment,
            sizeof(boot_llm_tcp_segment), 2U);
        /* Le bridge retourne la longueur du ClientHello ; l’ABI publique
         * publie un succès normalisé lorsque la phase TLS est effectivement démarrée. */
        return status < 0 ? OS_LLM_TLS_FAILED : 0;
    }
    if (net_llm_client_utc(&boot_llm_rtc_io, utc_time, sizeof(utc_time)) != 0) return OS_LLM_TLS_FAILED;
    status = ne2k_llm_socket_session_poll_tls(
        g_llm_dev, g_llm_io, &boot_llm_arp_cache,
        boot_llm_arp_rx, sizeof(boot_llm_arp_rx), boot_llm_frame, sizeof(boot_llm_frame),
        boot_llm_lease.ipv4, &boot_llm_socket_session, &boot_llm_tls_client, boot_llm_client_random,
        boot_llm_client_private, kernel_llm_select_trust_anchor(), kernel_llm_identity_hostname(), utc_time,
        boot_llm_rsa_workspace, KERNEL_LLM_TLS_WORKSPACE_WORDS,
        boot_llm_x25519_workspace, KERNEL_LLM_TLS_WORKSPACE_WORDS, boot_llm_prf_workspace,
        sizeof(boot_llm_prf_workspace), boot_llm_tcp_segment, sizeof(boot_llm_tcp_segment),
        boot_llm_flight_records, sizeof(boot_llm_flight_records), &boot_llm_flight_records_length,
        boot_llm_plaintext, sizeof(boot_llm_plaintext), 2U, &consumed);
    if (status < 0) return OS_LLM_TLS_FAILED;
    if (status == 0 && boot_llm_socket_session.state.phase == NE2K_LLM_CONNECTION_TLS_COMPLETE &&
        boot_llm_application_recovery.pending) {
        if (!boot_llm_application_recovery.is_sse_resume) {
            status = kernel_llm_request(&boot_llm_application_recovery.request);
            return status == 0 ? 2 : OS_LLM_REQUEST_FAILED;
        }
        if (net_llm_sse_response_init(&boot_llm_sse_response, boot_llm_sse_http_buffer,
                                      sizeof(boot_llm_sse_http_buffer), boot_llm_sse_event_buffer,
                                      sizeof(boot_llm_sse_event_buffer)) != 0)
            return OS_LLM_REQUEST_FAILED;
        boot_llm_sse_response.sse.event_id_length = boot_llm_application_recovery.event_id_length;
        boot_llm_sse_response.sse.event_id_valid = 1U;
        for (consumed = 0U; consumed < boot_llm_application_recovery.event_id_length; ++consumed)
            boot_llm_sse_response.sse.event_id[consumed] = boot_llm_application_recovery.event_id[consumed];
        status = ne2k_llm_socket_session_resume_sse(
            g_llm_dev, g_llm_io, &boot_llm_arp_cache, boot_llm_frame, sizeof(boot_llm_frame),
            boot_llm_lease.ipv4, &boot_llm_socket_session, &boot_llm_tls_client.session,
            boot_llm_http_request, sizeof(boot_llm_http_request), boot_llm_hostname,
            boot_llm_application_recovery.request.path, &boot_llm_sse_response, boot_llm_http_tls_record,
            sizeof(boot_llm_http_tls_record), boot_llm_tcp_segment, sizeof(boot_llm_tcp_segment), 2U);
        if (status < 0) return OS_LLM_REQUEST_FAILED;
        boot_llm_http_provider = boot_llm_application_recovery.request.provider;
        boot_llm_http_streaming = 1U;
        return 2;
    }
    return status;
}
