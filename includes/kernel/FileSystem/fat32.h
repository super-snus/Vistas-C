#ifndef FAT32_H
#define FAT32_H

typedef unsigned char  u8;
typedef unsigned short u16;
typedef unsigned int   u32;

typedef struct {
    char name[13];     // Имя файла
    u32 size;          // Размер
    u8 attr;           // Папка или нет
    u32 first_cluster; // Номер кластера
} fat_node_t; // <--- ИМЯ ТУТ

// Прототипы функций
int fat32_init(void);
int fat32_get_dir_contents(fat_node_t *nodes, int max_nodes); // НОВОЕ
int fat32_read(const char* filename, u8* buffer, u32 max_size);
int fat32_create(const char* filename);
int fat32_mkdir(const char* dirname);
void fat32_cd(u32 cluster); // НОВОЕ

extern u32 current_dir_cluster; // Номер текущей папки

#endif
