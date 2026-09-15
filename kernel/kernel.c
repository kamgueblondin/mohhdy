#include "gdt.h"
#include "idt.h"
#include "interrupts.h"
#include "multiboot.h"
#include "mem/pmm.h"
#include "mem/vmm.h"
#include "task/task.h"
#include "timer.h"
#include "syscall/syscall.h"
#include "elf.h"
#include "../fs/initrd.h"
#include "../fs/overlay.h"
#include "ata.h"
#include "fs/fat16.h"
#include "fs/fat32.h"
#include "llm/gpt2_model.h"
#include "llm/gpt2_gguf.h"
#include "llm/gpt2_gguf_infer.h"
#include "llm/gpt2_infer.h"
#include "llm/gpt2_tokenizer.h"
#include "keyboard.h"
#include "service_registry.h"
#include "vga_console.h"
#include "ne2k.h"
#include "net_socket.h"
#include "tls_trust_anchor.h"
#include "tls_test_trust_anchor.h"
#include "ecdsa_p256.h"
#include <stddef.h>

// Function to read a byte from a port
unsigned char inb(unsigned short port);
// Function to write a byte to a port
void outb(unsigned short port, unsigned char data);
// Function to print string to serial port (forward declaration)
void print_string_serial(const char* str);
void print_string(const char* str);

#define KERNEL_LLM_FRAME_CAPACITY NE2K_ETHERNET_MAX_FRAME
#define KERNEL_LLM_TLS_RECORD_CAPACITY 8192U
#define KERNEL_LLM_TLS_HELLO_CAPACITY 512U
/* Une même zone caller-owned sert à RSA ou ECDSA ; P-256 impose 2 048 mots. */
#define KERNEL_LLM_TLS_WORKSPACE_WORDS ECDSA_P256_WORKSPACE_WORDS

static ne2k_device_t boot_ne2k_device;
static ne2k_io_t boot_ne2k_io;
/* Contexte persistant appartenant au noyau ; le seul secret est le bearer borné et effaçable ci-dessous. */
static net_dhcp_lease_t boot_llm_lease;
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
static net_arp_cache_t boot_llm_arp_cache;
static uint8_t boot_llm_dhcp_tx[KERNEL_LLM_FRAME_CAPACITY];
static uint8_t boot_llm_dhcp_rx[KERNEL_LLM_FRAME_CAPACITY];
static uint8_t boot_llm_arp_request[KERNEL_LLM_FRAME_CAPACITY];
static uint8_t boot_llm_arp_rx[KERNEL_LLM_FRAME_CAPACITY];
static uint8_t boot_llm_frame[KERNEL_LLM_FRAME_CAPACITY];
/* Matériaux, client TLS et buffers réservés au noyau ; aucun n’est accessible via syscall. */
static ne2k_tls_client_t boot_llm_tls_client;
static x509_certificate_view_t boot_llm_trust_anchor;
static uint8_t boot_llm_trust_anchor_ready;
static x509_certificate_view_t boot_llm_test_trust_anchor;
static uint8_t boot_llm_test_trust_anchor_ready;
static rtc_io_t boot_llm_rtc_io;
static char boot_llm_hostname[OS_LLM_HOSTNAME_MAX];
static char boot_llm_openai_bearer[OS_LLM_BEARER_MAX];
static uint8_t boot_llm_openai_bearer_ready;
static uint8_t boot_llm_client_random[NET_TLS_X25519_KEY_LENGTH];
static uint8_t boot_llm_client_private[NET_TLS_X25519_KEY_LENGTH];
static uint8_t boot_llm_rdrand_supported;
static uint8_t boot_llm_tls_entropy_ready;
static uint8_t boot_llm_tls_material_ready;
static uint8_t boot_llm_tls_record[KERNEL_LLM_TLS_RECORD_CAPACITY];
static uint8_t boot_llm_tls_handshake[KERNEL_LLM_TLS_RECORD_CAPACITY];
static uint8_t boot_llm_tls_transcript[KERNEL_LLM_TLS_RECORD_CAPACITY];
static uint8_t boot_llm_tls_hello[KERNEL_LLM_TLS_HELLO_CAPACITY];
static uint32_t boot_llm_rsa_workspace[KERNEL_LLM_TLS_WORKSPACE_WORDS];
static uint32_t boot_llm_x25519_workspace[KERNEL_LLM_TLS_WORKSPACE_WORDS];
static uint8_t boot_llm_prf_workspace[KERNEL_LLM_TLS_RECORD_CAPACITY];
static uint8_t boot_llm_tcp_segment[KERNEL_LLM_TLS_RECORD_CAPACITY];
static uint8_t boot_llm_flight_records[KERNEL_LLM_TLS_RECORD_CAPACITY];
static uint32_t boot_llm_flight_records_length;
static uint8_t boot_llm_plaintext[KERNEL_LLM_TLS_RECORD_CAPACITY];
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
static uint8_t boot_ne2k_present;
static void kernel_llm_clear_bytes(uint8_t* buffer, uint32_t length);
static int kernel_llm_rdrand_supported(void);
static int kernel_llm_close_internal(uint8_t preserve_provider);
int kernel_llm_close(void);
void ne2k_irq_handler(void) { ne2k_irq_service(); }

static void ne2k_boot_probe(void) {
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
    boot_ne2k_present = 0U;
    if (ne2k_i386_io(&boot_ne2k_io) != 0) return;
    if (ne2k_probe(&boot_ne2k_device, 0x300U, &boot_ne2k_io) != 0) {
        print_string("NE2000 ISA absent; reseau reste desactive.\\n");
        return;
    }
    if (ne2k_prepare(&boot_ne2k_device, &boot_ne2k_io) != 0 ||
        ne2k_read_mac(&boot_ne2k_device, &boot_ne2k_io) != 0 ||
        ne2k_configure_rings(&boot_ne2k_device, &boot_ne2k_io) != 0) {
        print_string("NE2000 detecte mais initialisation ou MAC incomplete.\\n");
        return;
    }
    if (ne2k_irq_attach(&boot_ne2k_device, &boot_ne2k_io) != 0) {
        print_string("NE2000 detecte mais IRQ non attachee.\\n");
        return;
    }
    boot_ne2k_present = 1U;
    print_string("NE2000 ISA detecte, MAC valide et anneaux RX/TX configures.\\n");
    print_string(boot_llm_test_trust_anchor_ready ?
        "Ancre TLS de test locale prete (example.com).\n" :
        "Ancre TLS de test locale indisponible.\n");
}

uint32_t kernel_net_status(void) {
    return boot_ne2k_present ? 3U : 0U;
}

static int kernel_llm_rdrand_supported(void) {
    uint32_t eax, ebx, ecx, edx;
    __asm__ volatile("cpuid" : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx) : "a"(1U), "c"(0U));
    (void)eax;
    (void)ebx;
    (void)edx;
    return (ecx & (1U << 30)) != 0U;
}

static int kernel_llm_rdrand_word(uint32_t* output) {
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
    return (boot_ne2k_present ? 1U : 0U) |
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
         kernel_llm_hostname_has_suffix(hostname, "example.com")))
        return &boot_llm_test_trust_anchor;
    return &boot_llm_trust_anchor;
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
    if (!boot_ne2k_present) return OS_LLM_ACQUIRE_UNAVAILABLE;
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
        &boot_ne2k_device, &boot_ne2k_io, &boot_llm_arp_cache,
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
    if (!boot_llm_dhcp_maintenance.armed || !boot_ne2k_present) return 0;
    if (boot_llm_lease.valid) {
        /* ne2k_dhcp_poll_ack consomme la tete du ring, DHCP ou pas. Pendant
         * SYN/TLS/HTTP ces trames ne doivent pas disparaitre. */
        if (boot_llm_socket_session.state.phase == NE2K_LLM_CONNECTION_SYN_SENT ||
            boot_llm_socket_session.state.phase == NE2K_LLM_CONNECTION_TLS_STARTED ||
            boot_llm_socket_session.state.phase == NE2K_LLM_CONNECTION_REQUEST_SENT ||
            boot_llm_socket_session.state.phase == NE2K_LLM_CONNECTION_STREAMING)
            return 0;
        status = ne2k_dhcp_renew_if_due(&boot_ne2k_device, &boot_ne2k_io,
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
    if (!boot_ne2k_present || boot_llm_socket_session.state.phase != NE2K_LLM_CONNECTION_TLS_COMPLETE)
        return OS_LLM_REQUEST_BAD_PHASE;
    if (request->provider == NE2K_LLM_PROVIDER_OPENAI && !boot_llm_openai_bearer_ready) return OS_LLM_REQUEST_UNCONFIGURED;
    if (request->streaming) {
        if (net_llm_sse_response_init(&boot_llm_sse_response, boot_llm_sse_http_buffer,
                                      sizeof(boot_llm_sse_http_buffer), boot_llm_sse_event_buffer,
                                      sizeof(boot_llm_sse_event_buffer)) != 0) return OS_LLM_REQUEST_FAILED;
        status = ne2k_llm_socket_session_request(
            &boot_ne2k_device, &boot_ne2k_io, &boot_llm_arp_cache, boot_llm_frame, sizeof(boot_llm_frame),
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
            &boot_ne2k_device, &boot_ne2k_io, &boot_llm_arp_cache, boot_llm_frame, sizeof(boot_llm_frame),
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
        &boot_ne2k_device, &boot_ne2k_io, &boot_llm_arp_cache, boot_llm_arp_rx, sizeof(boot_llm_arp_rx),
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
        if (!boot_ne2k_present || !boot_llm_lease.valid) fin_failed = 1U;
        else {
            if (boot_llm_tls_client.complete && net_tls_handshake_is_complete(&boot_llm_tls_client.handshake) &&
                ne2k_socket_tls_close_notify(&boot_ne2k_device, &boot_ne2k_io, &boot_llm_arp_cache,
                                             boot_llm_frame, sizeof(boot_llm_frame), boot_llm_lease.ipv4,
                                             boot_llm_socket_session.state.remote_ip,
                                             boot_llm_socket_session.socket_id, &boot_llm_tls_client,
                                             boot_llm_http_tls_record, sizeof(boot_llm_http_tls_record), 2U) < 0)
                fin_failed = 1U;
            if (ne2k_socket_fin(&boot_ne2k_device, &boot_ne2k_io, &boot_llm_arp_cache,
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
        &boot_ne2k_device, &boot_ne2k_io, &boot_llm_arp_cache, boot_llm_arp_rx, sizeof(boot_llm_arp_rx),
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
    if (!boot_ne2k_present) return OS_LLM_ACQUIRE_UNAVAILABLE;
    if (boot_llm_socket_session.state.phase != NE2K_LLM_CONNECTION_SYN_SENT &&
        boot_llm_socket_session.state.phase != NE2K_LLM_CONNECTION_TLS_STARTED) return OS_LLM_TLS_BAD_PHASE;
    /* Aucune clé éphémère faible ni ancre vide ne doit initier un ClientHello. */
    if (!boot_llm_tls_material_ready) return OS_LLM_TLS_UNCONFIGURED;
    if (boot_llm_socket_session.state.phase == NE2K_LLM_CONNECTION_SYN_SENT) {
        status = ne2k_llm_socket_session_poll_tls_start(
            &boot_ne2k_device, &boot_ne2k_io, &boot_llm_arp_cache,
            boot_llm_arp_rx, sizeof(boot_llm_arp_rx), boot_llm_frame, sizeof(boot_llm_frame),
            boot_llm_lease.ipv4, &boot_llm_socket_session, &boot_llm_tls_client, boot_llm_client_random,
            boot_llm_tls_hello, sizeof(boot_llm_tls_hello), boot_llm_tcp_segment,
            sizeof(boot_llm_tcp_segment), 2U);
        /* Le bridge retourne la longueur du ClientHello ; l’ABI publique
         * publie un succès normalisé lorsque la phase TLS est effectivement démarrée. */
        return status < 0 ? OS_LLM_TLS_FAILED : 0;
    }
    if (rtc_i386_io(&boot_llm_rtc_io) != 0 ||
        rtc_read_utc(&boot_llm_rtc_io, utc_time, sizeof(utc_time)) != 0) return OS_LLM_TLS_FAILED;
    status = ne2k_llm_socket_session_poll_tls(
        &boot_ne2k_device, &boot_ne2k_io, &boot_llm_arp_cache,
        boot_llm_arp_rx, sizeof(boot_llm_arp_rx), boot_llm_frame, sizeof(boot_llm_frame),
        boot_llm_lease.ipv4, &boot_llm_socket_session, &boot_llm_tls_client, boot_llm_client_random,
        boot_llm_client_private, kernel_llm_select_trust_anchor(), kernel_llm_session_hostname(), utc_time,
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
            &boot_ne2k_device, &boot_ne2k_io, &boot_llm_arp_cache, boot_llm_frame, sizeof(boot_llm_frame),
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

#define FAT16_ATA_READ_WINDOW_SECTORS 16U
static uint8_t fat16_ata_read_window[FAT16_ATA_READ_WINDOW_SECTORS * 512U];

static int fat16_ata_read_sector(uint32_t lba, void* buffer) {
    return ata_read_sectors(lba, 1U, buffer);
}

static int fat16_ata_read_sectors(uint32_t lba, uint32_t count, void* buffer) {
    return ata_read_sectors(lba, count, buffer);
}

static int fat16_ata_write_sector(uint32_t lba, const void* buffer) {
    return ata_write_sectors(lba, 1U, buffer);
}

static int fat32_ata_slave_read_sector(uint32_t lba, void* buffer) {
    return ata_read_sectors_drive(ATA_DRIVE_SLAVE, lba, 1U, buffer);
}

static int fat32_ata_slave_write_sector(uint32_t lba, const void* buffer) {
    return ata_write_sectors_drive(ATA_DRIVE_SLAVE, lba, 1U, buffer);
}

void serial_init() {
    // Disable all interrupts
    outb(0x3F8 + 1, 0x00);
    // Enable DLAB (set baud rate divisor)
    outb(0x3F8 + 3, 0x80);
    // Set baud rate to 38400 (divisor = 3)
    outb(0x3F8 + 0, 0x03);
    outb(0x3F8 + 1, 0x00);
    // Disable DLAB, set 8 data bits, 1 stop bit, no parity
    outb(0x3F8 + 3, 0x03);
    // Enable FIFO, clear them, with 14-byte threshold
    outb(0x3F8 + 2, 0xC7);
    // IRQs enabled, RTS/DSR set
    outb(0x3F8 + 4, 0x0B);
}

int is_transmit_empty() {
    return inb(0x3F8 + 5) & 0x20;
}

void write_serial(char a) {
    while (!is_transmit_empty());
    outb(0x3F8, a);
}

// Fonction pour vérifier si des données sont disponibles en lecture sur le port série
int is_receive_ready() {
    return inb(0x3F8 + 5) & 0x01;
}

// Fonction pour lire un caractère depuis le port série (non-bloquante)
char read_serial() {
    if (is_receive_ready()) {
        return inb(0x3F8);
    }
    return 0; // Aucun caractère disponible
}

// Function to read a byte from a port
unsigned char inb(unsigned short port) {
    unsigned char ret;
    asm volatile ("inb %1, %0" : "=a"(ret) : "dN"(port));
    return ret;
}

// Function to write a byte to a port
void outb(unsigned short port, unsigned char data) {
    asm volatile ("outb %0, %1" : : "a"(data), "dN"(port));
}

// Fonction pic_send_eoi définie dans interrupts.c
extern void pic_send_eoi(unsigned char irq);

// Pointeur vers la mémoire vidéo VGA. L'adresse 0xB8000 est standard.
volatile unsigned short* vga_buffer = (unsigned short*)0xB8000;
// Position actuelle du curseur
int vga_x = 0;
int vga_y = 0;

void scroll_screen() {
    vga_console_scroll();
}

void print_char_vga(char c, int x, int y, char color) {
    vga_console_put_xy(c, x, y, color);
}

// Définir les états pour le parseur de codes ANSI
typedef enum {
    NORMAL,
    ESCAPE,
    BRACKET,
    PARAM
} AnsiState;

// Variables statiques pour conserver l'état du parseur
static AnsiState ansi_state = NORMAL;
static char ansi_buffer[16];
static int ansi_pos = 0;
static char current_color = 0x0F; // Blanc sur noir par défaut

#if CONFIG_UTF8_VGA
// Suivi minimal UTF-8 pour l'affichage VGA (le port série reste octet-par-octet)
static int utf8_expected_continuations = 0;
static unsigned int utf8_codepoint = 0;

static int unicode_to_cp437(unsigned int cp, char* out) {
    switch (cp) {
        case 0x2500: *out = (char)0xC4; return 1; // ─
        case 0x2502: *out = (char)0xB3; return 1; // │
        case 0x250C: *out = (char)0xDA; return 1; // ┌
        case 0x2510: *out = (char)0xBF; return 1; // ┐
        case 0x2514: *out = (char)0xC0; return 1; // └
        case 0x2518: *out = (char)0xD9; return 1; // ┘
        case 0x251C: *out = (char)0xC3; return 1; // ├
        case 0x2524: *out = (char)0xB4; return 1; // ┤
        case 0x252C: *out = (char)0xC2; return 1; // ┬
        case 0x2534: *out = (char)0xC1; return 1; // ┴
        case 0x253C: *out = (char)0xC5; return 1; // ┼
        case 0x2550: *out = (char)0xCD; return 1; // ═
        case 0x2551: *out = (char)0xBA; return 1; // ║
        case 0x2554: *out = (char)0xC9; return 1; // ╔
        case 0x2557: *out = (char)0xBB; return 1; // ╗
        case 0x255A: *out = (char)0xC8; return 1; // ╚
        case 0x255D: *out = (char)0xBC; return 1; // ╝
        case 0x2560: *out = (char)0xCC; return 1; // ╠
        case 0x2563: *out = (char)0xB9; return 1; // ╣
        case 0x2566: *out = (char)0xCB; return 1; // ╦
        case 0x2569: *out = (char)0xCA; return 1; // ╩
        case 0x256C: *out = (char)0xCE; return 1; // ╬
        case 0x2591: *out = (char)0xB0; return 1; // ░
        case 0x2592: *out = (char)0xB1; return 1; // ▒
        case 0x2593: *out = (char)0xB2; return 1; // ▓
        case 0x2588: *out = (char)0xDB; return 1; // █
        case 0x2013: case 0x2014: *out = '-'; return 1; // – —
        case 0x00E9: case 0x00E8: case 0x00EA: case 0x00EB: *out = 'e'; return 1; // é è ê ë
        case 0x00E0: case 0x00E1: case 0x00E2: case 0x00E4: *out = 'a'; return 1; // à á â ä
        case 0x00E7: *out = 'c'; return 1; // ç
        case 0x00F1: *out = 'n'; return 1; // ñ
        case 0x00FC: case 0x00F9: case 0x00FA: *out = 'u'; return 1; // ü ù ú
        case 0x00F6: case 0x00F3: case 0x00F4: *out = 'o'; return 1; // ö ó ô
        case 0x00ED: case 0x00EF: case 0x00EC: *out = 'i'; return 1; // í ï ì
        case 0x00C9: *out = 'E'; return 1; // É
        case 0x00C7: *out = 'C'; return 1; // Ç
        case 0x00D1: *out = 'N'; return 1; // Ñ
        case 0x00DC: *out = 'U'; return 1; // Ü
        case 0x00C0: *out = 'A'; return 1; // À
        default: return 0;
    }
}
#endif

void clear_screen_vga() {
    vga_console_clear(current_color);
    vga_x = 0;
    vga_y = 0;
    vga_console_set_cursor(vga_x, vga_y);
}

// Fonction pour parser les paramètres numériques des codes ANSI
int ansi_parse_param() {
    int val = 0;
    for (int i = 0; i < ansi_pos; i++) {
        val = val * 10 + (ansi_buffer[i] - '0');
    }
    return val;
}


// Remplace l'ancienne fonction print_char par celle-ci
void print_char(char c, int x, int y, char color) {
    if (ansi_state == NORMAL) {
#if CONFIG_UTF8_VGA
        // Décodage UTF-8 minimal et rendu VGA via CP437
        unsigned char uc = (unsigned char)c;
        if (uc == '\x1b') {
            // traité par la machine ANSI plus bas
        } else if (utf8_expected_continuations > 0) {
            if ((uc & 0xC0) == 0x80) {
                utf8_codepoint = (utf8_codepoint << 6) | (uc & 0x3F);
                utf8_expected_continuations--;
                if (utf8_expected_continuations > 0) {
                    return; // en cours
                }
                char mapped;
                if (unicode_to_cp437(utf8_codepoint, &mapped)) {
                    c = mapped;
                } else {
                    c = '?';
                }
            } else {
                utf8_expected_continuations = 0; // séquence invalide
            }
        } else if (uc >= 0x80) {
            if ((uc & 0xE0) == 0xC0) { utf8_expected_continuations = 1; utf8_codepoint = (uc & 0x1F); return; }
            if ((uc & 0xF0) == 0xE0) { utf8_expected_continuations = 2; utf8_codepoint = (uc & 0x0F); return; }
            if ((uc & 0xF8) == 0xF0) { utf8_expected_continuations = 3; utf8_codepoint = (uc & 0x07); return; }
            return; // octet >127 non conforme, ignorer
        }
#endif
        if (c != '\x1b') {
        if (x == -1 && y == -1) {
            if (c == '\n') {
                vga_x = 0; vga_y++;
            } else if (c == '\b') {
                if (vga_x > 0) vga_x--;
                print_char_vga(' ', vga_x, vga_y, current_color);
            } else {
                print_char_vga(c, vga_x, vga_y, current_color);
                vga_x++;
            }
            if (vga_x >= 80) { vga_x = 0; vga_y++; }
            if (vga_y >= 25) { scroll_screen(); vga_y = 24; }
            vga_console_set_cursor(vga_x, vga_y);
        } else {
            print_char_vga(c, x, y, color);
        }
        return;
        }
    }

    // Gestion de la machine à états ANSI
    switch (ansi_state) {
        case NORMAL:
            if (c == '\x1b') {
                ansi_state = ESCAPE;
            }
            break;

        case ESCAPE:
            if (c == '[') {
                ansi_state = BRACKET;
                ansi_pos = 0;
                for(int i=0; i<16; ++i) ansi_buffer[i] = 0;
            } else {
                ansi_state = NORMAL;
            }
            break;

        case BRACKET:
            if ((c >= '0' && c <= '9') || c == ';') {
                if (ansi_pos < 15) ansi_buffer[ansi_pos++] = c;
                ansi_state = PARAM;
            } else if (c == 'H') { // Cursor to Home (0,0)
                vga_x = 0;
                vga_y = 0;
                vga_console_set_cursor(vga_x, vga_y);
                ansi_state = NORMAL;
            } else if (c == 'J') { // Erase screen
                clear_screen_vga();
                ansi_state = NORMAL;
            } else if (c == 'm') { // Reset color
                current_color = 0x0F;
                ansi_state = NORMAL;
            } else {
                ansi_state = NORMAL;
            }
            break;

        case PARAM:
            if ((c >= '0' && c <= '9') || c == ';') {
                if (ansi_pos < 15) ansi_buffer[ansi_pos++] = c;
            } else {
                ansi_buffer[ansi_pos] = '\0';
                if (c == 'm') {
                    // For now, we only parse the first parameter for simplicity
                    int code = ansi_parse_param();
                    switch (code) {
                        case 0: current_color = 0x0F; break; // Reset
                        case 1: /* Ignore Bright */ break;
                        case 30: current_color = 0x00; break; // Black
                        case 31: current_color = 0x04; break; // Red
                        case 32: current_color = 0x02; break; // Green
                        case 33: current_color = 0x06; break; // Yellow
                        case 34: current_color = 0x01; break; // Blue
                        case 35: current_color = 0x05; break; // Magenta
                        case 36: current_color = 0x03; break; // Cyan
                        case 37: current_color = 0x07; break; // White
                    }
                } else if (c == 'J') {
                    int code = ansi_parse_param();
                    if (code == 2) clear_screen_vga();
                } else if (c == 'H') {
                    // For now, ignore params and just go to 0,0
                    vga_x = 0;
                    vga_y = 0;
                }
                ansi_state = NORMAL;
            }
            break;
    }
}

// Fonction pour afficher une chaîne de caractères sur VGA
void print_string_vga(const char* str, char color) {
    for (int i = 0; str[i] != '\0'; i++) {
        print_char(str[i], -1, -1, color);
    }
}

// Fonction pour afficher une chaîne de caractères sur le port série
void print_string_serial(const char* str) {
    for (int i = 0; str[i] != '\0'; i++) {
        write_serial(str[i]);
    }
}

// Fonction pour afficher un uint32_t en hexadecimal sur le port série
void print_hex_serial(uint32_t n) {
    char* hex = "0123456789abcdef";
    write_serial('0');
    write_serial('x');
    for (int i = 28; i >= 0; i -= 4) {
        write_serial(hex[(n >> i) & 0xF]);
    }
}

// Fonction pour afficher sur les deux sorties
void print_string(const char* str) {
    print_string_vga(str, 0x1F);
    print_string_serial(str);
}

// Fonction de comparaison de chaînes simple
int strcmp_simple(const char* s1, const char* s2) {
    int i = 0;
    while (s1[i] != '\0' && s2[i] != '\0') {
        if (s1[i] != s2[i]) {
            return s1[i] - s2[i];
        }
        i++;
    }
    return s1[i] - s2[i];
}

// Tâches de test pour démontrer le multitâche
void task_A_function() {
    int counter = 0;
    while(1) {
        print_char_vga('A', 78, 24, 0x1C); // Affiche 'A' en rouge dans le coin

        // Petit délai pour ralentir l'affichage
        for (volatile int i = 0; i < 1000000; i++);

        counter++;
        if (counter > 50) {
            print_string_serial("Tache A se termine\n");
            task_exit();
        }
    }
}

void task_B_function() {
    int counter = 0;
    while(1) {
        print_char_vga('B', 79, 24, 0x1A); // Affiche 'B' en vert juste à côté

        // Petit délai pour ralentir l'affichage
        for (volatile int i = 0; i < 1500000; i++);

        counter++;
        if (counter > 30) {
            print_string_serial("Tache B se termine\n");
            task_exit();
        }
    }
}

void task_C_function() {
    int counter = 0;
    while(1) {
        print_char_vga('C', 77, 24, 0x1E); // Affiche 'C' en jaune

        // Petit délai différent
        for (volatile int i = 0; i < 2000000; i++);

        counter++;
        if (counter > 20) {
            print_string_serial("Tache C se termine\n");
            task_exit();
        }
    }
}

// Fonction pour chercher une sous-chaîne
int strstr_simple(const char* haystack, const char* needle) {
    int i, j;
    for (i = 0; haystack[i] != '\0'; i++) {
        for (j = 0; needle[j] != '\0' && haystack[i + j] == needle[j]; j++);
        if (needle[j] == '\0') return 1;
    }
    return 0;
}

// Fonction pour effacer l'écran
void clear_screen() {
    vga_console_clear(0x07);
    vga_x = 0;
    vga_y = 0;
    vga_console_set_cursor(vga_x, vga_y);
}

/* Active le coprocesseur et SSE2 avant toute operation flottante du moteur GPT-2. */
static void cpu_enable_sse(void) {
    asm volatile(
        "mov %%cr0, %%eax\n\t"
        "andl $0xfffffffb, %%eax\n\t" /* clear CR0.EM */
        "orl $0x00000002, %%eax\n\t"  /* set CR0.MP */
        "mov %%eax, %%cr0\n\t"
        "mov %%cr4, %%eax\n\t"
        "orl $0x00000600, %%eax\n\t"  /* CR4.OSFXSR + CR4.OSXMMEXCPT */
        "mov %%eax, %%cr4\n\t"
        "fninit\n\t"
        : : : "eax", "memory");
}

// La fonction principale de notre noyau - MISE À JOUR pour le multitâche
void kmain(uint32_t multiboot_magic, uint32_t multiboot_addr) {
    char color = 0x1F;

    cpu_enable_sse();
    vga_console_init(color);

    // Initialisation du port série
    serial_init();

    // Initialisation de la GDT et du TSS
    gdt_init();

    // Effacer l'écran VGA
    for (int y = 0; y < 25; y++) {
        for (int x = 0; x < 80; x++) {
            print_char_vga(' ', x, y, color);
        }
    }

    // Afficher notre message de bienvenue
    vga_x = 2;
    vga_y = 2;
    vga_console_set_cursor(vga_x, vga_y);
    print_string("=== Bienvenue dans MOHHDY v4.0 ===\n");
    print_string("Systeme complet avec espace utilisateur\n\n");

    // Vérification du magic number Multiboot
    if (multiboot_magic != MULTIBOOT_MAGIC) {
        print_string("ERREUR: Magic Multiboot invalide!\n");
        print_string("Le systeme ne peut pas continuer.\n");
        while(1) { asm volatile("hlt"); }
    }

    print_string("Multiboot detecte correctement.\n");

    // Récupération des informations Multiboot
    multiboot_info_t* mbi = (multiboot_info_t*)multiboot_addr;

    // Initialisation des interruptions - ORDRE CRITIQUE POUR QEMU
    print_string("=== Initialisation systeme interruptions ===\n");
    print_string("Etape 1: IDT...\n");
    idt_init();         // Initialise la table des interruptions
    
    print_string("Etape 2: PIC et handlers...\n");
    interrupts_init();  // Initialise le PIC et active les interruptions
    
    print_string("Etape 3: Clavier PS/2 (interruptions temporairement desactivees)...\n");
    // Initialise le clavier avec les interruptions désactivées pour éviter les race conditions
    asm volatile("cli");
    keyboard_init();
    asm volatile("sti");
    
    print_string("=== Systeme interruptions PRET ===\n");
    ne2k_boot_probe();
    print_string("IRQ0 (timer): OK\n");
    print_string("IRQ1 (keyboard): OK\n");
    print_string("QEMU devrait maintenant generer les interruptions clavier.\n");

    // Initialiser la gestion de la mémoire
    print_string("Initialisation de la gestion memoire...\n");
    uint32_t memory_size = multiboot_get_memory_size(mbi);
    pmm_init(memory_size, multiboot_addr);
    print_string("Physical Memory Manager initialise.\n");

    vmm_init(); // Active le paging
    print_string("Virtual Memory Manager initialise.\n");

    // Initialiser l'initrd si disponible
    uint32_t module_count = multiboot_get_module_count(mbi);
    if (module_count > 0) {
        multiboot_module_t* initrd_module = multiboot_get_module(mbi, 0);
        if (initrd_module) {
            uint32_t initrd_location = initrd_module->mod_start;
            uint32_t initrd_size = initrd_module->mod_end - initrd_module->mod_start;

            print_string("Initrd trouve ! Initialisation...\n");
            initrd_init(initrd_location, initrd_size);
            if (initrd_file_exists("models/gpt2.gguf")) {
                gpt2_gguf_info_t gguf_info;
                int gguf_status = gpt2_gguf_probe_blob((const uint8_t*)initrd_read_file("models/gpt2.gguf"),
                                                        initrd_get_file_size("models/gpt2.gguf"), &gguf_info);
                print_string(gpt2_gguf_probe_status(gguf_status));
                if (gguf_status == 0 && gguf_info.unsupported_quantized_tensors != 0U) {
                    print_string("; kernels de quantification a activer\n");
                } else {
                    print_string("\n");
                }
            }
            if (gpt2_tokenizer_load_from_initrd("models/gpt2_tokenizer.bin") == 0) {
                print_string("Tokenizer GPT-2 local charge depuis l'initrd.\n");
            } else {
                print_string(gpt2_tokenizer_status());
                print_string("\n");
            }
            if (gpt2_model_load_from_initrd("models/gpt2_124M.bin") == 0) {
                const gpt2_model_t* gpt2 = gpt2_model_current();
                print_string("Modele GPT-2 local charge depuis l'initrd.\n");
                /* Les mini-checkpoints de validation executent un jeton CPU au boot. */
                if (gpt2->config.vocab_size <= 8 && gpt2->config.channels <= 16) {
                    uint32_t seed_token = 0;
                    uint32_t generated_token = 0;
                    if (gpt2_generate_next(&seed_token, 1, &generated_token) == 0) {
                        print_string(gpt2_infer_status());
                        print_string("\n");
                    } else {
                        print_string(gpt2_infer_status());
                        print_string("\n");
                    }
                }
            } else {
                print_string(gpt2_model_status());
                print_string("\n");
            }
        }
    }

    overlay_init();
    if (ata_init() == 0) {
        if (overlay_load_disk() == 0) {
            print_string("Overlay FS charge depuis le disque IDE.\n");
        } else {
            print_string("Overlay FS initialise (disque IDE vide).\n");
        }
        if (ata_present_drive(ATA_DRIVE_SLAVE) &&
            fat32_mount(fat32_root(), fat32_ata_slave_read_sector, 0U) == 0) {
            if (fat32_attach_writer(fat32_root(), fat32_ata_slave_write_sector) != 0) {
                print_string("FAT32: writer ATA esclave indisponible; creation desactivee.\n");
            }
            print_string("FAT32 secondaire monte.\n");
        }
        if (fat16_mount(fat16_root(), fat16_ata_read_sector, 64U) == 0) {
            if (fat16_attach_read_window(fat16_root(), fat16_ata_read_sectors,
                                          fat16_ata_read_window,
                                          sizeof(fat16_ata_read_window)) != 0) {
                print_string("FAT16: cache multi-secteurs indisponible; repli secteur.\n");
            }
            if (fat16_attach_writer(fat16_root(), fat16_ata_write_sector) != 0) {
                print_string("FAT16: writer ATA indisponible; creation desactivee.\n");
            }
            print_string(fat16_status());
            print_string("\n");
            if (gpt2_gguf_infer_init_fat16(fat16_root(), "GPT2.GGU") == 0) {
                print_string(gpt2_gguf_infer_status());
                print_string("\n");
            } else {
                print_string("GGUF FAT16 optionnel indisponible; profil .gguf desactive.\n");
            }
        } else {
            print_string(fat16_status());
            print_string("\n");
        }
    } else {
        print_string("Overlay FS initialise (mkdir/rm en RAM).\n");
    }

    // NOUVEAU: Initialisation du système de tâches
    print_string("Initialisation du systeme de taches...\n");
    tasking_init();
    service_registry_init();

    // NOUVEAU: Initialisation des appels système
    print_string("Initialisation des appels systeme...\n");
    syscall_init();

    // Crée des tâches de test kernel (DÉSACTIVÉ pour stabilité)
    print_string("Creation des taches kernel de demonstration... DESACTIVE\n");
    print_string("Mode mono-tache pour stabilite maximale.\n");

/*
 * SUPPRIMEZ OU COMMENTEZ CES LIGNES
 *
 * task_t* task_a = create_task(task_A_function);
 * if (task_a) print_string("Tache A creee\n");
 *
 * task_t* task_b = create_task(task_B_function);
 * if (task_b) print_string("Tache B creee\n");
 *
 * task_t* task_c = create_task(task_C_function);
 * if (task_c) print_string("Tache C creee\n");
*/


    // PHASE 2: Réactiver le timer pour les interruptions clavier
    print_string("PHASE 2: Timer reactive pour interruptions clavier...\n");
    // timer_init(100); // Réactiver le timer à 100Hz pour les interruptions
    print_string("Timer reactive - Interruptions clavier fonctionnelles.\n");

    // NOUVEAU: Lancement du shell interactif avec IA
    print_string("Lancement du shell interactif MOHHDY...\n");

    if (module_count > 0) {
        // Chercher le shell dans l'initrd
        uint8_t* shell_program = (uint8_t*)initrd_read_file("bin/shell");
        if (shell_program) {
            print_string("Shell trouve ! Chargement...\n");

            // Le code de simulation est retiré, on va lancer le vrai shell.
            print_string("Shell trouve. Preparation du lancement...\n");
        } else {
            print_string("ERREUR: Fichier 'shell' non trouve dans l'initrd!\n");
        }
    } else {
        print_string("ERREUR: Aucun module initrd trouve!\n");
    }

    // --- Lancement du Shell Utilisateur ---
    print_string("\nLancement du Shell Utilisateur...\n");
    
    task_t* shell_task = create_task_from_initrd_file("bin/shell");
    if (!shell_task) {
        print_string("ERREUR: Impossible de creer la tache shell. Arret du systeme.\n");
        while(1) asm volatile("hlt");
    }
    
    print_string("Tache shell prete. Demarrage du timer...\n");
    timer_init(100);

    print_string("\n=== MOHHDY v6.0 - Force le premier changement de contexte ===\n");
    print_string("Declencher immediatement le planificateur...\n");
    
    // Forcer le premier changement de contexte vers le shell utilisateur
    extern volatile int g_reschedule_needed;
    g_reschedule_needed = 1;
    
    // Activer les interruptions pour que le timer puisse déclencher le scheduler
    asm volatile("sti");

    // Boucle d'inactivité du kernel. Le scheduler fera le travail.
    while(1) {
        asm volatile("hlt");
    }
}

