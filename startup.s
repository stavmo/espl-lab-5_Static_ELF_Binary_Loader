section .rodata
str1:   db "(:^..^:) starting up the code",10,0
        
section .text
global startup:function startup_end - startup

startup:
        push    ebp             ; Save caller state
        mov     ebp, esp
        sub     esp, 4          ; Leave space for local var on stack
        pushad                  ; Save more registers

        mov     ecx, [ebp+8]    ; Gets argc from C
        mov     ebx, [ebp+12]   ; Gets argv (char**) from C

        mov     eax, ecx
        dec     eax             ; skip program name
        shl     eax, 2          ; eax = 4 * (argc - 1) Push evry char* of argv for _start

.loop1:
        cmp     eax, 0
        jl      .done_args
        mov     edx, ebx
        add     edx, eax
        push    dword [edx]
        sub     eax, 4
        jmp     .loop1

.done_args:
        ; [ebp+8]  = argc
        ; [ebp+12] = argv

        mov edx, [ebp+12]	; Load argv (pointer to array of char*)
        push edx		; Push argv (second argument to main)
        mov     ecx, [ebp+8]    ; Gets argc from C
        push    ecx             ; Push argc for _start

        ; At this point, stack is ready to call a main(argc, argv)

        mov     ebx, [ebp+16]   ; Gets the function address from C
        jmp    ebx             ; Jumps to  _start

        mov     [ebp-4], eax    ; Save return value
        popad                   ; Restore caller state (registers)
        mov     eax, [ebp-4]    ; Place returned value where caller can see it
        add     esp, 4          ; Deallocate local variable
        pop     ebp             ; Restore caller state (ebp)
        ret			; Back to caller

startup_end:
