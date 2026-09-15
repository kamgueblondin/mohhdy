#ifndef INTERRUPTS_H
#define INTERRUPTS_H

// Type pour les handlers d'interruption
typedef void (*interrupt_handler_t)();

void pic_remap();
void interrupts_init();
void register_interrupt_handler(unsigned char interrupt, interrupt_handler_t handler);
void interrupt_handler(unsigned char interrupt_number);

#endif

