#ifndef SCHED_QUANTUM_H
#define SCHED_QUANTUM_H

#include <stdint.h>

/* IRQ0 preemption quantum, in timer ticks (100 Hz: 20 ticks = 200 ms). */
#define TIMER_PREEMPT_QUANTUM 20U

/* IRQ0 preemption decision, kept pure so it can be unit tested.
 *
 * The quantum is measured from the moment the CURRENT task was scheduled
 * (task->last_scheduled_ticks), not from the last global preemption. With a
 * global stamp, a task woken by IPC or by a cooperative switch after a long
 * idle period was preempted on its very first tick: an ipc server or a
 * granted child was cut in the middle of a line, control went back to the
 * shell, the shell parked in SYS_GETS (kernel frame, never preempted) and
 * the cut task starved until the next shell command. */
static inline int sched_quantum_preempt_due(int user_frame, int current_is_user,
                                            int other_ready_user, uint32_t now,
                                            uint32_t scheduled_at) {
    if (!user_frame || !current_is_user || !other_ready_user) return 0;
    return (uint32_t)(now - scheduled_at) >= TIMER_PREEMPT_QUANTUM;
}

#endif
