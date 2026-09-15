/* spin.c - tâche de charge non coopérative pour valider la préemption IRQ0.
 * Aucun syscall n'est exécuté dans la boucle : le shell ne peut redevenir
 * réactif que si le timer peut reprendre une autre tâche Ring 3. */

volatile unsigned long spin_counter;

void main(void) {
    for (;;) {
        spin_counter++;
        asm volatile("" ::: "memory");
    }
}
