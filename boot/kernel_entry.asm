global _start
extern kernel_main

section .text
_start:
    mov esp, stack_top
    call kernel_main
    cli
    hlt
    jmp $

section .bss
align 16
stack_bottom:
    resb 16384
stack_top: