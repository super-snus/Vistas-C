CC = gcc
LD = ld
ASM = nasm
CFLAGS = -m32 -ffreestanding -nostdlib -fno-pie -fno-stack-protector -Wall -Wextra -O0 -g
LDFLAGS = -m elf_i386 -T linker.ld -nostdlib -Map=kernel.map
ASFLAGS = -f elf32

# Автоматический поиск ВСЕХ .c файлов в kernel/ и подпапках
KERNEL_SRC = $(shell find kernel -name "*.c" 2>/dev/null || echo "")
KERNEL_OBJ = $(KERNEL_SRC:.c=.o)

# Автоматический поиск ВСЕХ папок в includes/ для -I
INCLUDE_DIRS = $(shell find includes -type d 2>/dev/null || echo "")
CFLAGS += $(addprefix -I,$(INCLUDE_DIRS))

# Файлы загрузчика (фиксированные)
BOOT_OBJ = boot/boot.o boot/kernel_entry.o

.PHONY: all clean run iso disk test

all: myos.iso

kernel/kernel.bin: $(BOOT_OBJ) $(KERNEL_OBJ)
	@mkdir -p $(@D)
	@echo "Linking kernel..."
	$(LD) $(LDFLAGS) -o $@ $^
	@echo "Kernel size:"
	@wc -c < kernel/kernel.bin

# Автоматическое правило для .c файлов
%.o: %.c
	@mkdir -p $(@D)
	@echo "Compiling $<..."
	$(CC) $(CFLAGS) -c $< -o $@

# Файлы загрузчика
boot/%.o: boot/%.asm
	@mkdir -p $(@D)
	@echo "Assembling $<..."
	$(ASM) $(ASFLAGS) $< -o $@

# Создание ISO
iso: kernel/kernel.bin
	mkdir -p isodir/boot/grub
	cp kernel/kernel.bin isodir/boot/
	echo 'menuentry "MyOS" { multiboot /boot/kernel.bin }' > isodir/boot/grub/grub.cfg

myos.iso: iso
	grub-mkrescue -o myos.iso isodir 2>/dev/null || \
	echo "Note: ISO created without fancy boot sector"

# Создание тестового диска FAT32
disk.img:
	@echo "Creating FAT32 disk image..."
	dd if=/dev/zero of=disk.img bs=1M count=16
	mkfs.fat -F32 disk.img
	@echo "Disk image created: disk.img"

# Копирование ядра на диск
install: kernel/kernel.bin disk.img
	@echo "Installing kernel to disk..."
	sudo mount disk.img /mnt 2>/dev/null || \
	  (sudo mkdir -p /mnt && sudo mount -o loop disk.img /mnt)
	sudo cp kernel/kernel.bin /mnt/KERNEL.BIN
	sudo umount /mnt
	@echo "Kernel installed as KERNEL.BIN"

# Запуск с реальным диском (не ISO)
run-disk: install
	qemu-system-i386 -drive file=disk.img,format=raw -boot c -serial stdio

# Запуск с диском и отладкой
debug: install
	qemu-system-i386 -drive file=disk.img,format=raw -boot c -s -S -serial stdio &
	gdb -ex "target remote localhost:1234" -ex "file kernel/kernel.bin"

run: myos.iso
	qemu-system-x86_64 -cdrom myos.iso -serial stdio

clean:
	rm -f $(KERNEL_OBJ) $(BOOT_OBJ) kernel/kernel.bin myos.iso kernel.map disk.img
	rm -rf isodir
	sudo umount /mnt 2>/dev/null || true

# Вывод информации
info:
	@echo "=== MyOS Build Information ==="
	@echo "Kernel C files:"
	@for f in $(KERNEL_SRC); do echo "  $$f"; done
	@echo ""
	@echo "Include directories:"
	@for d in $(INCLUDE_DIRS); do echo "  $$d"; done
	@echo ""
	@echo "Total source files: $(words $(KERNEL_SRC))"
	@echo "CFLAGS: $(CFLAGS)"
	@echo "LDFLAGS: $(LDFLAGS)"