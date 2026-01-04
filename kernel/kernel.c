#include "kernel/vga.h"
#include "kernel/input.h"

// Простой strcmp
int strcmp(const char* s1, const char* s2) {
    while(*s1 && (*s1 == *s2)) {
        s1++;
        s2++;
    }
    return *(unsigned char*)s1 - *(unsigned char*)s2;
}

void kernel_main() {
    // Отключаем прерывания на время инициализации
    asm volatile("cli");
    
    clear_screen(' ', 0x00);
    println("Vistas OS 0.3 Restore", 0x07);
    println("enter 'help' to view commands", 0xA0);
    
    // Прерывания НЕ включаем - работаем с polling
    // asm volatile("sti");  // НЕ включаем!
    
    while(1) {
        print("_> ", 0x07);
        char* cmd = input(0x07);  // Будет работать через polling
        
        if(strcmp(cmd, "help") == 0) {
            println("=== Available commands ===", 0x0E);
            println("help    - show this help", 0x0A);
            println("clear   - clear screen", 0x0A);
            println("sysinfo - system info", 0x0A);
        } 
        else if(strcmp(cmd, "clear") == 0) {
            clear_screen(' ', 0x00);
        }
        else if(strcmp(cmd, "sysinfo") == 0) {
            //println("System: Vistas OS", 0x0B);
            //println("Version: 0.1", 0x0B);
            //println("Memory: 640K base", 0x0B);
            //println("VGA: 80x25 text mode", 0x0B);

            println("                                 |", 0x04);
            println("     #######################     |", 0x04);
            println("   ###########################   |         ==SYSINFO==", 0x04);
            println("  #####                   -####  |", 0x04);
            println("  ####                     ####  |", 0x04);
            println("  ###.          ###         ###  |", 0x04);
            println("  ###.        ###           ###  |", 0x04);
            println("  ###.       ###            ###  |", 0x04);
            println("  ###.       ####           ###  |    I'm too lazy to do this", 0x04);
            println("  ###.        #####         ###  |", 0x04);
            println("  ###.          #####       ###  |", 0x04);
            println("  ###.          #####-      ###  |", 0x04);
            println("  ####       #########     ####  |", 0x04);
            println("  ##### ##############    #####  |", 0x04);
            println("   ###########################   |", 0x04);
            println("     +######################     |", 0x04);
            println("                                 |", 0x04);

        }
        else {
            print("Unknown command: ", 0x04);
            println(cmd, 0x04);
        }
    }
}