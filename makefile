CC = gcc
LD = ld
ASM = nasm
CFLAGS = -m32 -ffreestanding -nostdlib -fno-pie -fno-stack-protector
LDFLAGS = -m elf_i386 -T linker.ld -nostdlib
ASFLAGS = -f elf32

# Автоматический поиск ВСЕХ .c файлов в kernel/ и подпапках
KERNEL_SRC = $(shell find kernel -name "*.c" 2>/dev/null || echo "")
KERNEL_OBJ = $(KERNEL_SRC:.c=.o)

# Автоматический поиск ВСЕХ папок в includes/ для -I
INCLUDE_DIRS = $(shell find includes -type d 2>/dev/null || echo "")
CFLAGS += $(addprefix -I,$(INCLUDE_DIRS))

# Файлы загрузчика (фиксированные)
BOOT_OBJ = boot/boot.o boot/kernel_entry.o

.PHONY: all clean run iso

all: myos.iso

kernel/kernel.bin: $(BOOT_OBJ) $(KERNEL_OBJ)
	@mkdir -p $(@D)
	$(LD) $(LDFLAGS) -o $@ $^

# Автоматическое правило для ЛЮБЫХ .c файлов в kernel/
kernel/%.o: kernel/%.c
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) -c $< -o $@

# Автоматическое правило для C файлов в ЛЮБОЙ подпапке kernel/
%.o: %.c
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) -c $< -o $@

# Файлы загрузчика
boot/%.o: boot/%.asm
	@mkdir -p $(@D)
	$(ASM) $(ASFLAGS) $< -o $@

# Создание ISO
iso: kernel/kernel.bin
	mkdir -p isodir/boot/grub
	cp kernel/kernel.bin isodir/boot/
	echo 'menuentry "MyOS" { multiboot /boot/kernel.bin }' > isodir/boot/grub/grub.cfg

myos.iso: iso
	grub-mkrescue -o myos.iso isodir 2>/dev/null || \
	echo "Установи grub-mkrescue или используй xorriso"

run: myos.iso
	qemu-system-x86_64 -cdrom myos.iso

clean:
	rm -f $(KERNEL_OBJ) $(BOOT_OBJ) kernel/kernel.bin myos.iso
	rm -rf isodir

# Вывод информации
info:
	@echo "Найдены C файлы:"
	@for f in $(KERNEL_SRC); do echo "  $$f"; done
	@echo "Папки includes:"
	@for d in $(INCLUDE_DIRS); do echo "  $$d"; done