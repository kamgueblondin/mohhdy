#ifndef INTERRUPTS_H
#define INTERRUPTS_H

#include "task/task.h" /* cpu_state_t */

/* Every registered handler receives the saved CPU frame of the interrupt,
 * the same frame the assembly stubs pass to timer_handler and
 * syscall_handler. Handlers that do not need it ignore the argument. */
typedef void (*interrupt_handler_t)(cpu_state_t *cpu);

void pic_remap(void);
void interrupts_init(void);
void register_interrupt_handler(unsigned char interrupt, interrupt_handler_t handler);
void interrupt_handler(unsigned char interrupt_number, cpu_state_t *cpu);

#endif

