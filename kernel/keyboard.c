#include "keyboard.h"
#include "kernel.h"
#include "vga_console.h"
#include <stdint.h>

// Fonctions externes pour les ports I/O et autres
extern unsigned char inb(unsigned short port);
extern void outb(unsigned short port, unsigned char value);
extern void print_char_vga(char c, int x, int y, char color);
extern void write_serial(char c);
extern int vga_x, vga_y;

// Buffer clavier hybride (interruption + polling)
#define KBD_BUF_SIZE 256
static volatile unsigned char kbd_buf[KBD_BUF_SIZE];
static volatile unsigned int kbd_head = 0;
static volatile unsigned int kbd_tail = 0;

// Système de fallback hybride
static volatile int interrupt_mode_active = 1;
static volatile int polling_fallback_active = 0;
static volatile int debug_interrupt_count = 0;
static volatile int debug_polling_count = 0;
static volatile uint32_t last_poll_time = 0;

// Variables de diagnostic
static volatile int initialization_phase = 0;

// Modifieurs
static volatile int g_shift_pressed = 0;
static volatile int g_caps_lock = 0;
static volatile int g_e0_prefix = 0;

// Délai optimisé pour QEMU
void qemu_delay() {
    for (volatile int i = 0; i < 1000; i++);
}

// Délai plus long pour les opérations critiques
void qemu_long_delay() {
    for (volatile int i = 0; i < 50000; i++);
}

// Attendre que le contrôleur soit prêt (version optimisée QEMU)
int wait_kbd_ready(int is_command) {
    int timeout = 10000; // Timeout plus court pour QEMU
    uint8_t status;
    
    while (timeout-- > 0) {
        status = inb(0x64);
        if (is_command) {
            if (!(status & 0x02)) return 1; // Prêt pour commande
        } else {
            if (status & 0x01) return 1;    // Données disponibles
        }
        qemu_delay();
    }
    
    return 0; // Timeout
}

// Buffer management amélioré avec filtrage anti-fantômes
void kbd_put_char(char c) {
    // Filtrer les caractères nuls pour éviter les touches fantômes
    if (c == 0) {
        return; // Ne pas ajouter les caractères nuls au buffer
    }
    
    unsigned int next = (kbd_head + 1) & (KBD_BUF_SIZE - 1);
    if (next != kbd_tail) {
        kbd_buf[kbd_head] = c;
        kbd_head = next;
        
        // Debug limité
        if (debug_interrupt_count + debug_polling_count <= 5) {
            print_string_serial("KBD_PUT: '");
            write_serial(c);
            print_string_serial("'\n");
        }
    }
}

int kbd_get_char_nonblock(char *out) {
    if (kbd_head == kbd_tail) {
        return 0; // Vide
    }
    
    // Chercher le prochain caractère valide (non-nul) dans le buffer
    int max_tries = KBD_BUF_SIZE; // Éviter boucle infinie
    while (kbd_head != kbd_tail && max_tries-- > 0) {
        char c = kbd_buf[kbd_tail];
        kbd_tail = (kbd_tail + 1) & (KBD_BUF_SIZE - 1);
        
        // Retourner seulement les caractères valides (non-nuls)
        if (c != 0) {
            *out = c;
            return 1; // Succès
        }
        // Sinon continuer à chercher le prochain caractère valide
    }
    
    return 0; // Aucun caractère valide trouvé
}

// Tables US QWERTY base et SHIFT (Set 1)
static const char base_map[128] = {
    /*00-0F*/  0,   0,  '1','2','3','4','5','6','7','8','9','0','-','=', '\b','\t',
    /*10-1F*/ 'q','w','e','r','t','y','u','i','o','p','[',']','\n',  0, 'a','s',
    /*20-2F*/ 'd','f','g','h','j','k','l',';','\'', '`',  0, '\\','z','x','c','v',
    /*30-3F*/ 'b','n','m',',','.','/',  0,  '*',  0, ' ',  0,   0,   0,   0,   0,   0,
    /*40-4F*/  0,  0,  0,  0,  0,  0,  0,  '7','8','9','-','4','5','6','+','1',
    /*50-5F*/ '2','3','0','.', 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    /*60-6F*/  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    /*70-7F*/  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0
};

static const char shift_map[128] = {
    /*00-0F*/  0,   0,  '!','@','#','$','%','^','&','*','(',')','_','+', '\b','\t',
    /*10-1F*/ 'Q','W','E','R','T','Y','U','I','O','P','{','}','\n',  0, 'A','S',
    /*20-2F*/ 'D','F','G','H','J','K','L',':','"','~',  0,  '|', 'Z','X','C','V',
    /*30-3F*/ 'B','N','M','<','>','?',  0,  '*',  0, ' ',  0,   0,   0,   0,   0,   0,
    /*40-4F*/  0,  0,  0,  0,  0,  0,  0,  '7','8','9','-','4','5','6','+','1',
    /*50-5F*/ '2','3','0','.', 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    /*60-6F*/  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    /*70-7F*/  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0
};

static inline char map_scancode(uint8_t sc) {
    if (sc >= 128) return 0;
    char ch = g_shift_pressed ? shift_map[sc] : base_map[sc];
    // Caps Lock: inverser casse pour lettres si SHIFT non actif
    if (g_caps_lock) {
        if (!g_shift_pressed && ch >= 'a' && ch <= 'z') return (char)(ch - 'a' + 'A');
        if (!g_shift_pressed && ch >= 'A' && ch <= 'Z') return (char)(ch - 'A' + 'a');
    }
    return ch;
}

static int kbd_handle_extended(uint8_t scancode) {
    uint8_t key = (uint8_t)(scancode & 0x7F);

    if (scancode & 0x80) {
        return 1;
    }
    if (key == 0x49) {
        vga_console_view_up(VGA_ROWS - 1);
        return 1;
    }
    if (key == 0x51) {
        vga_console_view_down(VGA_ROWS - 1);
        return 1;
    }
    if (key == 0x48) {
        vga_console_view_up(1);
        return 1;
    }
    if (key == 0x50) {
        vga_console_view_down(1);
        return 1;
    }
    return 1;
}

static int kbd_take_extended_prefix(uint8_t scancode) {
    if (scancode == 0xE0) {
        g_e0_prefix = 1;
        return 1;
    }
    if (g_e0_prefix) {
        g_e0_prefix = 0;
        return kbd_handle_extended(scancode);
    }
    return 0;
}

// Polling de secours optimisé (controlled fallback) - plus agressif pour mode console
void keyboard_poll_check() {
    static uint32_t poll_counter = 0;
    poll_counter++;
    
    // Polling plus agressif en console si pas d'IRQ, mais avec moins de pauses
    int poll_freq = (debug_interrupt_count > 0) ? 20000 : 500; // fréquence d'essai
    
    if (poll_counter % poll_freq == 0) {
        uint8_t status = inb(0x64);
        if (status & 0x01) { // Données disponibles
            uint8_t scancode = inb(0x60);
            
            debug_polling_count++;
            
            // Filtrer les ACK et codes de contrôle
            if (scancode == 0xFA || scancode == 0xFE || scancode == 0xAA) {
                return;
            }
            if (kbd_take_extended_prefix(scancode)) {
                return;
            }
            
            // Debug polling réduit pour éviter le bruit
            if ((debug_interrupt_count == 0 && debug_polling_count <= 3)) {
                print_string_serial("POLL: scancode=0x");
                char hex[] = "0123456789ABCDEF";
                write_serial(hex[(scancode >> 4) & 0xF]);
                write_serial(hex[scancode & 0xF]);
                print_string_serial(" (mode console)\n");
            }
            
            // Gérer press/release pour maintenir correctement l'état SHIFT en mode polling
            if (scancode & 0x80) {
                // Key release
                uint8_t rel = scancode;
                if (rel == 0xAA || rel == 0xB6) { g_shift_pressed = 0; return; } // LSHIFT/RSHIFT release
                return; // ignorer autres releases
            } else {
                // Modifieurs
                if (scancode == 0x2A || scancode == 0x36) { g_shift_pressed = 1; return; }
                if (scancode == 0x3A) { g_caps_lock = !g_caps_lock; return; }
                char c = map_scancode(scancode);
                if (c != 0) {
                    kbd_put_char(c);
                    polling_fallback_active = 1;
                }
            }
        }
    }
}

// Handler d'interruption optimisé
void keyboard_interrupt_handler() {
    debug_interrupt_count++;
    
    // Debug interruptions
    if (debug_interrupt_count <= 3) {
        print_string_serial("KBD_IRQ: #");
        write_serial('0' + (debug_interrupt_count % 10));
        print_string_serial("\n");
    }
    
    uint8_t status = inb(0x64);
    if (!(status & 0x01)) return; // Pas de données
    
    uint8_t scancode = inb(0x60);
    
    // Debug scancode
    if (debug_interrupt_count <= 5) {
        print_string_serial("IRQ: scancode=0x");
        char hex[] = "0123456789ABCDEF";
        write_serial(hex[(scancode >> 4) & 0xF]);
        write_serial(hex[scancode & 0xF]);
        print_string_serial("\n");
    }
    
    // Filtrer les codes de contrôle
    if (scancode == 0xFA || scancode == 0xFE || scancode == 0xAA) {
        return;
    }
    if (kbd_take_extended_prefix(scancode)) {
        return;
    }
    
    // Modifieurs (press/release)
    if (scancode == 0x2A || scancode == 0x36) { g_shift_pressed = 1; return; }     // LSHIFT/RSHIFT press
    if (scancode == 0xAA || scancode == 0xB6) { g_shift_pressed = 0; return; }     // LSHIFT/RSHIFT release
    if (scancode == 0x3A) { g_caps_lock = !g_caps_lock; return; }                  // CAPSLOCK toggle
    
    // Traiter seulement les key press
    if (!(scancode & 0x80)) {
        char c = map_scancode(scancode);
        if (c != 0) {
            kbd_put_char(c);
        }
    }
}

// Initialisation clavier hybride optimisée pour QEMU
void keyboard_init() {
    print_string_serial("=== KEYBOARD HYBRID INIT (FIXED) ===\n");
    
    // Reset variables
    debug_interrupt_count = 0;
    debug_polling_count = 0;
    interrupt_mode_active = 1;
    polling_fallback_active = 0;
    kbd_head = 0;
    kbd_tail = 0;
    g_shift_pressed = 0;
    g_caps_lock = 0;
    g_e0_prefix = 0;
    
    initialization_phase = 1;
    
    // Phase 1: Nettoyage initial
    print_string_serial("Phase 1: Nettoyage initial...\n");
    
    int flush_count = 0;
    while ((inb(0x64) & 1) && flush_count < 100) {
        inb(0x60);
        flush_count++;
        qemu_delay();
    }
    
    // Phase 2: Configuration minimale pour QEMU
    print_string_serial("Phase 2: Configuration QEMU...\n");
    
    // Désactivation temporaire des ports
    if (wait_kbd_ready(1)) {
        outb(0x64, 0xAD); // Disable port 1
        qemu_delay();
    }
    
    if (wait_kbd_ready(1)) {
        outb(0x64, 0xA7); // Disable port 2  
        qemu_delay();
    }
    
    // Flush après désactivation
    flush_count = 0;
    while ((inb(0x64) & 1) && flush_count < 50) {
        inb(0x60);
        flush_count++;
        qemu_delay();
    }
    
    // Configuration optimale pour QEMU avec scancode Set 1 (renforcée pour console)
    print_string_serial("Phase 2: Configuration PS/2 renforcée...\n");
    if (wait_kbd_ready(1)) {
        outb(0x64, 0x20); // Read configuration
        qemu_delay();
        
        if (wait_kbd_ready(0)) {
            uint8_t config = inb(0x60);
            qemu_delay();
            
            print_string_serial("Configuration actuelle: 0x");
            char hex[] = "0123456789ABCDEF";
            write_serial(hex[(config >> 4) & 0xF]);
            write_serial(hex[config & 0xF]);
            print_string_serial("\n");
            
            // Configuration PS/2 Set 1 compatible renforcée
            config |= 0x01;  // Enable port 1 interrupt
            config &= ~0x10; // Enable port 1 clock  
            config |= 0x40;  // Enable scancode translation (Set 1)
            config &= ~0x20; // Disable port 2 interrupt (focus sur port 1)
            
            if (wait_kbd_ready(1)) {
                outb(0x64, 0x60); // Write configuration
                qemu_delay();
                
                if (wait_kbd_ready(1)) {
                    outb(0x60, config);
                    qemu_delay();
                    
                    print_string_serial("Nouvelle configuration: 0x");
                    write_serial(hex[(config >> 4) & 0xF]);
                    write_serial(hex[config & 0xF]);
                    print_string_serial("\n");
                    print_string_serial("Phase 2: Configuration PS/2 Set 1 appliquée\n");
                }
            }
        }
    }
    
    // Réactivation du port 1
    if (wait_kbd_ready(1)) {
        outb(0x64, 0xAE); // Enable port 1
        qemu_delay();
        print_string_serial("Phase 2: Port 1 réactivé\n");
    }
    
    // Phase 3: Configuration du périphérique simple
    print_string_serial("Phase 3: Configuration périphérique...\n");
    
    // Activer le scanning (simple pour QEMU)
    if (wait_kbd_ready(1)) {
        outb(0x60, 0xF4); // Enable scanning
        qemu_delay();
        
        // Attendre ACK éventuel
        int ack_wait = 5000;
        while (ack_wait-- > 0) {
            if (inb(0x64) & 1) {
                uint8_t response = inb(0x60);
                if (response == 0xFA) {
                    print_string_serial("Phase 3: Scanning activé\n");
                    break;
                }
            }
            qemu_delay();
        }
    }
    
    // Phase 4: Finalisation et test
    print_string_serial("Phase 4: Finalisation...\n");
    qemu_long_delay(); // Pause de stabilisation
    
    // Nettoyage final
    int flush_count2 = 0;
    while ((inb(0x64) & 1) && flush_count2 < 20) {
        inb(0x60);
        flush_count2++;
        qemu_delay();
    }
    
    initialization_phase = 0;
    print_string_serial("=== KEYBOARD INIT COMPLETE ===\n");
    print_string_serial("Mode: Interruption + Polling Fallback\n");
    print_string_serial("Compatible: Console et GUI\n");
    print_string_serial("Ready for input!\n");
    print_string_serial("===============================\n");
}

// Fonction getchar hybride avec plusieurs mécanismes et protection anti-fantômes
char keyboard_getc(void) {
    static int getc_calls = 0;
    static int consecutive_empty_returns = 0;
    char c;
    int attempts = 0;
    const int MAX_ATTEMPTS = 200000; // Timeout raisonnable
    const int MAX_CONSECUTIVE_EMPTY = 5; // Maximum de retours vides consécutifs
    
    getc_calls++;
    
    // Debug limité
    if (getc_calls <= 3) {
        print_string_serial("GETC: start (call #");
        write_serial('0' + (getc_calls % 10));
        print_string_serial(")\n");
    }
    
    // Réactiver les interruptions
    asm volatile("sti");
    
    while (attempts < MAX_ATTEMPTS) {
        // 1. Essayer d'abord le buffer d'interruptions
        if (kbd_get_char_nonblock(&c)) {
            // Filtrer: uniquement ASCII imprimable + contrôle utiles
            if ((c >= 32 && c <= 126) || c == '\n' || c == '\r' || c == '\t' || c == '\b') {
                consecutive_empty_returns = 0; // Reset compteur
                if (getc_calls <= 3) {
                    print_string_serial("GETC: got valid '");
                    write_serial(c);
                    print_string_serial("' from buffer\n");
                }
                return c;
            } else {
                continue;
            }
        }
        
        // 2. Polling de secours (actif même avec interruptions)
        keyboard_poll_check();
        
        // 3. Vérifier à nouveau le buffer
        if (kbd_get_char_nonblock(&c)) {
            if ((c >= 32 && c <= 126) || c == '\n' || c == '\r' || c == '\t' || c == '\b') {
                consecutive_empty_returns = 0; // Reset compteur
                if (getc_calls <= 3) {
                    print_string_serial("GETC: got valid '");
                    write_serial(c);
                    print_string_serial("' from polling\n");
                }
                return c;
            } else {
                continue;
            }
        }
        
        attempts++;
        
        // Debug périodique - réduit si trop de tentatives vides
        if (attempts % 100000 == 0 && getc_calls <= 1) {
            print_string_serial("GETC: waiting... (");
            write_serial('0' + (attempts / 50000));
            print_string_serial(")\n");
        }
        
        // Protection contre les boucles infinies de caractères vides
        if (attempts > MAX_ATTEMPTS / 2) {
            consecutive_empty_returns++;
            if (consecutive_empty_returns > MAX_CONSECUTIVE_EMPTY) {
                // Pause très courte et silencieuse
                for (volatile int pause = 0; pause < 10000; pause++);
                consecutive_empty_returns = 0; // Reset
            }
        }
    }
    
    // Timeout - ne retourner des caractères nuls qu'en dernier recours
    consecutive_empty_returns++;
    if (getc_calls <= 2) {
        print_string_serial("GETC: timeout après filtrage\n");
    }
    
    // Au lieu de retourner 0 (qui cause les touches fantômes), attendre plus
    if (consecutive_empty_returns < MAX_CONSECUTIVE_EMPTY) {
        // Pause et retry une dernière fois
        for (volatile int pause = 0; pause < 50000; pause++);
        if (kbd_get_char_nonblock(&c) && c != 0) {
            consecutive_empty_returns = 0;
            return c;
        }
    }
    
    return 0; // Caractère null seulement après tous les filtres
}

// Fonctions de compatibilité
char scancode_to_ascii(uint8_t scancode) {
    return map_scancode(scancode);
}

// Diagnostic complet
void keyboard_diagnostic() {
    print_string_serial("=== DIAGNOSTIC CLAVIER HYBRID ===\n");
    
    uint8_t status = inb(0x64);
    uint8_t pic_mask = inb(0x21);
    
    print_string_serial("Status: 0x");
    char hex[] = "0123456789ABCDEF";
    write_serial(hex[(status >> 4) & 0xF]);
    write_serial(hex[status & 0xF]);
    print_string_serial(" | PIC: 0x");
    write_serial(hex[(pic_mask >> 4) & 0xF]);
    write_serial(hex[pic_mask & 0xF]);
    print_string_serial("\n");
    
    print_string_serial("IRQ1: ");
    print_string_serial((pic_mask & 2) ? "MASKED" : "ACTIVE");
    print_string_serial("\n");
    
    print_string_serial("Interruptions: ");
    write_serial('0' + (debug_interrupt_count % 10));
    print_string_serial(" | Polling: ");
    write_serial('0' + (debug_polling_count % 10));
    print_string_serial("\n");
    
    print_string_serial("Mode principal: ");
    if (debug_interrupt_count > 0) {
        print_string_serial("INTERRUPTION");
        if (polling_fallback_active) print_string_serial(" + FALLBACK");
    } else if (polling_fallback_active) {
        print_string_serial("POLLING");
    } else {
        print_string_serial("ATTENTE");
    }
    print_string_serial("\n");
    
    print_string_serial("===============================\n");
}