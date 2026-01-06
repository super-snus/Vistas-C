/* linker.ld - для ядра с поддержкой FAT32 */

ENTRY(_start)
SECTIONS
{
    /* Ядро загружается по адресу 1MB */
    . = 0x100000;
    
    /* Multiboot заголовок ДОЛЖЕН быть в первых 8KB */
    .text ALIGN(4K) : AT(ADDR(.text))
    {
        *(.multiboot)
        *(.text)
    }
    
    /* Read-only data */
    .rodata ALIGN(4K) : AT(ADDR(.rodata))
    {
        *(.rodata*)
    }
    
    /* Data section */
    .data ALIGN(4K) : AT(ADDR(.data))
    {
        *(.data)
    }
    
    /* BSS section */
    .bss ALIGN(4K) : AT(ADDR(.bss))
    {
        *(COMMON)
        *(.bss)
    }
    
    /* End of kernel */
    kernel_end = .;
}