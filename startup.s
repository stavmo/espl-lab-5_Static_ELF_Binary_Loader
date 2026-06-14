section .rodata
str1:   db "(:^..^:) starting up the code",10,0
        
section .text
global startup

startup:
        push    ebp
        mov     ebp, esp
        sub     esp, 4          ; Leave space for local var on stack
        pushad

        mov     ecx, [ebp+8]    ; Gets argc
        mov     ebx, [ebp+12]   ; Gets argv

        mov     eax, ecx
        dec     eax             ; skip program name
        shl     eax, 2          ; eax = 4 * (argc - 1) Push evry char* of argv for _start

        push    dword 0
        mov     ebx, [ebp+12]

.loop1:
        cmp     eax, 0
        jl      .done_args
        mov     edx, ebx
        add     edx, eax
        push    dword [edx]
        sub     eax, 4
        jmp     .loop1

.done_args:
        mov     ecx, [ebp+8]    ; Gets argc
        push    ecx             ; Push argc for _start

        mov     ebx, [ebp+16]   ; Gets the function address
        jmp     ebx

        mov     [ebp-4], eax    ; Save return value
        popad
        mov     eax, [ebp-4]
        add     esp, 4          ; Deallocate local variable
        pop     ebp             ; Restore caller state (ebp)
        ret

startup_end:
