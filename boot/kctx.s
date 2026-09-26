[BITS 32]
; Tranche 4 slice 3: kernel continuation for a task blocked inside a syscall
; while the Ring 3 atadriver serves its sector job. setjmp/longjmp style: the
; blocked task keeps its own per-task kernel stack untouched while other
; tasks run; kctx_resume jumps back onto it.
; kctx layout: ebx 0, esi 4, edi 8, ebp 12, esp 16, eip 20, eflags 24

section .text
global kctx_save, kctx_resume

; int kctx_save(uint32_t* ctx) -> 0 when saving, 1 when resumed
kctx_save:
    mov eax, [esp + 4]
    mov [eax + 0], ebx
    mov [eax + 4], esi
    mov [eax + 8], edi
    mov [eax + 12], ebp
    lea ecx, [esp + 4]
    mov [eax + 16], ecx
    mov ecx, [esp]
    mov [eax + 20], ecx
    pushfd
    pop ecx
    mov [eax + 24], ecx
    xor eax, eax
    ret

; void kctx_resume(const uint32_t* ctx) - never returns
kctx_resume:
    mov eax, [esp + 4]
    mov ebx, [eax + 0]
    mov esi, [eax + 4]
    mov edi, [eax + 8]
    mov ebp, [eax + 12]
    mov esp, [eax + 16]
    mov ecx, [eax + 20]
    push dword [eax + 24]
    popfd
    mov eax, 1
    jmp ecx
