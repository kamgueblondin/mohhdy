; userspace_switch.s - Transition sécurisée vers l'espace utilisateur
bits 32

section .text

global switch_to_userspace
switch_to_userspace:
    ; Arguments: EIP utilisateur, ESP utilisateur
    ; [esp+4] = EIP utilisateur
    ; [esp+8] = ESP utilisateur
    
    cli                     ; Désactiver les interruptions temporairement
    
    mov eax, [esp+4]        ; EIP utilisateur
    mov ebx, [esp+8]        ; ESP utilisateur
    
    ; Configurer les segments de données utilisateur
    mov cx, 0x23            ; Sélecteur de segment de données utilisateur (Ring 3)
    mov ds, cx
    mov es, cx
    mov fs, cx
    mov gs, cx
    
    ; Préparer la pile pour iret
    push 0x23               ; SS (segment de pile utilisateur)
    push ebx                ; ESP (pointeur de pile utilisateur)
    pushfd                  ; EFLAGS
    pop ecx
    or ecx, 0x200           ; Activer les interruptions
    push ecx
    push 0x1B               ; CS (segment de code utilisateur)
    push eax                ; EIP (point d'entrée utilisateur)
    
    ; Transition vers l'espace utilisateur
    iret

