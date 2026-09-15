#include "idt.h"
#include "keyboard.h"
#include "timer.h"

// Déclaration pour le nouveau handler
void keyboard_interrupt_handler();
void io_delay();

// Fonctions externes pour les ports I/O (définies dans kernel.c)
extern unsigned char inb(unsigned short port);
extern void outb(unsigned short port, unsigned char data);
extern void print_string_serial(const char* str);

// Externe pour les ISR qui seront définies en assembleur
extern void isr0(); extern void isr1(); extern void isr2(); extern void isr3();
extern void isr4(); extern void isr5(); extern void isr6(); extern void isr7();
extern void isr8(); extern void isr9(); extern void isr10(); extern void isr11();
extern void isr12(); extern void isr13(); extern void isr14(); extern void isr15();
extern void isr16(); extern void isr17(); extern void isr18(); extern void isr19();
extern void isr20(); extern void isr21(); extern void isr22(); extern void isr23();
extern void isr24(); extern void isr25(); extern void isr26(); extern void isr27();
extern void isr28(); extern void isr29(); extern void isr30(); extern void isr31();
extern void irq0(); // ISR pour le timer
extern void irq1(); // ISR pour le clavier
extern void irq3(); // ISR pour la NE2000
extern void isr_syscall(); // ISR pour les appels système
extern void isr_schedule(); // ISR pour le scheduling volontaire

// Structure pour les registres passés par l'ISR stub
typedef struct {
    uint32_t gs, fs, es, ds;
    uint32_t edi, esi, ebp, esp, ebx, edx, ecx, eax;
    uint32_t int_no, err_code;
    uint32_t eip, cs, eflags, useresp, ss;
} registers_t;

// C-level fault handler
void fault_handler_c(registers_t *r) {
    print_string_serial("\n!!! KERNEL EXCEPTION !!!\n");
    print_string_serial("Interrupt: ");
    print_hex_serial(r->int_no);
    print_string_serial("\nError Code: ");
    print_hex_serial(r->err_code);
    print_string_serial("\nEIP: ");
    print_hex_serial(r->eip);
    print_string_serial("\n");

    if (r->int_no == 14) { // Page Fault
        uint32_t faulting_address;
        asm volatile("mov %%cr2, %0" : "=r" (faulting_address));
        print_string_serial("Page Fault at address ");
        print_hex_serial(faulting_address);
        print_string_serial("\n");
    }

    print_string_serial("System Halted.\n");
    for(;;);
}

// Type pour les handlers d'interruption
typedef void (*interrupt_handler_t)();

// Table des handlers d'interruption
static interrupt_handler_t interrupt_handlers[256];

// Fonction pour remapper le PIC
// Par défaut, les IRQs du PIC (0-15) entrent en conflit avec les exceptions CPU.
// On les décale vers les entrées IDT 32-47.
void pic_remap() {
    print_string_serial("PIC: Début du remappage...\n");
    
    // Sauvegarde les masques actuels
    unsigned char a1, a2;
    a1 = inb(0x21);
    a2 = inb(0xA1);
    print_string_serial("PIC: Masques sauvegardés\n");
    
    // Démarre la séquence d'initialisation ICW1 (en mode cascade)
    outb(0x20, 0x11);  // PIC1: ICW1 - Initialisation + ICW4 needed + Cascade mode
    io_delay();
    outb(0xA0, 0x11);  // PIC2: ICW1
    io_delay();
    print_string_serial("PIC: ICW1 envoyé\n");
    
    // ICW2: Offset de vecteur
    outb(0x21, 0x20);  // PIC1: offset de vecteur 0x20 (32)
    io_delay();
    outb(0xA1, 0x28);  // PIC2: offset de vecteur 0x28 (40)
    io_delay();
    print_string_serial("PIC: ICW2 envoyé (offsets: PIC1=32, PIC2=40)\n");
    
    // ICW3: Configuration de la cascade
    outb(0x21, 0x04);  // PIC1: il y a un esclave PIC à IRQ2 (0000 0100)
    io_delay();
    outb(0xA1, 0x02);  // PIC2: son identité en cascade (0000 0010)
    io_delay();
    print_string_serial("PIC: ICW3 envoyé (cascade configurée)\n");
    
    // ICW4: Mode 8086/88
    outb(0x21, 0x01);  // PIC1: Mode 8086
    io_delay();
    outb(0xA1, 0x01);  // PIC2: Mode 8086
    io_delay();
    print_string_serial("PIC: ICW4 envoyé (mode 8086)\n");
    
    // Configuration optimale des masques pour QEMU
    // IRQ0 (timer) et IRQ1 (clavier) démasquées, autres masquées
    outb(0x21, 0xFC);  // 11111100b : IRQ0, IRQ1 démasquées uniquement
    io_delay();
    outb(0xA1, 0xFF);  // Toutes les IRQ du PIC2 masquées
    io_delay();
    
    print_string_serial("PIC: Masques configurés - IRQ0/IRQ1 activées\n");
    print_string_serial("PIC: Remappage terminé avec succès\n");
}

// Fonction de délai pour les opérations PIC (nécessaire sur hardware réel)
void io_delay() {
    // Courte pause pour laisser le temps au PIC de traiter
    asm volatile("outb %%al, $0x80" : : "a"(0));
}

// Fonction pour diagnostiquer l'état du PIC
void pic_diagnose() {
    print_string_serial("=== DIAGNOSTIC PIC ===\n");
    uint8_t mask1 = inb(0x21);
    uint8_t mask2 = inb(0xA1);
    
    print_string_serial("PIC1 mask: 0b");
    for (int i = 7; i >= 0; i--) {
        write_serial((mask1 & (1 << i)) ? '1' : '0');
    }
    print_string_serial(" (0x");
    char hex[3] = "00";
    hex[0] = (mask1 >> 4) < 10 ? '0' + (mask1 >> 4) : 'A' + (mask1 >> 4) - 10;
    hex[1] = (mask1 & 0xF) < 10 ? '0' + (mask1 & 0xF) : 'A' + (mask1 & 0xF) - 10;
    print_string_serial(hex);
    print_string_serial(")\n");
    
    print_string_serial("IRQ0 (timer): ");
    print_string_serial((mask1 & 1) ? "MASKED" : "ENABLED");
    print_string_serial("\n");
    
    print_string_serial("IRQ1 (keyboard): ");
    print_string_serial((mask1 & 2) ? "MASKED" : "ENABLED");
    print_string_serial("\n");
    
    print_string_serial("======================\n");
}

// Fonction pour envoyer EOI (End of Interrupt)
void pic_send_eoi(unsigned char irq) {
    if (irq >= 8) {
        outb(0xA0, 0x20);
    }
    outb(0x20, 0x20);
}

// Enregistre un handler pour une interruption donnée
void register_interrupt_handler(unsigned char interrupt, interrupt_handler_t handler) {
    interrupt_handlers[interrupt] = handler;
}

// ISR commune pour les interruptions
void interrupt_handler(unsigned char interrupt_number) {
    // Appelle le handler spécifique s'il existe
    if (interrupt_handlers[interrupt_number]) {
        interrupt_handlers[interrupt_number]();
    }
    
    // Envoie EOI pour les IRQs
    if (interrupt_number >= 32 && interrupt_number < 48) {
        pic_send_eoi(interrupt_number - 32);
    }
}

void install_exception_handlers() {
    idt_set_gate(0, (uint32_t)isr0, 0x08, 0x8E);
    idt_set_gate(1, (uint32_t)isr1, 0x08, 0x8E);
    idt_set_gate(2, (uint32_t)isr2, 0x08, 0x8E);
    idt_set_gate(3, (uint32_t)isr3, 0x08, 0x8E);
    idt_set_gate(4, (uint32_t)isr4, 0x08, 0x8E);
    idt_set_gate(5, (uint32_t)isr5, 0x08, 0x8E);
    idt_set_gate(6, (uint32_t)isr6, 0x08, 0x8E);
    idt_set_gate(7, (uint32_t)isr7, 0x08, 0x8E);
    idt_set_gate(8, (uint32_t)isr8, 0x08, 0x8E);
    idt_set_gate(9, (uint32_t)isr9, 0x08, 0x8E);
    idt_set_gate(10, (uint32_t)isr10, 0x08, 0x8E);
    idt_set_gate(11, (uint32_t)isr11, 0x08, 0x8E);
    idt_set_gate(12, (uint32_t)isr12, 0x08, 0x8E);
    idt_set_gate(13, (uint32_t)isr13, 0x08, 0x8E);
    idt_set_gate(14, (uint32_t)isr14, 0x08, 0x8E);
    idt_set_gate(15, (uint32_t)isr15, 0x08, 0x8E);
    idt_set_gate(16, (uint32_t)isr16, 0x08, 0x8E);
    idt_set_gate(17, (uint32_t)isr17, 0x08, 0x8E);
    idt_set_gate(18, (uint32_t)isr18, 0x08, 0x8E);
    idt_set_gate(19, (uint32_t)isr19, 0x08, 0x8E);
    idt_set_gate(20, (uint32_t)isr20, 0x08, 0x8E);
    idt_set_gate(21, (uint32_t)isr21, 0x08, 0x8E);
    idt_set_gate(22, (uint32_t)isr22, 0x08, 0x8E);
    idt_set_gate(23, (uint32_t)isr23, 0x08, 0x8E);
    idt_set_gate(24, (uint32_t)isr24, 0x08, 0x8E);
    idt_set_gate(25, (uint32_t)isr25, 0x08, 0x8E);
    idt_set_gate(26, (uint32_t)isr26, 0x08, 0x8E);
    idt_set_gate(27, (uint32_t)isr27, 0x08, 0x8E);
    idt_set_gate(28, (uint32_t)isr28, 0x08, 0x8E);
    idt_set_gate(29, (uint32_t)isr29, 0x08, 0x8E);
    idt_set_gate(30, (uint32_t)isr30, 0x08, 0x8E);
    idt_set_gate(31, (uint32_t)isr31, 0x08, 0x8E);
}

extern void ne2k_irq_handler(void);

// Initialise toutes nos interruptions
void interrupts_init() {
    print_string_serial("=== INITIALISATION SYSTEME INTERRUPTIONS ===\n");
    
    // 1. Désactiver les interruptions pendant l'initialisation
    asm volatile ("cli");
    print_string_serial("Step 1: Interruptions CPU désactivées\n");
    
    // 2. Installer les handlers d'exceptions CPU (0-31)
    install_exception_handlers();
    print_string_serial("Step 2: Handlers d'exceptions installés\n");

    // 3. Initialiser la table des handlers
    for (int i = 0; i < 256; i++) {
        interrupt_handlers[i] = 0;
    }
    print_string_serial("Step 3: Table des handlers initialisée\n");
    
    // 4. Remapper le PIC AVANT d'enregistrer les handlers
    print_string_serial("Step 4: Remapping du PIC...\n");
    pic_remap();
    print_string_serial("Step 4: PIC remappé avec succès\n");

    // 5. Enregistrer les handlers d'IRQ
    print_string_serial("Step 5: Enregistrement des handlers IRQ...\n");
    register_interrupt_handler(32, timer_handler);    // IRQ 0 - Timer
    register_interrupt_handler(33, keyboard_interrupt_handler); // IRQ 1 - Clavier
    register_interrupt_handler(35, ne2k_irq_handler); // IRQ 3 - NE2000 ISA
    print_string_serial("Step 5: Handlers IRQ enregistrés\n");
    
    // 6. Associer les entrées de l'IDT aux routines assembleur
    print_string_serial("Step 6: Configuration des entrées IDT...\n");
    idt_set_gate(32, (uint32_t)irq0, 0x08, 0x8E);        // Timer
    idt_set_gate(33, (uint32_t)irq1, 0x08, 0x8E);        // Clavier
    idt_set_gate(35, (uint32_t)irq3, 0x08, 0x8E);        // NE2000 ISA
    idt_set_gate(0x30, (uint32_t)isr_schedule, 0x08, 0xEE); // Scheduler (Ring 3)
    idt_set_gate(0x80, (uint32_t)isr_syscall, 0x08, 0xEE); // Syscalls (Ring 3 accessible)
    print_string_serial("Step 6: Entrées IDT configurées\n");

    // 7. Diagnostic du PIC avant activation
    print_string_serial("Step 7: Diagnostic PIC...\n");
    pic_diagnose();

    // 8. Forcer l'unmask des IRQ critiques
    print_string_serial("Step 8: Activation forcée des IRQ critiques...\n");
    outb(0x21, 0xF4);   // IRQ0, IRQ1 et IRQ3 activées
    outb(0xA1, 0xFF);   // Toutes les IRQ du PIC2 désactivées
    
    // Vérification finale
    uint8_t final_mask = inb(0x21);
    print_string_serial("Masque PIC final: 0x");
    char hex[3] = "00";
    hex[0] = (final_mask >> 4) < 10 ? '0' + (final_mask >> 4) : 'A' + (final_mask >> 4) - 10;
    hex[1] = (final_mask & 0xF) < 10 ? '0' + (final_mask & 0xF) : 'A' + (final_mask & 0xF) - 10;
    print_string_serial(hex);
    print_string_serial("\n");

    // 9. Activer les interruptions sur le CPU
    print_string_serial("Step 9: Activation des interruptions CPU...\n");
    asm volatile ("sti");
    
    // 10. Test immédiat des interruptions
    print_string_serial("Step 10: Test des interruptions...\n");
    
    print_string_serial("=== SYSTEME INTERRUPTIONS PRET ===\n");
    print_string_serial("IRQ0 (timer): activé\n");
    print_string_serial("IRQ1 (keyboard): activé\n");
    print_string_serial("Interruptions CPU: activées\n");
    print_string_serial("=====================================\n");
}

