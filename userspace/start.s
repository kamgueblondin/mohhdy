.section .note.GNU-stack,"",@progbits

.section .text
.global _start

_start:
    # La pile est déjà initialisée par le noyau.
    # On peut l'utiliser directement.
    
    # Construire argc/argv minimal depuis EBX (le noyau place la question dans EBX)
    # argv = { NULL, (char*)EBX, NULL }, argc = 2
    movl %ebx, %eax          # EAX = pointeur question (ou 0)
    subl $12, %esp           # Allouer 3 pointeurs (argv[0..2])
    movl $0, 0(%esp)         # argv[0] = NULL (nom programme inconnu)
    movl %eax, 4(%esp)       # argv[1] = question (peut etre NULL)
    movl $0, 8(%esp)         # argv[2] = NULL
    push %esp                # push &argv[0]
    push $2                  # push argc = 2
    call main
    addl $8, %esp            # nettoyer les 2 arguments pushes
    addl $12, %esp           # liberer l'espace argv temporaire
    
    # Si main() retourne, appeler exit
    movl %eax, %ebx         # Code de retour de main
    movl $0, %eax           # SYS_EXIT
    int $0x80               # Appel système
    
    # Boucle infinie au cas où
    jmp .

