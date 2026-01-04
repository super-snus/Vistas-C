#include "kernel/vga.h"

#define VIDEO_MEMORY 0xB8000
#define WIDTH 80
#define HEIGHT 25

unsigned int cursor_pos = 0;  // <-- БЕЗ 'static'!

void clear_screen(char ch, unsigned char color) {
    unsigned short* vga = (unsigned short*)VIDEO_MEMORY;
    unsigned short value = (color << 8) | ch;
    
    for(int i = 0; i < WIDTH * HEIGHT; i++) {
        vga[i] = value;
    }
    cursor_pos = 0;
}

void putchar(char ch, unsigned char color) {
    unsigned short* vga = (unsigned short*)VIDEO_MEMORY;
    
    if(ch == '\n') {
        cursor_pos = ((cursor_pos / WIDTH) + 1) * WIDTH;
    } else {
        vga[cursor_pos++] = (color << 8) | ch;
    }
    
    if(cursor_pos >= WIDTH * HEIGHT) {
        cursor_pos = 0;
        clear_screen(' ', 0x00);
    }
}

void print(const char* str, unsigned char color) {
    for(int i = 0; str[i] != '\0'; i++) {
        putchar(str[i], color);
    }
}

void println(const char* str, unsigned char color) {
    print(str, color);
    putchar('\n', color);
}