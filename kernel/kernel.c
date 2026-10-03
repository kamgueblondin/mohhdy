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
#include "input/usb_tablet.h"
#include "service_registry.h"
#include "ata_job.h"
#include "vga_console.h"
#include "ne2k.h"
#include "net_socket.h"
#include "net_wire.h"
#include "net_nic_owner.h"
#include "net_llm_client.h"
#include "tls_test_leaf.h"
#include "net_tls_server.h"
#include "ecdsa_p256.h"
#include <stddef.h>

// Function to read a byte from a port
unsigned char inb(unsigned short port);
// Function to write a byte to a port
void outb(unsigned short port, unsigned char data);
// Function to print string to serial port (forward declaration)
void print_string_serial(const char* str);
void print_string(const char* str);


static ne2k_device_t boot_ne2k_device;
static ne2k_io_t boot_ne2k_io;
static uint8_t boot_ne2k_present;
/* Tranche 5 suite: while the Ring 3 worker owns the NE2000, IRQ3 is only
 * counted for it (the worker reads and acks the ISR with its own PIO); the
 * PIC EOI stays in the stub. */
void ne2k_irq_handler(void) {
    if (nic_owner_irq()) return;
    ne2k_irq_service();
}

/* Tranche 5 suite: every kernel NE2000 access goes through this gate. While
 * a Ring 3 worker owns the card the kernel entry points refuse first
 * (OS_NET_NIC_WORKER_OWNED); this is the safety net: a stray access is
 * dropped and counted (kernel_refused, expected 0 in the contracts). */
static ne2k_io_t boot_ne2k_raw_io;
static uint8_t kernel_nic_inb(void* context, uint16_t port) {
    (void)context;
    if (!nic_owner_kernel_may_touch()) { nic_owner_note_kernel_refused(); return 0xFFU; }
    return boot_ne2k_raw_io.inb(boot_ne2k_raw_io.context, port);
}
static void kernel_nic_outb(void* context, uint16_t port, uint8_t value) {
    (void)context;
    if (!nic_owner_kernel_may_touch()) { nic_owner_note_kernel_refused(); return; }
    boot_ne2k_raw_io.outb(boot_ne2k_raw_io.context, port, value);
}

/* Tranche 5 pile: UTC for the Ring 3 TLS client (SYS_NET_NIC UTC). */
int kernel_net_utc(char* out, uint16_t capacity) {
    rtc_io_t io;
    return net_llm_client_utc(&io, out, capacity) == 0 ? 0 : OS_NET_NIC_ABSENT;
}

static void ne2k_boot_probe(void) {
    net_llm_client_bind(&boot_ne2k_device, &boot_ne2k_io, 0);
    net_llm_client_reset();
    boot_ne2k_present = 0U;
    if (ne2k_i386_io(&boot_ne2k_raw_io) != 0) return;
    boot_ne2k_io.context = 0;
    boot_ne2k_io.inb = kernel_nic_inb;
    boot_ne2k_io.outb = kernel_nic_outb;
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
    net_llm_client_bind(&boot_ne2k_device, &boot_ne2k_io, 1);
    print_string("NE2000 ISA detecte, MAC valide et anneaux RX/TX configures.\\n");
    print_string(boot_llm_test_trust_anchor_ready ?
        "Ancre TLS de test locale prete (example.com).\n" :
        "Ancre TLS de test locale indisponible.\n");
}

uint32_t kernel_net_status(void) {
    return boot_ne2k_present ? 3U : 0U;
}

/* Tranche 5 slice 3: worker-only wire path. The syscall layer only calls
 * these for the live net-driver PID; they drive the boot NE2000 (Ring 0)
 * with dedicated frame buffers so the LLM/peer paths keep theirs. */
static uint8_t boot_wire_tx[KERNEL_LLM_FRAME_CAPACITY];
static uint8_t boot_wire_rx[KERNEL_LLM_FRAME_CAPACITY];

/* Tranche 5 suite: frames of a wire op running on the worker's NIC wait
 * here until the next SYS_NET_NIC pump copies them out. */
static uint8_t boot_wire_q[OS_NET_NIC_PUMP_TX_MAX][OS_NET_NIC_FRAME_MAX];
static uint16_t boot_wire_q_len[OS_NET_NIC_PUMP_TX_MAX];
static uint16_t boot_wire_q_count;
static struct { uint8_t pending; uint8_t done; int32_t result; } boot_wire_port;

static int kernel_wire_emit(void* context, const uint8_t* frame, uint16_t length) {
    uint16_t i;
    (void)context;
    if (boot_wire_q_count >= OS_NET_NIC_PUMP_TX_MAX || length > OS_NET_NIC_FRAME_MAX) return -1;
    for (i = 0U; i < length; i++) boot_wire_q[boot_wire_q_count][i] = frame[i];
    boot_wire_q_len[boot_wire_q_count] = length;
    boot_wire_q_count++;
    return 0;
}

int kernel_net_nic_port_mode(void) { return nic_owner_pid() != 0; }

static int kernel_net_wire_ctx(net_wire_ctx_t* ctx) {
    if (!boot_ne2k_present) return OS_NET_WIRE_UNAVAILABLE;
    ctx->device = &boot_ne2k_device;
    ctx->io = &boot_ne2k_io;
    ctx->cache = &boot_llm_arp_cache;
    ctx->tx = boot_wire_tx;
    ctx->rx = boot_wire_rx;
    ctx->capacity = (uint16_t)sizeof(boot_wire_tx);
    ctx->emit = 0;
    ctx->emit_context = 0;
    if (kernel_net_nic_port_mode()) {
        if (boot_wire_port.pending) return OS_NET_WIRE_UNAVAILABLE; /* one op at a time */
        ctx->emit = kernel_wire_emit;
        boot_wire_q_count = 0U;
    }
    return 0;
}

/* Port mode: the op keeps running on the worker's NIC. Frames already queued
 * (even by an op that finished at once, e.g. a CLOSE_WAIT FIN) are handed out
 * by the next pump, together with the result. */
static int kernel_net_wire_port_begin(const net_wire_ctx_t* ctx, int running) {
    if (!ctx->emit) return running; /* unused */
    if (!running && boot_wire_q_count == 0U) return net_wire_op_result();
    boot_wire_port.pending = 1U;
    boot_wire_port.done = running ? 0U : 1U;
    boot_wire_port.result = running ? 0 : net_wire_op_result();
    return OS_NET_WIRE_PENDING;
}

int kernel_net_nic_pump(os_net_nic_pump_t* pump) {
    net_wire_ctx_t ctx;
    uint16_t i, j, frame_in = 0U;
    if (!pump || !boot_wire_port.pending) return OS_NET_WIRE_UNAVAILABLE;
    if (pump->mode != OS_NET_NIC_PUMP_FETCH && !boot_wire_port.done) {
        ctx.device = &boot_ne2k_device; ctx.io = &boot_ne2k_io; ctx.cache = &boot_llm_arp_cache;
        ctx.tx = boot_wire_tx; ctx.rx = boot_wire_rx; ctx.capacity = (uint16_t)sizeof(boot_wire_tx);
        ctx.emit = kernel_wire_emit; ctx.emit_context = 0;
        if (pump->mode == OS_NET_NIC_PUMP_FRAME) {
            if (!pump->rx || pump->rx_length == 0U || pump->rx_length > sizeof(boot_wire_rx))
                return OS_SOCKET_BAD_ARGUMENT;
            for (i = 0U; i < pump->rx_length; i++) boot_wire_rx[i] = pump->rx[i];
            frame_in = 1U;
        }
        if (!net_wire_op_step(&ctx, frame_in, frame_in ? pump->rx_length : 0U)) {
            boot_wire_port.done = 1U;
            boot_wire_port.result = net_wire_op_result();
        }
    }
    pump->tx_count = boot_wire_q_count;
    for (i = 0U; i < boot_wire_q_count; i++) {
        pump->tx_length[i] = boot_wire_q_len[i];
        for (j = 0U; j < boot_wire_q_len[i]; j++)
            pump->tx[(uint32_t)i * OS_NET_NIC_FRAME_MAX + j] = boot_wire_q[i][j];
    }
    nic_owner_note_pump(boot_wire_q_count, frame_in, pump->tx_sent, pump->tx_failed);
    boot_wire_q_count = 0U;
    pump->done = boot_wire_port.done;
    pump->result = boot_wire_port.result;
    if (boot_wire_port.done) boot_wire_port.pending = 0U;
    return 0;
}

int kernel_net_nic_info(os_net_nic_info_t* info) {
    uint8_t i;
    if (!boot_ne2k_present || !info) return OS_NET_NIC_ABSENT;
    info->base_port = (uint16_t)OS_NET_NIC_BASE_PORT;
    info->irq = (uint8_t)OS_NET_NIC_IRQ_LINE;
    for (i = 0U; i < 6U; i++) info->mac[i] = boot_ne2k_device.mac[i];
    info->reserved = 0U;
    return 0;
}

int kernel_net_nic_present(void) { return boot_ne2k_present ? 1 : 0; }

/* LBA 4224 holds the service-event journal. Skip the write when that sector
 * sits inside the mounted FAT16 volume (the large GGUF disk). The foundation
 * fixture keeps it past the volume, next to the mount journal. */
int service_registry_event_journal_allowed(void) {
    fat16_volume_t* volume = fat16_root();
    uint32_t end;
    if (!volume || !volume->mounted) return 1;
    end = volume->base_lba + volume->total_sectors;
    if (end < volume->base_lba) return 0;
    return 4224U < end ? 0 : 1;
}

/* Worker lost: cancel its op, then re-initialise the card from Ring 0 (the
 * worker left rings, IMR and maybe a DMA in an unknown state). */
int kernel_net_nic_reclaim(void) {
    boot_wire_port.pending = 0U;
    boot_wire_q_count = 0U;
    if (net_wire_op_active()) net_wire_op_cancel();
    if (!boot_ne2k_present) return 0;
    nic_owner_note_reclaim();
    if (ne2k_probe(&boot_ne2k_device, 0x300U, &boot_ne2k_io) != 0 ||
        ne2k_prepare(&boot_ne2k_device, &boot_ne2k_io) != 0 ||
        ne2k_read_mac(&boot_ne2k_device, &boot_ne2k_io) != 0 ||
        ne2k_configure_rings(&boot_ne2k_device, &boot_ne2k_io) != 0) {
        print_string_serial("[NET] NE2000 reclaim failed\n");
        return -1;
    }
    boot_ne2k_io.outb(boot_ne2k_io.context, (uint16_t)(0x300U + 0x0FU), 0x00U); /* IMR off */
    (void)ne2k_irq_attach(&boot_ne2k_device, &boot_ne2k_io);
    print_string_serial("[NET] NE2000 back in Ring 0 after worker loss\n");
    return 0;
}

int kernel_net_wire_connect(const os_net_wire_connect_t* request) {
    net_wire_ctx_t ctx;
    int status = kernel_net_wire_ctx(&ctx);
    if (status != 0) return status;
    if (ctx.emit) return kernel_net_wire_port_begin(&ctx, net_wire_op_connect(&ctx, request));
    return net_wire_connect(&ctx, request);
}
int kernel_net_wire_send(int socket_id, const uint8_t* data, uint16_t length, uint8_t* segment,
                         uint16_t capacity, uint16_t* out_length, uint16_t attempts) {
    net_wire_ctx_t ctx;
    int status;
    if (!net_wire_is_bound(socket_id)) return OS_NET_WIRE_NOT_BOUND;
    status = kernel_net_wire_ctx(&ctx);
    if (status != 0) return status;
    if (ctx.emit)
        return kernel_net_wire_port_begin(&ctx, net_wire_op_send(&ctx, socket_id, data, length, segment,
                                                                 capacity, out_length, attempts));
    return net_wire_send(&ctx, socket_id, data, length, segment, capacity, out_length, attempts);
}
int kernel_net_wire_recv(int socket_id, uint8_t* buffer, uint16_t capacity, uint16_t* out_length,
                         uint16_t attempts) {
    net_wire_ctx_t ctx;
    int status;
    if (!net_wire_is_bound(socket_id)) return OS_NET_WIRE_NOT_BOUND;
    status = kernel_net_wire_ctx(&ctx);
    if (status != 0) return status;
    if (ctx.emit)
        return kernel_net_wire_port_begin(&ctx, net_wire_op_recv(&ctx, socket_id, buffer, capacity,
                                                                 out_length, attempts));
    return net_wire_recv(&ctx, socket_id, buffer, capacity, out_length, attempts);
}
int kernel_net_wire_close(int socket_id) {
    net_wire_ctx_t ctx;
    if (!net_wire_is_bound(socket_id)) return OS_NET_WIRE_NOT_BOUND;
    if (kernel_net_wire_ctx(&ctx) != 0) return net_wire_close(0, socket_id, 0U);
    if (ctx.emit) return kernel_net_wire_port_begin(&ctx, net_wire_op_close(&ctx, socket_id, 0U));
    return net_wire_close(&ctx, socket_id, 0U);
}





#define FAT16_ATA_READ_WINDOW_SECTORS 16U
static uint8_t fat16_ata_read_window[FAT16_ATA_READ_WINDOW_SECTORS * 512U];

/* Tranche 4 slice 3: FAT16/FAT32 sector I/O goes through the Ring 3
 * atadriver when it is live (synchronous sector RPC from the calling
 * syscall, see syscall_ata_fat_io); Ring 0 PIO only at boot (mount, before
 * any task) or as the fallback when the driver is absent or dead. */
static int fat_disk_io(uint8_t drive, uint32_t lba, uint32_t count, void* buffer, int write) {
    int rc;
    if (syscall_ata_fat_io(drive, lba, count, buffer, write, &rc)) return rc;
    rc = write ? ata_write_sectors_drive(drive, lba, count, buffer)
               : ata_read_sectors_drive(drive, lba, count, buffer);
    if (rc == 0) syscall_ata_note_fat_kernel_pio(count);
    return rc;
}

static int fat16_ata_read_sector(uint32_t lba, void* buffer) {
    return fat_disk_io(ATA_DRIVE_MASTER, lba, 1U, buffer, 0);
}

static int fat16_ata_read_sectors(uint32_t lba, uint32_t count, void* buffer) {
    return fat_disk_io(ATA_DRIVE_MASTER, lba, count, buffer, 0);
}

static int fat16_ata_write_sector(uint32_t lba, const void* buffer) {
    return fat_disk_io(ATA_DRIVE_MASTER, lba, 1U, (void*)buffer, 1);
}

static int fat32_ata_slave_read_sector(uint32_t lba, void* buffer) {
    return fat_disk_io(ATA_DRIVE_SLAVE, lba, 1U, buffer, 0);
}

static int fat32_ata_slave_write_sector(uint32_t lba, const void* buffer) {
    return fat_disk_io(ATA_DRIVE_SLAVE, lba, 1U, (void*)buffer, 1);
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
        if (vga_desktop_active() && x == -1 && y == -1) {
            return;
        }
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
    usb_tablet_init();
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
    nic_owner_init();
    syscall_ata_bridge_init();
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

    /* Tranche 4 slice 2: client write fences for the Ring 3 atadriver: the
     * FAT16 volume on the master and a FAT32 slave stay kernel-only. */
    ata_job_set_fences(fat16_root()->mounted ?
                           fat16_root()->base_lba + fat16_root()->total_sectors :
                           OS_ATA_KERNEL_RESERVED_LBAS,
                       (uint32_t)fat32_is_mounted(fat32_root()));

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
    
    task_set_root_shell(shell_task->id);

    /* Tranche 4 slice 3: the Ring 3 atadriver is spawned at boot when an IDE
     * disk is present, so FAT and overlay disk I/O take the driver path by
     * default; Ring 0 PIO stays the fallback when it is absent or dead. It is
     * created after the shell (the shell keeps PID 1) but runs first so it
     * registers before the shell reaches its input loop. It has no user
     * parent; only the root shell may stop it (kill). The overlay snapshot
     * was already loaded above by Ring 0 PIO (no task could run yet); the
     * boot driver does not reload it (see sys_service_register). */
    if (ata_present()) {
        task_t* ata_driver_task = create_task_from_initrd_file("bin/atadriver");
        if (ata_driver_task) {
            ata_driver_task->boot_service = 1U;
            task_queue_move_first_user(ata_driver_task);
            syscall_ata_set_boot_driver(ata_driver_task->id);
            print_string_serial("[ATA] boot atadriver spawned\n");
        } else {
            print_string_serial("[ATA] boot atadriver unavailable; Ring 0 PIO path\n");
        }
    }

    /* Tranche 5: when the boot probe saw a NE2000, start networker before the
     * shell prompt. Presence detection and the reclaim path stay in Ring 0.
     * The worker takes the ports; killing it (root shell) restores Ring 0. */
    if (!syscall_net_ring0_fallback_enabled())
        print_string_serial("[NET] build NET_RING0_FALLBACK=0: kernel socket/LLM/peer/wire syscalls "
                            "disabled, networker is the only network path\n");
    if (!syscall_gguf_ring0_fallback_enabled())
        print_string_serial("[AI] build GGUF_RING0_FALLBACK=0: syscalls 109/110 refused without a "
                            "GGUF worker\n");
    /* Strict build (default): networker is the only network path, so it
     * is started even without a card (loopback-only sockets in Ring 3). */
    if (kernel_net_nic_present() || !syscall_net_ring0_fallback_enabled()) {
        task_t* net_driver_task = create_task_from_initrd_file("bin/networker");
        if (net_driver_task) {
            net_driver_task->boot_service = 1U;
            task_queue_move_first_user(net_driver_task);
            print_string_serial("[NET] boot networker spawned\n");
        } else {
            print_string_serial("[NET] boot networker unavailable; Ring 0 NIC path\n");
        }
    }

    /* Inventory item 4: with a valid FP32 checkpoint and tokenizer in the
     * initrd, the GPT-2 engine runs in the Ring 3 aiworker ("ai-engine"),
     * which maps the same initrd frames read-only. The Ring 0 engine stays
     * as the fallback when the worker is absent, dead or stalled. With the
     * FAT16 GGUF profile ready, the worker also bulk-reads GPT2.GGU from the
     * disk and serves the 109/110 sampling steps. The default build refuses
     * 109/110 when that worker is absent; GGUF_READY releases the kernel
     * snapshot. The FP32 Ring 0 fallback stays. */
    if (gpt2_tokenizer_ready() &&
        (gpt2_model_current()->ready || gpt2_gguf_infer_resident_size() != 0U)) {
        task_t* ai_worker_task = create_task_from_initrd_file("bin/aiworker");
        if (ai_worker_task) {
            ai_worker_task->boot_service = 1U;
            task_queue_move_first_user(ai_worker_task);
            print_string_serial("[AI] boot aiworker spawned\n");
        } else {
            print_string_serial("[AI] boot aiworker unavailable; Ring 0 GPT-2 path\n");
        }
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

