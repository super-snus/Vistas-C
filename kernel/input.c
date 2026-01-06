#include "kernel/input.h"
#include "kernel/vga.h"

extern unsigned int cursor_pos;

#define KEYBOARD_DATA_PORT 0x60
#define KEYBOARD_STATUS_PORT 0x64

static char buffer[256];
static int buf_pos = 0;

// Карта сканкодов (только основные клавиши)
static char keymap_normal[128] = {
    0, 0, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', '\b',
    '\t', 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n',
    0, 'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`',
    0, '\\', 'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/', 0,
    '*', 0, ' ', 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, '-', 0, 0, 0, '+', 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};

// Ждать готовности клавиатуры
static void keyboard_wait() {
    while(1) {
        unsigned char status;
        asm volatile("inb %%dx, %%al" : "=a"(status) : "d"(KEYBOARD_STATUS_PORT));
        if(!(status & 0x02)) break;
    }
}

// Прочитать сканкод
static unsigned char keyboard_read() {
    keyboard_wait();
    unsigned char data;
    asm volatile("inb %%dx, %%al" : "=a"(data) : "d"(KEYBOARD_DATA_PORT));
    return data;
}

// Получить нажатую клавишу
static int get_key() {
    keyboard_wait();
    unsigned char status;
    asm volatile("inb %%dx, %%al" : "=a"(status) : "d"(KEYBOARD_STATUS_PORT));
    
    if(status & 0x01) {
        return keyboard_read();
    }
    return -1;
}

// Инициализация клавиатуры
void init_keyboard() {
    keyboard_wait();
    asm volatile("outb %%al, %%dx" : : "a"(0xAE), "d"(KEYBOARD_STATUS_PORT));
    
    keyboard_wait();
    asm volatile("outb %%al, %%dx" : : "a"(0xFF), "d"(KEYBOARD_DATA_PORT));
    
    // Ждем сброс
    for(volatile int i = 0; i < 10000; i++);
}  

char* input(unsigned char color) {
    buf_pos = 0;
    buffer[0] = '\0';
    
    init_keyboard();  // Инициализируем при каждом вводе на всякий случай
    
    while(1) {
        int scancode = get_key();
        if(scancode == -1) {
            // Небольшая пауза если нет данных
            for(volatile int i = 0; i < 100; i++);
            continue;
        }
        
        // Обработка сканкода
        if(scancode == 0xE0) { // Extended key
            // Пропускаем extended
            get_key();
            continue;
        }
        
        // Проверяем release (верхний бит установлен)
        if(scancode & 0x80) {
            continue;  // Игнорируем отпускание клавиш
        }
        
        char ch = keymap_normal[scancode & 0x7F];
        
        if(scancode == 0x1C) { // Enter
            putchar('\n', color);
            buffer[buf_pos] = '\0';
            return buffer;
        }
        else if(scancode == 0x0E) { // Backspace
            if(buf_pos > 0) {
                buf_pos--;
                buffer[buf_pos] = '\0';
                
                // Стираем с экрана
                if(cursor_pos > 0) {
                    cursor_pos--;
                    unsigned short* vga = (unsigned short*)0xB8000;
                    vga[cursor_pos] = (color << 8) | ' ';
                }
            }
        }
        else if(ch && buf_pos < 255) {
            buffer[buf_pos++] = ch;
            buffer[buf_pos] = '\0';
            putchar(ch, color);
        }
    }
}