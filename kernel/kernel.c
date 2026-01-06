#include "kernel/vga.h"
#include "kernel/input.h"
#include "kernel/FileSystem/fat32.h"


void init_keyboard(void);


// Добавь свою реализацию strncmp
int strncmp(const char* s1, const char* s2, int n) {
    for(int i = 0; i < n; i++) {
        if(s1[i] != s2[i]) return s1[i] - s2[i];
        if(s1[i] == '\0') return 0;
    }
    return 0;
}

// Простой strcmp (если ещё нет)
int strcmp(const char* s1, const char* s2) {
    while(*s1 && (*s1 == *s2)) {
        s1++;
        s2++;
    }
    return *(unsigned char*)s1 - *(unsigned char*)s2;
}

void kernel_main() {
    asm volatile("cli");
    
    // Выключаем курсор
    asm volatile("outb %%al, %%dx" : : "a"(0x0A), "d"(0x3D4));
    asm volatile("outb %%al, %%dx" : : "a"(0x20), "d"(0x3D5));
    
    clear_screen(' ', 0x00);
    println("Vistas OS v0.1", 0x07);
    println("Type 'help' for commands", 0x0A);
    
    // Инициализация FAT32
    if(fat32_init() == 0) {
        println("FAT32: Ready", 0x0A);
    } else {
        println("FAT32: No disk", 0x04);
    }
    
    init_keyboard();
    
    while(1) {
        print("> ", 0x07);
        char* cmd = input(0x07);
        
        if(strcmp(cmd, "help") == 0) {
            println("=== Commands ===", 0x0E);
            println("help    - show this", 0x0A);
            println("clear   - clear screen", 0x0A);
            println("sysinfo - system info", 0x0A);
            println("ls      - list files", 0x0A);
            println("mkfile  - create file", 0x0A);
        } 
        else if(strcmp(cmd, "clear") == 0) {
            clear_screen(' ', 0x00);
        }
        else if(strcmp(cmd, "sysinfo") == 0) {
            println("System: Vistas OS", 0x0B);
            println("Version: 0.1", 0x0B);
            println("FS: FAT32", 0x0B);
        }
        else if(strcmp(cmd, "ls") == 0) {
            println("Listing files...", 0x09);
            fat32_list();
        }
        else if(strncmp(cmd, "mkfile ", 7) == 0) {
            // Пример: "mkfile test.txt"
            if(fat32_create(cmd + 7) == 0) {
                print("Created: ", 0x0A);
                println(cmd + 7, 0x0A);
            } else {
                println("Failed to create", 0x04);
            }
        }
        else if(cmd[0] != '\0') {
            print("Unknown: ", 0x04);
            println(cmd, 0x04);
        }
    }
}