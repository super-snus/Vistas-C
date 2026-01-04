section .text
global keyboard_handler_asm

keyboard_handler_asm:
    pusha
    extern keyboard_handler
    call keyboard_handler
    popa
    iret