global load_page_directory
global enable_paging
global read_cr0
global write_cr0

; Charge un nouveau répertoire de pages dans CR3
load_page_directory:
    push ebp
    mov ebp, esp
    mov eax, [esp + 8]  ; Récupère l'adresse du répertoire de pages
    mov cr3, eax        ; Charge l'adresse dans CR3
    mov esp, ebp
    pop ebp
    ret

; Active le paging en mettant le bit 31 de CR0
enable_paging:
    push ebp
    mov ebp, esp
    mov eax, cr0        ; Lit la valeur actuelle de CR0
    or eax, 0x80000000  ; Met le bit 31 (PG - Paging)
    mov cr0, eax        ; Écrit la nouvelle valeur dans CR0
    mov esp, ebp
    pop ebp
    ret

; Lit la valeur du registre CR0
read_cr0:
    mov eax, cr0
    ret

; Écrit une valeur dans le registre CR0
write_cr0:
    push ebp
    mov ebp, esp
    mov eax, [esp + 8]  ; Récupère la valeur à écrire
    mov cr0, eax        ; Écrit dans CR0
    mov esp, ebp
    pop ebp
    ret

; Section GNU stack (sécurité - pile non exécutable)
section .note.GNU-stack
