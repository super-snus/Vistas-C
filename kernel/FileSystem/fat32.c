// fat32.c - исправленный драйвер для Vistas OS

typedef unsigned char  u8;
typedef unsigned short u16;
typedef unsigned int   u32;

// Импортируем твои функции вывода из ядра
extern void print(const char* str, u8 color);
extern void println(const char* str, u8 color);

// Порты ATA
static inline void outb(u16 port, u8 val) {
    __asm__ volatile ("outb %0, %1" : : "a"(val), "Nd"(port));
}

static inline u8 inb(u16 port) {
    u8 ret;
    __asm__ volatile ("inb %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

static inline void insw(u16 port, void *addr, u32 cnt) {
    __asm__ volatile ("cld; rep insw" : "+D"(addr), "+c"(cnt) : "d"(port) : "memory");
}

static u32 fat_start_lba;
static u32 data_start_lba;
static u8 sectors_per_cluster;
static u16 bytes_per_sector;
static int fat32_ready = 0;

u32 current_dir_cluster = 2;

typedef struct {
    char name[13];     // Имя файла
    u32 size;          // Размер
    u8 attr;           // Папка или нет
    u32 first_cluster; // Номер кластера
} fat_node_t;

static void read_sector(u32 lba, u8 *buf) {
    outb(0x1F6, (lba >> 24) | 0xE0);
    outb(0x1F2, 1);
    outb(0x1F3, (u8)lba);
    outb(0x1F4, (u8)(lba >> 8));
    outb(0x1F5, (u8)(lba >> 16));
    outb(0x1F7, 0x20);
    
    while (inb(0x1F7) & 0x80);
    while (!(inb(0x1F7) & 0x08));
    
    insw(0x1F0, buf, 256);
}

static u32 get_next_cluster(u32 cluster) {
    u8 sector[512];
    u32 fat_offset = cluster * 4;
    u32 fat_sector = fat_start_lba + (fat_offset / 512);
    u32 entry_offset = fat_offset % 512;
    
    read_sector(fat_sector, sector);
    return *(u32*)(sector + entry_offset) & 0x0FFFFFFF;
}

static u32 cluster_to_lba(u32 cluster) {
    return data_start_lba + (cluster - 2) * sectors_per_cluster;
}

int fat32_init() {
    u8 bpb[512];
    read_sector(0, bpb); // Предполагаем, что BPB в секторе 0 (без MBR)
    
    if (bpb[510] != 0x55 || bpb[511] != 0xAA) return -1;
    
    bytes_per_sector = *(u16*)(bpb + 11);
    sectors_per_cluster = bpb[13];
    u16 reserved_sectors = *(u16*)(bpb + 14);
    u8 fat_count = bpb[16];
    u32 fat_size_sectors = *(u32*)(bpb + 36);
    
    fat_start_lba = reserved_sectors;
    data_start_lba = reserved_sectors + (fat_count * fat_size_sectors);
    
    fat32_ready = 1;
    return 0;
}

void fat32_list() {
    if (!fat32_ready) return;
    
    u8 sector[512];
    u32 cluster = 2; 
    
    while (cluster < 0x0FFFFFF8) {
        u32 lba = cluster_to_lba(cluster);
        read_sector(lba, sector);
        
        for (int i = 0; i < 512; i += 32) {
            if (sector[i] == 0x00) return;
            if (sector[i] == 0xE5) continue;
            if (sector[i + 11] == 0x0F) continue; // Пропуск длинных имен LFN

            char name[13];
            int p = 0;
            for (int j = 0; j < 8; j++) if(sector[i+j] != ' ') name[p++] = sector[i+j];
            if (sector[i+8] != ' ') {
                name[p++] = '.';
                for (int j = 0; j < 3; j++) if(sector[i+8+j] != ' ') name[p++] = sector[i+8+j];
            }
            name[p] = '\0';
            
            println(name, 0x07); // Используем нормальный вывод ядра
        }
        cluster = get_next_cluster(cluster);
    }
}

int fat32_read(const char* filename, u8* buffer, u32 max_size) {
    if (!fat32_ready) return -1;
    
    u8 sector[512];
    u32 cluster = 2;
    
    while (cluster < 0x0FFFFFF8) {
        read_sector(cluster_to_lba(cluster), sector);
        for (int i = 0; i < 512; i += 32) {
            if (sector[i] == 0x00) return -1;
            
            // Сравнение имени (очень упрощенно, без учета расширения)
            int match = 1;
            for(int j=0; j<8; j++) {
                if (filename[j] == '.' || filename[j] == '\0') break;
                if (sector[i+j] != filename[j]) { match = 0; break; }
            }

            if (match) {
                u32 cur_clus = *(u16*)(sector + i + 26) | (*(u16*)(sector + i + 20) << 16);
                u32 file_size = *(u32*)(sector + i + 28);
                u32 read_bytes = 0;

                while (cur_clus < 0x0FFFFFF8 && read_bytes < file_size && read_bytes < max_size) {
                    u32 lba = cluster_to_lba(cur_clus);
                    for (int s = 0; s < sectors_per_cluster; s++) {
                        read_sector(lba + s, buffer + read_bytes);
                        read_bytes += 512;
                        if (read_bytes >= file_size) break;
                    }
                    cur_clus = get_next_cluster(cur_clus);
                }
                return (int)file_size;
            }
        }
        cluster = get_next_cluster(cluster);
    }
    return -1;
}

static void write_sector(u32 lba, u8 *buf) {
    outb(0x1F6, (lba >> 24) | 0xE0);
    outb(0x1F2, 1);
    outb(0x1F3, (u8)lba);
    outb(0x1F4, (u8)(lba >> 8));
    outb(0x1F5, (u8)(lba >> 16));
    outb(0x1F7, 0x30); // Команда WRITE SECTORS

    // Ждем готовности принять данные (BSY=0, DRQ=1)
    while ((inb(0x1F7) & 0x88) != 0x08);

    // Записываем данные
    __asm__ volatile (
        "cld; rep outsw"
        : "+S"(buf) : "c"(256), "d"(0x1F0) : "memory"
    );

    // ВАЖНО: Ждем завершения записи на пластины/память
    for(int i = 0; i < 4; i++) inb(0x1F7); // Небольшая задержка чтения
    while (inb(0x1F7) & 0x80);             // Ждем пока BSY станет 0

    // Команда FLUSH CACHE (чтобы данные точно легли на диск)
    outb(0x1F7, 0xE7);
    while (inb(0x1F7) & 0x80); 
}



// Поиск первого свободного кластера в FAT
u32 find_free_cluster() {
    u8 sector[512]; // Обязательно массив, иначе затрешь стек ядра!
    u32 entries_per_sector = 128; // 512 байт / 4 байта на запись
    
    // Пройдемся по первым 128 секторам FAT (этого хватит для диска 64МБ)
    for (u32 i = 0; i < 128; i++) {
        read_sector(fat_start_lba + i, sector);
        u32* fat = (u32*)sector;
        
        for (u32 j = 0; j < entries_per_sector; j++) {
            // Игнорируем зарезервированные кластеры (0 и 1)
            if (i == 0 && j < 2) continue;

            if ((fat[j] & 0x0FFFFFFF) == 0x00000000) {
                return (i * entries_per_sector) + j;
            }
        }
    }
    return 0; // Нет места
}


int fat32_create(const char* filename) {
    if (!fat32_ready) return -1;

    u8 dir_buf[512];
    u32 root_lba = cluster_to_lba(2);
    read_sector(root_lba, dir_buf);

    int entry_idx = -1;
    for (int i = 0; i < 512; i += 32) {
        if (dir_buf[i] == 0x00 || dir_buf[i] == 0xE5) {
            entry_idx = i;
            break;
        }
    }
    if (entry_idx == -1) return -1;

    u32 free_clus = find_free_cluster();
    if (free_clus == 0) return -1;

    // Подготовка имени (8.3 формат)
    for (int j = 0; j < 11; j++) dir_buf[entry_idx + j] = ' ';
    for (int j = 0; j < 8 && filename[j] != '\0' && filename[j] != '.'; j++) {
        dir_buf[entry_idx + j] = filename[j];
    }
    // Если есть расширение (после точки)
    // (Тут можно дописать логику расширения, если нужно)

    dir_buf[entry_idx + 11] = 0x20; // Обычный файл
    *(u16*)(dir_buf + entry_idx + 26) = (u16)(free_clus & 0xFFFF);
    *(u16*)(dir_buf + entry_idx + 20) = (u16)((free_clus >> 16) & 0xFFFF);
    *(u32*)(dir_buf + entry_idx + 28) = 0; // Размер 0

    // Запись
    write_sector(root_lba, dir_buf);

    // Обновление FAT
    u8 fat_sector_buf[512];
    u32 f_sec_lba = fat_start_lba + (free_clus * 4 / 512);
    u32 f_off = (free_clus * 4) % 512;
    
    read_sector(f_sec_lba, fat_sector_buf);
    *(u32*)(fat_sector_buf + f_off) = 0x0FFFFFFF;
    write_sector(f_sec_lba, fat_sector_buf);

    return 0; // Теперь система не должна виснуть!
}

//функция для создания папки мемной
int fat32_mkdir(const char* dirname) {
    // 1. Создаем "файл" через твою функцию
    if (fat32_create(dirname) != 0) return -1;

    // 2. Находим эту запись в корне и меняем атрибут на 0x10
    u8 sector[512];
    u32 root_lba = cluster_to_lba(2);
    read_sector(root_lba, sector);

    u32 cluster_of_new_dir = 0;
    for (int i = 0; i < 512; i += 32) {
        // Упрощенно ищем по первой букве или имени
        if (sector[i] == dirname[0]) { 
            sector[i+11] = 0x10; // Ставим атрибут DIRECTORY
            cluster_of_new_dir = *(u16*)(sector + i + 26) | (*(u16*)(sector + i + 20) << 16);
            write_sector(root_lba, sector);
            break;
        }
    }

    // 3. Инициализируем кластер новой папки (пишем "." и "..")
    u8 new_dir_data[512];
    for(int i=0; i<512; i++) new_dir_data[i] = 0;
    
    // Запись "."
    for(int j=0; j<11; j++) new_dir_data[j] = (j==0) ? '.' : ' ';
    new_dir_data[11] = 0x10;
    *(u16*)(new_dir_data + 26) = (u16)(cluster_of_new_dir & 0xFFFF);
    
    // Запись ".."
    new_dir_data[32] = '.'; new_dir_data[33] = '.';
    for(int j=34; j<43; j++) new_dir_data[j] = ' ';
    new_dir_data[32+11] = 0x10;
    *(u16*)(new_dir_data + 32 + 26) = 2; // Ссылка на корень (кластер 2)

    write_sector(cluster_to_lba(cluster_of_new_dir), new_dir_data);
    return 0;
}


// НОВАЯ ФУНКЦИЯ: собирает файлы в массив, НИЧЕГО НЕ ПЕЧАТАЕТ
int fat32_get_dir_contents(fat_node_t *nodes, int max_nodes) {
    if (!fat32_ready) return 0;
    u8 sector[512];
    int count = 0;
    
    read_sector(cluster_to_lba(current_dir_cluster), sector);
    
    for (int i = 0; i < 512 && count < max_nodes; i += 32) {
        if (sector[i] == 0x00) break;
        if (sector[i] == 0xE5 || sector[i+11] == 0x0F) continue;

        fat_node_t *node = &nodes[count];
        node->attr = sector[i+11];
        node->size = *(u32*)(sector + i + 28);
        node->first_cluster = *(u16*)(sector + i + 26) | (*(u16*)(sector + i + 20) << 16);

        // Копируем имя
        int p = 0;
        for (int j = 0; j < 8; j++) if(sector[i+j] != ' ') node->name[p++] = sector[i+j];
        if (!(node->attr & 0x10) && sector[i+8] != ' ') {
            node->name[p++] = '.';
            for (int j = 0; j < 3; j++) if(sector[i+8+j] != ' ') node->name[p++] = sector[i+8+j];
        }
        node->name[p] = '\0';
        count++;
    }
    return count;
}

// НОВАЯ ФУНКЦИЯ: просто меняет текущий кластер
void fat32_cd(u32 cluster) {
    if (cluster >= 2) current_dir_cluster = cluster;
    else current_dir_cluster = 2; // если что-то не так, валим в корень
}