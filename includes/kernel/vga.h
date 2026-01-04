#ifndef VGA_H
#define VGA_H

void clear_screen(char ch, unsigned char color);
void print(const char* str, unsigned char color);
void println(const char* str, unsigned char color);
void putchar(char ch, unsigned char color);

#endif