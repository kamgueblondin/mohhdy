# Outils de compilation
CC = gcc
AS = nasm
LD = ld

# Options de compilation
# -m32 : Compiler en 32-bit
# -ffreestanding : Ne pas utiliser la bibliothèque standard C
# -nostdlib : Ne pas lier avec la bibliothèque standard C
# -fno-pie : Produire du code indépendant de la position
CFLAGS = -m32 -ffreestanding -nostdlib -fno-pie -Wall -Wextra -O3 -msse2 -mfpmath=sse -mstackrealign -fomit-frame-pointer -I. -Iinclude -DCONFIG_UTF8_VGA=1
ASFLAGS = -f elf32

# Nom du fichier final de notre OS
OS_IMAGE = build/mohhdy.bin
ISO_IMAGE = build/mohhdy.iso
INITRD_IMAGE = my_initrd.tar
DISK_IMAGE ?= build/overlay.img
GGUF_DISK_IMAGE ?= build/gpt2_gguf_fat16.img
FAT32_SECONDARY_IMAGE ?= build/fat32_secondary.img
GPT2_GGUF_DEPLOY_MODEL ?= models/gpt2-Q3_K_M.gguf
DISK_SECTORS ?= 4224
FAT_BASE_LBA ?= 64
QEMU_DISK_OPTS = -drive file=$(DISK_IMAGE),format=raw,if=ide,cache=writethrough
MODEL_DIR ?= models
GPT2_MODEL ?= $(MODEL_DIR)/gpt2_124M.bin
GPT2_GGUF_MODEL ?= $(MODEL_DIR)/gpt2.gguf
# GPT-2 124M charge ses 475 Mio de poids depuis l'initrd; 1 Gio est le minimum valide.
GPT2_RAM ?= 1024M

# Variables pour la création de l'initrd
USER_SHELL := userspace/shell
INITRD_DIR := initrd_content
BIN_DEST_DIR := $(INITRD_DIR)/bin

# Liste des fichiers objets - MISE À JOUR avec tous les nouveaux fichiers
OBJECTS = build/boot.o build/idt_loader.o build/isr_stubs.o build/paging.o build/context_switch.o build/userspace_switch.o \
          build/string.o build/pmm.o build/heap.o build/gdt_asm.o build/gdt.o build/idt.o build/vmm.o build/task.o \
          build/syscall.o build/elf.o build/initrd.o build/overlay.o build/ata.o build/rtc.o build/fat16.o build/fat32.o build/gpt2_model.o build/gpt2_gguf.o build/gpt2_gguf_loader.o build/gpt2_quant.o build/gpt2_gguf_infer.o build/gpt2_tokenizer.o build/gpt2_sample.o build/gpt2_infer.o build/interrupts.o \
          build/keyboard.o build/timer.o build/ipc.o build/service_registry.o build/multiboot.o build/kernel.o build/vga_console.o build/kbd_buffer.o build/net_ethernet_arp.o build/net_nic.o build/pci.o build/ne2k.o build/net_dhcp.o build/net_ipv4_udp.o build/net_dns.o build/net_tcp.o build/net_socket.o build/net_llm_socket.o build/sha256.o build/aes_gcm.o build/x509_der.o build/bigint.o build/ecdsa_p256.o build/x25519.o build/rsa_verify.o build/net_tls_record.o build/net_http_tls.o

# L'ABI partagée influence notamment la taille de task_t et des messages IPC.
# Une évolution de structure doit donc reconstruire toute l'image, pas seulement ipc.o.
$(OBJECTS): include/os_syscalls.h

# Cible par défaut : construire le système complet (noyau + initrd + disque overlay)
all: $(OS_IMAGE) pack-initrd disk
	@echo "=== MOHHDY v7 - Système avec GPT-2 local construit ==="
	@echo "Noyau: $(OS_IMAGE) ($(shell ls -lh $(OS_IMAGE) | awk '{print $$5}'))"
	@echo "Initrd: $(INITRD_IMAGE) ($(shell ls -lh $(INITRD_IMAGE) | awk '{print $$5}'))"
	@echo "Système prêt pour exécution avec: make run"

# Paquets hôte (Debian/Ubuntu) - même ensemble que .github/workflows/ci.yml
.PHONY: deps check-build-deps
deps:
	@bash scripts/bootstrap-dev.sh

# nasm, gcc -m32 (gcc-multilib + libc6-dev-i386), qemu-system-i386
check-build-deps:
	@command -v nasm >/dev/null 2>&1 || { \
		echo "ERROR: 'nasm' introuvable. Installez les dépendances : make deps"; \
		echo "       Debian/Ubuntu: sudo apt-get install -y nasm"; \
		exit 1; \
	}
	@mkdir -p build
	@printf 'int main(void){return 0;}\n' | $(CC) -m32 -x c - -o build/.m32-check - >/dev/null 2>&1 || { \
		echo "ERROR: 'gcc -m32' indisponible (paquets gcc-multilib et libc6-dev-i386)."; \
		echo "       Debian/Ubuntu: make deps"; \
		exit 1; \
	}
	@rm -f build/.m32-check
	@command -v qemu-system-i386 >/dev/null 2>&1 || { \
		echo "ERROR: 'qemu-system-i386' introuvable (paquet qemu-system-x86)."; \
		echo "       Debian/Ubuntu: make deps"; \
		exit 1; \
	}

# Règle pour lier les fichiers objets et créer l'image finale
$(OS_IMAGE): $(OBJECTS)
	@mkdir -p $(dir $@)
	$(LD) -m elf_i386 -T linker.ld -o $@ --start-group $(OBJECTS) --end-group

# Cible pour compiler seulement le noyau (sans initrd)
kernel-only: $(OS_IMAGE)
	@echo "=== Noyau MOHHDY Compilé ==="
	@echo "Fichier: $(OS_IMAGE) ($(shell ls -lh $(OS_IMAGE) | awk '{print $$5}'))"

# Règles de compilation pour les fichiers .c du kernel principal
build/kernel.o: kernel/kernel.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

build/vga_console.o: kernel/vga_console.c kernel/vga_console.h
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

build/gdt.o: kernel/gdt.c kernel/gdt.h
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

build/idt.o: kernel/idt.c kernel/idt.h
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@
	
build/interrupts.o: kernel/interrupts.c kernel/interrupts.h
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

build/keyboard.o: kernel/keyboard.c kernel/keyboard.h
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

build/kbd_buffer.o: kernel/input/kbd_buffer.c kernel/input/kbd_buffer.h
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@


build/timer.o: kernel/timer.c kernel/timer.h
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

build/ipc.o: kernel/ipc.c kernel/ipc.h include/os_syscalls.h
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

build/service_registry.o: kernel/service_registry.c kernel/service_registry.h include/os_syscalls.h
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

build/multiboot.o: kernel/multiboot.c kernel/multiboot.h
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

build/elf.o: kernel/elf.c kernel/elf.h
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

# Règles de compilation pour les fichiers de gestion mémoire
build/pmm.o: kernel/mem/pmm.c kernel/mem/pmm.h
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

build/vmm.o: kernel/mem/vmm.c kernel/mem/vmm.h
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

build/heap.o: kernel/mem/heap.c kernel/mem/heap.h
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

build/string.o: kernel/mem/string.c kernel/mem/string.h
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

# Règles de compilation pour le système de tâches (version complète)
build/task.o: kernel/task/task.c kernel/task/task.h
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

# Règles de compilation pour les appels système
build/syscall.o: kernel/syscall/syscall.c kernel/syscall/syscall.h
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

build/net_ethernet_arp.o: kernel/net_ethernet_arp.c kernel/net_ethernet_arp.h
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

build/net_nic.o: kernel/net_nic.c kernel/net_nic.h
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

build/pci.o: kernel/pci.c kernel/pci.h
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

build/ne2k.o: kernel/ne2k.c kernel/ne2k.h
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

build/net_dhcp.o: kernel/net_dhcp.c kernel/net_dhcp.h
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

build/net_ipv4_udp.o: kernel/net_ipv4_udp.c kernel/net_ipv4_udp.h
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

build/net_dns.o: kernel/net_dns.c kernel/net_dns.h
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

build/net_tcp.o: kernel/net_tcp.c kernel/net_tcp.h
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

build/net_socket.o: kernel/net_socket.c kernel/net_socket.h kernel/net_tcp.h
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

build/net_llm_socket.o: kernel/net_llm_socket.c kernel/net_llm_socket.h kernel/net_socket.h kernel/net_http_tls.h
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

build/sha256.o: kernel/sha256.c kernel/sha256.h
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

build/aes_gcm.o: kernel/aes_gcm.c kernel/aes_gcm.h
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

build/x509_der.o: kernel/x509_der.c kernel/x509_der.h
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

build/bigint.o: kernel/bigint.c kernel/bigint.h
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

build/ecdsa_p256.o: kernel/ecdsa_p256.c kernel/ecdsa_p256.h kernel/bigint.h
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

build/x25519.o: kernel/x25519.c kernel/x25519.h kernel/bigint.h
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

build/rsa_verify.o: kernel/rsa_verify.c kernel/rsa_verify.h kernel/bigint.h
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

build/net_tls_record.o: kernel/net_tls_record.c kernel/net_tls_record.h kernel/aes_gcm.h kernel/x509_der.h kernel/rsa_verify.h kernel/x25519.h
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

build/net_http_tls.o: kernel/net_http_tls.c kernel/net_http_tls.h kernel/net_tcp.h kernel/net_tls_record.h
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@


# Règles de compilation pour le système de fichiers
build/initrd.o: fs/initrd.c fs/initrd.h
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

build/overlay.o: fs/overlay.c fs/overlay.h fs/initrd.h kernel/ata.h
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

build/ata.o: kernel/ata.c kernel/ata.h
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

build/rtc.o: kernel/rtc.c kernel/rtc.h
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

build/fat16.o: kernel/fs/fat16.c kernel/fs/fat16.h include/os_syscalls.h
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

build/fat32.o: kernel/fs/fat32.c kernel/fs/fat32.h kernel/fs/fat16.h include/os_syscalls.h
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

# Chargeur GPT-2 local : valide un checkpoint dans l'initrd sans dependance hote.
build/gpt2_model.o: kernel/llm/gpt2_model.c kernel/llm/gpt2_model.h fs/initrd.h
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

# Sonde GGUF v3 et index borné ; le loader lit un fichier FAT16 dans un buffer caller-owned.
build/gpt2_gguf.o: kernel/llm/gpt2_gguf.c kernel/llm/gpt2_gguf.h kernel/llm/gpt2_quant.h
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

build/gpt2_gguf_loader.o: kernel/llm/gpt2_gguf_loader.c kernel/llm/gpt2_gguf_loader.h kernel/llm/gpt2_gguf.h kernel/fs/fat16.h
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

# Kernels de produits Q8_0/Q3_K/Q4_K/Q6_K x FP32 pour la quantification GGUF.
build/gpt2_quant.o: kernel/llm/gpt2_quant.c kernel/llm/gpt2_quant.h
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

# Tokenizer GPT-2 local en lecture seule.
build/gpt2_tokenizer.o: kernel/llm/gpt2_tokenizer.c kernel/llm/gpt2_tokenizer.h fs/initrd.h
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

# Echantillonnage top-k GPT-2 (sans dependance heap / checkpoint).
build/gpt2_sample.o: kernel/llm/gpt2_sample.c kernel/llm/gpt2_sample.h
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

# Noyau d'inference GPT-2 CPU freestanding.
build/gpt2_gguf_infer.o: kernel/llm/gpt2_gguf_infer.c kernel/llm/gpt2_gguf_infer.h kernel/llm/gpt2_gguf_loader.h kernel/llm/gpt2_sample.h kernel/fs/fat16.h
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

build/gpt2_infer.o: kernel/llm/gpt2_infer.c kernel/llm/gpt2_infer.h kernel/llm/gpt2_sample.h kernel/llm/gpt2_model.h kernel/mem/heap.h
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

# Règles pour compiler le code assembleur
build/boot.o: boot/boot.s
	@mkdir -p $(dir $@)
	$(AS) $(ASFLAGS) $< -o $@

build/gdt_asm.o: boot/gdt.s
	@mkdir -p $(dir $@)
	$(AS) $(ASFLAGS) $< -o $@

build/idt_loader.o: boot/idt_loader.s
	@mkdir -p $(dir $@)
	$(AS) $(ASFLAGS) $< -o $@

build/isr_stubs.o: boot/isr_stubs.s
	@mkdir -p $(dir $@)
	$(AS) $(ASFLAGS) $< -o $@

build/paging.o: boot/paging.s
	@mkdir -p $(dir $@)
	$(AS) $(ASFLAGS) $< -o $@

build/context_switch.o: boot/context_switch_new.s
	@mkdir -p $(dir $@)
	$(AS) $(ASFLAGS) $< -o $@

build/userspace_switch.o: boot/userspace_switch.s
	@mkdir -p $(dir $@)
	$(AS) $(ASFLAGS) $< -o $@

# Toujours recompiler le userspace : les ELF commités sont périmés (même mtime au checkout CI).
.PHONY: userspace-all
userspace-all:
	$(MAKE) -C userspace all

# Règle pour empaqueter l'initrd automatiquement
pack-initrd: userspace-all
	@echo "[mkinitrd] Création de l'initrd MOHHDY v7..."
	@mkdir -p $(BIN_DEST_DIR) $(INITRD_DIR)/models
	@echo "Ceci est un fichier de test depuis l'initrd !" > $(INITRD_DIR)/test.txt
	@echo "Un autre fichier de demonstration." > $(INITRD_DIR)/hello.txt
	@echo "Configuration du systeme MOHHDY v7" > $(INITRD_DIR)/config.cfg
	@echo "#!/bin/sh" > $(INITRD_DIR)/startup.sh
	@echo "echo 'Script de demarrage MOHHDY v7'" >> $(INITRD_DIR)/startup.sh
	@echo "Donnees de demonstration pour l'intelligence artificielle locale" > $(INITRD_DIR)/ai_data.txt
	@echo "Base de connaissances statique - pas de base vectorielle" > $(INITRD_DIR)/ai_knowledge.txt
	@echo "# MOHHDY bare-metal LLM manifest" > $(INITRD_DIR)/models/models.manifest
	@echo "format=llmc_v3" >> $(INITRD_DIR)/models/models.manifest
	@echo "default=gpt2_124M.bin" >> $(INITRD_DIR)/models/models.manifest
	@echo "gpt2_124M.bin|gpt2|124M|FP32|local" >> $(INITRD_DIR)/models/models.manifest
	@echo "gpt2.gguf|gpt2|optional|GGUF-v3|kquant-kernels" >> $(INITRD_DIR)/models/models.manifest
	@echo "# Provide gpt2_124M.bin and gpt2_tokenizer.bin in models/ before build." > $(INITRD_DIR)/models/README.txt
	@echo "# models/gpt2.gguf is optional: v3 structure and Q3_K/Q4_K/Q6_K kernel layouts are validated; full GGUF GPT-2 loading remains pending." >> $(INITRD_DIR)/models/README.txt
	@if [ -f "$(GPT2_MODEL)" ] && [ -f "$(MODEL_DIR)/gpt2_tokenizer.bin" ]; then \
		echo "[mkinitrd] Inclusion du checkpoint et du tokenizer GPT-2 locaux..."; \
		cp -f "$(GPT2_MODEL)" "$(INITRD_DIR)/models/gpt2_124M.bin"; \
		cp -f "$(MODEL_DIR)/gpt2_tokenizer.bin" "$(INITRD_DIR)/models/gpt2_tokenizer.bin"; \
	else \
		echo "[mkinitrd] Checkpoint ou tokenizer GPT-2 local absent."; \
	fi
	@rm -f "$(INITRD_DIR)/models/gpt2.gguf"
	@if [ -f "$(GPT2_GGUF_MODEL)" ]; then \
		echo "[mkinitrd] Inclusion du profil GGUF optionnel..."; \
		cp -f "$(GPT2_GGUF_MODEL)" "$(INITRD_DIR)/models/gpt2.gguf"; \
	fi
	@cp -f $(USER_SHELL) $(BIN_DEST_DIR)/shell
	@cp -f userspace/fake_ai $(BIN_DEST_DIR)/fake_ai
	@cp -f userspace/ai_assistant $(BIN_DEST_DIR)/ai_assistant
	@cp -f userspace/test_program $(BIN_DEST_DIR)/user_program
	@cp -f userspace/idle $(BIN_DEST_DIR)/idle
	@cp -f userspace/spin $(BIN_DEST_DIR)/spin
	@cp -f userspace/ipcserver $(BIN_DEST_DIR)/ipcserver
	@cp -f userspace/vfsserver $(BIN_DEST_DIR)/vfsserver
	@cp -f userspace/vfsvirtual $(BIN_DEST_DIR)/vfsvirtual
	@cp -f userspace/vfsflight $(BIN_DEST_DIR)/vfsflight
	@cp -f userspace/vfsaliasflight $(BIN_DEST_DIR)/vfsaliasflight
	@cp -f userspace/serviceclaim $(BIN_DEST_DIR)/serviceclaim
	@cp -f userspace/vfsclaim $(BIN_DEST_DIR)/vfsclaim
	@cp -f userspace/vfscapclaim $(BIN_DEST_DIR)/vfscapclaim
	@cp -f userspace/vfsreleaseclaim $(BIN_DEST_DIR)/vfsreleaseclaim
	@cp -f userspace/vfsreadclaim $(BIN_DEST_DIR)/vfsreadclaim
	@cp -f userspace/vfsmutateclaim $(BIN_DEST_DIR)/vfsmutateclaim
	@cp -f userspace/waitchild $(BIN_DEST_DIR)/waitchild
	@cp -f userspace/ok $(BIN_DEST_DIR)/ok
	@tar -C $(INITRD_DIR) -cf $(INITRD_IMAGE) .
	@echo "[mkinitrd] Packed executables into $(INITRD_IMAGE)"

# ===== ISO (GRUB) Build =====
.PHONY: iso check-iso-deps run-iso iso-clean

check-iso-deps:
	@command -v grub-mkrescue >/dev/null 2>&1 || { \
		echo "ERROR: 'grub-mkrescue' introuvable. Installez grub-pc-bin et xorriso."; \
		echo "       Debian/Ubuntu: sudo apt-get install -y grub-pc-bin xorriso"; \
		exit 1; \
	}
	@command -v xorriso >/dev/null 2>&1 || { \
		echo "ERROR: 'xorriso' introuvable. Installez-le: sudo apt-get install -y xorriso"; \
		exit 1; \
	}

# Construire une image ISO bootable (Multiboot + GRUB2)
iso: check-iso-deps $(OS_IMAGE) pack-initrd
	@echo "=== Construction ISO bootable (GRUB2) ==="
	@rm -rf build/isodir
	@mkdir -p build/isodir/boot/grub
	@cp -f $(OS_IMAGE) build/isodir/boot/mohhdy.bin
	@cp -f $(INITRD_IMAGE) build/isodir/boot/$(INITRD_IMAGE)
	@echo "set timeout=0" > build/isodir/boot/grub/grub.cfg
	@echo "set default=0" >> build/isodir/boot/grub/grub.cfg
	@echo "menuentry 'MOHHDY' {" >> build/isodir/boot/grub/grub.cfg
	@echo "  multiboot /boot/mohhdy.bin" >> build/isodir/boot/grub/grub.cfg
	@echo "  module    /boot/$(INITRD_IMAGE)" >> build/isodir/boot/grub/grub.cfg
	@echo "  boot" >> build/isodir/boot/grub/grub.cfg
	@echo "}" >> build/isodir/boot/grub/grub.cfg
	@grub-mkrescue -o $(ISO_IMAGE) build/isodir >/dev/null 2>&1 || \
		(grub-mkrescue -o $(ISO_IMAGE) build/isodir)
	@echo "ISO générée: $(ISO_IMAGE)"

# Lancer l'ISO avec QEMU (boot CD)
run-iso: iso disk
	qemu-system-i386 -cdrom $(ISO_IMAGE) -boot d -m $(GPT2_RAM) -cpu pentium3 -no-reboot -no-shutdown $(QEMU_DISK_OPTS)

iso-clean:
	@rm -rf build/isodir $(ISO_IMAGE)

# Compile tous les programmes utilisateur
user-program userspace/shell userspace/fake_ai userspace/test_program userspace/ai_assistant userspace/idle userspace/spin userspace/ipcserver userspace/vfsserver userspace/vfsvirtual userspace/vfsflight userspace/serviceclaim userspace/vfsclaim userspace/vfscapclaim userspace/vfsreleaseclaim userspace/vfsreadclaim userspace/vfsmutateclaim userspace/waitchild userspace/ok: userspace-all

# Cible pour exécuter l'OS dans QEMU avec initrd (mode console corrigé)
run: $(OS_IMAGE) pack-initrd disk
	qemu-system-i386 -kernel $(OS_IMAGE) -initrd $(INITRD_IMAGE) \
		-display curses \
		-m $(GPT2_RAM) -cpu pentium3 \
		-no-reboot -no-shutdown $(QEMU_DISK_OPTS)

# Cible pour exécuter l'OS dans QEMU avec interface graphique améliorée
run-gui: $(OS_IMAGE) pack-initrd disk
	qemu-system-i386 -kernel $(OS_IMAGE) -initrd $(INITRD_IMAGE) \
		-m $(GPT2_RAM) -cpu pentium3 -vga std \
		-display gtk \
		-no-reboot -no-shutdown $(QEMU_DISK_OPTS)

# Alternative nographic (si curses ne fonctionne pas)
run-nographic: $(OS_IMAGE) pack-initrd disk
	@echo "=== Mode NOGRAPHIC - Clavier peut être limité ==="
	@echo "Utilisez 'make run' pour le mode console optimal"
	qemu-system-i386 -kernel $(OS_IMAGE) -initrd $(INITRD_IMAGE) \
		-nographic \
		-chardev stdio,id=serial0 \
		-m $(GPT2_RAM) -cpu pentium3 \
		-no-reboot -no-shutdown $(QEMU_DISK_OPTS)

# Cible pour tester le clavier avec GUI et capture des logs série
run-kbd-gui-test: $(OS_IMAGE) pack-initrd
	@echo "=== Test Clavier avec Interface Graphique ==="
	@echo "1. QEMU va s'ouvrir dans une fenêtre graphique"
	@echo "2. Les logs série seront sauvés dans keyboard_test_serial.log"
	@echo "3. Testez le clavier dans la fenêtre QEMU"
	@echo "4. Fermez QEMU quand terminé, puis vérifiez les logs"
	@echo "=============================================="
	qemu-system-i386 -kernel $(OS_IMAGE) -initrd $(INITRD_IMAGE) \
		-m 256M -vga std \
		-machine type=pc,accel=tcg \
		-device i8042 \
		-serial file:keyboard_test_serial.log \
		-rtc base=utc -no-reboot &
	@echo "QEMU lancé en arrière-plan. Attendez qu'il s'ouvre..."
	@sleep 3
	@echo "Maintenant:"
	@echo "1. Cliquez dans la fenêtre QEMU pour la focus"
	@echo "2. Tapez quelques caractères"
	@echo "3. Appuyez sur Ctrl+Alt+G pour libérer la souris"
	@echo "4. Fermez QEMU avec Alt+F4 ou le bouton X"
	@echo "5. Vérifiez les résultats avec: tail -50 keyboard_test_serial.log"
		-monitor stdio \
		-machine pc \
		-no-reboot -no-shutdown

# Cible pour test interactif du clavier
run-interactive: $(OS_IMAGE) pack-initrd
	@echo "=== Test Interactif du Clavier MOHHDY ==="
	@echo "Instructions:"
	@echo "1. Le système va démarrer avec interface graphique"
	@echo "2. Tapez des caractères pour tester le clavier"
	@echo "3. Utilisez Ctrl+Alt+2 pour accéder au moniteur QEMU"
	@echo "4. Dans le moniteur, tapez 'quit' pour quitter"
	@echo ""
	qemu-system-i386 -kernel $(OS_IMAGE) -initrd $(INITRD_IMAGE) \
		-serial stdio \
		-monitor telnet:localhost:4444,server,nowait \
		-machine pc

# Cible pour tester la compilation sans exécution
test-build: $(OS_IMAGE)
	@echo "Compilation réussie ! Image générée: $(OS_IMAGE)"
	@ls -la $(OS_IMAGE)

# Cible pour afficher les informations sur l'initrd
info-initrd: pack-initrd
	@echo "Contenu de l'initrd:"
	@tar -tvf $(INITRD_IMAGE)

# Cible pour afficher les informations sur le programme utilisateur
info-user:
	@$(MAKE) -C userspace info

# Cible pour nettoyer le projet
clean:
	rm -rf build
	rm -f $(INITRD_IMAGE)
	rm -rf initrd_content
	rm -f output.log
	@$(MAKE) -C userspace clean

# Cible pour nettoyer complètement (y compris les fichiers de sauvegarde)
distclean: clean
	rm -f *~ */*~ */*/*~
	rm -f *.bak */*.bak */*/*.bak

# Cibles pour les tests de non-régression
test-setup:
	@echo "=== Configuration des tests de non-régression ==="
	@$(MAKE) -C tests setup
	@echo "Tests configurés avec succès"

test-quick:
	@echo "=== Tests rapides (critiques seulement) ==="
	@$(MAKE) -C tests test-quick

test-kernel:
	@echo "=== Tests des modules kernel ==="
	@$(MAKE) -C tests test-kernel

test-userspace:
	@echo "=== Tests des modules userspace ==="
	@$(MAKE) -C tests test-userspace

test-all:
	@echo "=== Suite complète de tests de non-régression ==="
	@$(MAKE) -C tests test

test-performance:
	@echo "=== Tests de performance et benchmarks ==="
	@$(MAKE) -C tests benchmark

test-valgrind:
	@echo "=== Tests avec détection de fuites mémoire ==="
	@command -v valgrind >/dev/null 2>&1 || { \
		echo "INFO: 'valgrind' non installé. Les tests mémoire sont ignorés."; \
		echo "      Pour l'installer: sudo apt-get install -y valgrind"; \
		exit 0; \
	}
	@$(MAKE) -C tests test-valgrind || true

test-clean:
	@echo "=== Nettoyage des fichiers de test ==="
	@$(MAKE) -C tests clean

# Cible pour les développeurs - tests avant commit
pre-commit-tests: check-build-deps $(OS_IMAGE) pack-initrd test-quick
	@echo "=== Vérification pré-commit terminée ==="

# Cible pour l'intégration continue
ci-tests: $(OS_IMAGE) pack-initrd
	@echo "=== Tests d'intégration continue ==="
	@$(MAKE) -C tests ci-test

# Image disque IDE brute (snapshot overlay). Recréee seulement si absente.
$(DISK_IMAGE):
	@mkdir -p $(dir $@)
	dd if=/dev/zero of=$@ bs=512 count=$(DISK_SECTORS) status=none
	python3 tests/scripts/make_fat16_image.py --image $@

.PHONY: disk fat16-fixture gguf-disk
disk: $(DISK_IMAGE)

fat16-fixture: $(DISK_IMAGE)
	@python3 tests/scripts/make_fat16_image.py --image $(DISK_IMAGE)

# Boot QEMU headless, tape ls/cat/ps/uptime (sendkey), exige l'initrd et le noyau.
qemu-smoke: $(OS_IMAGE) pack-initrd disk
	@chmod +x tests/scripts/ci_qemu_smoke.sh
	@tests/scripts/ci_qemu_smoke.sh

# Contrats d’intégration QEMU versionnés : boot, shell/overlay, préemption IRQ0
# et absence réseau OpenAI explicitement vérifiable.
.PHONY: gui-captures gui-record
gui-captures: $(OS_IMAGE) pack-initrd disk
	@python3 tests/scripts/gui_screendump.py

gui-record: $(OS_IMAGE) pack-initrd disk
	@python3 tests/scripts/gui_record_demo.py

.PHONY: integration-qemu qemu-integration-plan qemu-irq0-preemption qemu-ai-provider qemu-ne2k-status qemu-ne2k-tls-http qemu-ne2k-tls-sse qemu-ne2k-tls-next qemu-ne2k-tls-multipair qemu-ipc-foundation qemu-vfs-service qemu-service-grant
qemu-irq0-preemption: $(OS_IMAGE) pack-initrd disk
	@python3 tests/integration/test_qemu_irq0_preemption.py

qemu-ai-provider: $(OS_IMAGE) pack-initrd disk
	@python3 tests/scripts/test_ai_provider_commands.py
qemu-ne2k-status: $(OS_IMAGE) pack-initrd disk
	@python3 tests/scripts/test_qemu_ne2k_status.py

qemu-ne2k-tls-multipair: $(OS_IMAGE) pack-initrd
	@python3 tests/scripts/test_qemu_ne2k_tls_multipair.py
qemu-ipc-foundation: $(OS_IMAGE) pack-initrd disk
	@python3 tests/integration/test_qemu_ipc_foundation.py

qemu-vfs-service: $(OS_IMAGE) pack-initrd disk
	@python3 tests/integration/test_qemu_vfs_service.py

qemu-service-grant: $(OS_IMAGE) pack-initrd disk
	@python3 tests/integration/test_qemu_service_grant.py

# Les sept contrats restent inchangés ; l’ordonnanceur les exécute dans un pool borne
# a deux QEMU par defaut. QEMU_INTEGRATION_JOBS=1 conserve le mode strictement sequentiel.
integration-qemu: $(OS_IMAGE) pack-initrd disk
	@python3 tests/integration/run_qemu_contracts.py

# Affiche la liste exacte des contrats et leur ordre sans demarrer QEMU.
qemu-integration-plan: $(OS_IMAGE) pack-initrd disk
	@QEMU_INTEGRATION_DRY_RUN=1 python3 tests/integration/run_qemu_contracts.py

# Tests d'intégration réels GPT-2 : les poids locaux sous models/ sont requis.
.PHONY: gpt2-recovery gpt2-benchmark gpt2-tests
gpt2-recovery: $(OS_IMAGE) pack-initrd
	@python3 tests/scripts/test_gpt2_shell_recovery.py

gpt2-benchmark: $(OS_IMAGE) pack-initrd
	@python3 tests/scripts/benchmark_gpt2_kv_latency.py

gpt2-tests: gpt2-recovery gpt2-benchmark
	@echo "=== Vérifications GPT-2 QEMU terminées ==="

# Gate CI local : image + tests unitaires + smoke QEMU
ci: all test-all qemu-smoke qemu-ne2k-tls-multipair
	@echo "=== CI locale OK (build + tests + smokes QEMU locaux) ==="

# Cible pour afficher l'aide
help:
	@echo "=== MOHHDY v7 - Cibles de compilation ==="
	@echo ""
	@echo "Cibles principales:"
	@echo "  deps         - Installe les paquets hôte (scripts/bootstrap-dev.sh)"
	@echo "  all          - Compile le système complet (noyau + initrd + disque overlay)"
	@echo "  kernel-only  - Compile seulement le noyau"
	@echo "  run          - Compile et exécute avec QEMU (mode texte)"
	@echo "  run-gui      - Compile et exécute avec QEMU (mode graphique)"
	@echo "  iso          - Image GRUB Multiboot (grub-pc-bin + xorriso)"
	@echo ""
	@echo "Cibles de développement:"
	@echo "  test-build   - Compile sans exécuter"
	@echo "  user-program - Compile seulement les programmes utilisateur"
	@echo "  info-initrd  - Affiche le contenu de l'initrd"
	@echo "  info-user    - Affiche les infos des programmes utilisateur"
	@echo ""
	@echo "Cibles de tests (NOUVEAU):"
	@echo "  test-setup      - Configure l'environnement de test"
	@echo "  test-quick      - Tests rapides (< 1 min, pour développement)"
	@echo "  test-kernel     - Tests des modules kernel uniquement"
	@echo "  test-userspace  - Tests des modules userspace uniquement"  
	@echo "  test-all        - Suite complète de tests (< 5 min)"
	@echo "  qemu-smoke      - Boots QEMU : overlay, extras, persist, spawn, exec"
	@echo "  integration-qemu - Sept contrats QEMU, deux workers bornes (QEMU_INTEGRATION_JOBS=1 pour le mode sequentiel)"
	@echo "  qemu-integration-plan - Affiche les sept contrats sans demarrer QEMU"
	@echo "  qemu-irq0-preemption - Prouve la reprise du shell après spawn spin"
	@echo "  qemu-ai-provider - Vérifie le stub OpenAI/réseau explicite"
	@echo "  qemu-ne2k-acquire - Exerce DHCP, DNS, ARP, SYN/SYN-ACK et ClientHello via NIC QEMU"
	@echo "  qemu-ne2k-tls-http - TLS authentifie local puis HTTP 200 JSON sur pair QEMU"
	@echo "  qemu-ne2k-tls-sse - TLS authentifie local puis flux SSE chunked sur pair QEMU"
	@echo "  qemu-ne2k-tls-close - close_notify TLS distant et réponse authentifiée sur pair QEMU"
	@echo "  qemu-ne2k-tls-next - Second tour LLM sur la meme session TLS via ai-next"
	@echo "  qemu-ne2k-tls-multipair - Deux liens NE2000/TLS locaux séquentiels, MAC et journaux séparés"
	@echo "  qemu-ipc-foundation - Vérifie l’IPC entre tâches Ring 3"
	@echo "  qemu-vfs-service - Vérifie une lecture via le médiateur VFS Ring 3"
	@echo "  gguf-benchmark  - Mesure répétée du premier token et de ai-continue GGUF sous QEMU"
	@echo "  gguf-benchmark-check - Vérifie le protocole de synthèse sans démarrer QEMU"
	@echo "  gui-captures    - Screendumps QEMU GTK (DISPLAY=:1, artefacts PNG)"
	@echo "  gui-record      - Video courte QEMU GTK (ffmpeg x11grab)"
	@echo "  disk            - Cree build/overlay.img (IDE, 32 Kio) si absent"
	@echo "  gpt2-recovery   - Modèle requis : réponse GPT-2 puis reprise shell (rc)"
	@echo "  gpt2-benchmark  - Modèle requis : mesure de latence QEMU SSE2"
	@echo "  gpt2-tests      - Modèle requis : recovery + benchmark GPT-2"
	@echo "  ci              - make all + test-all + smokes QEMU locaux (gate PR)"
	@echo "  test-performance - Benchmarks et tests de performance"
	@echo "  test-valgrind   - Tests avec détection fuites mémoire"
	@echo "  pre-commit-tests - Tests rapides avant commit"
	@echo "  ci-tests        - Tests pour intégration continue"
	@echo "  test-clean      - Nettoie les fichiers de test"
	@echo ""
	@echo "Cibles de maintenance:"
	@echo "  clean        - Nettoie les fichiers générés"
	@echo "  distclean    - Nettoie tout (y compris sauvegardes)"
	@echo "  help         - Affiche cette aide"
	@echo ""
	@echo "Usage recommandé pour développeurs:"
	@echo "  make deps                 # Paquets Debian/Ubuntu (CI)"
	@echo "  make check-build-deps     # nasm, gcc -m32, qemu-system-i386"
	@echo "  make clean && make all    # Compilation complète"
	@echo "  make ci                   # Gate PR : all + test-all + qemu-smoke"
	@echo "  make run                  # Session QEMU curses"
	@echo ""
	@echo "Tests de non-régression:"
	@echo "  make test-setup           # Configuration initiale (une fois)"
	@echo "  make test-quick           # Tests pendant développement"
	@echo "  make test-all             # 497 tests de non-régression avant push"

.PHONY: all kernel-only run run-gui test-build info-initrd info-user user-program userspace-all clean distclean help pack-initrd test-setup test-quick test-kernel test-userspace test-all test-performance test-valgrind test-clean pre-commit-tests ci-tests qemu-smoke qemu-ne2k-acquire qemu-ne2k-tls-http qemu-ne2k-tls-sse qemu-ne2k-tls-close qemu-ne2k-tls-next qemu-ne2k-tls-multipair gpt2-recovery gpt2-benchmark gpt2-tests qemu-gguf-smoke gguf-benchmark gguf-benchmark-check ci deps disk gui-captures gui-record


gguf-disk:
	@python3 scripts/make_gguf_fat16_image.py --model $(GPT2_GGUF_DEPLOY_MODEL) --image $(GGUF_DISK_IMAGE)

fat32-secondary-disk:
	@python3 scripts/make_fat32_secondary_image.py --image $(FAT32_SECONDARY_IMAGE)

.PHONY: qemu-gguf-smoke gguf-benchmark
qemu-ne2k-acquire: $(OS_IMAGE) pack-initrd
	@python3 tests/scripts/test_qemu_ne2k_llm_acquire.py

qemu-ne2k-tls-http: $(OS_IMAGE) pack-initrd
	@python3 tests/scripts/qemu_ne2k_tls12_server.py
	@python3 tests/scripts/test_qemu_ne2k_tls_http.py

qemu-ne2k-tls-sse: $(OS_IMAGE) pack-initrd
	@python3 tests/scripts/qemu_ne2k_tls12_server.py
	@python3 tests/scripts/test_qemu_ne2k_tls_sse.py

qemu-ne2k-tls-close: $(OS_IMAGE) pack-initrd
	@python3 tests/scripts/qemu_ne2k_tls12_server.py
	@python3 tests/scripts/test_qemu_ne2k_tls_sse.py --peer-close

qemu-ne2k-tls-next: $(OS_IMAGE) pack-initrd
	@python3 tests/scripts/qemu_ne2k_tls12_server.py
	@python3 tests/scripts/test_qemu_ne2k_tls_next.py

qemu-gguf-smoke: $(OS_IMAGE) pack-initrd gguf-disk
	@OVERLAY_DISK="$(abspath $(GGUF_DISK_IMAGE))" python3 tests/scripts/ci_qemu_gguf_local_smoke.py

gguf-benchmark: $(OS_IMAGE) pack-initrd gguf-disk
	@OVERLAY_DISK="$(abspath $(GGUF_DISK_IMAGE))" python3 tests/scripts/benchmark_qemu_gguf_latency.py

gguf-benchmark-check:
	@python3 tests/scripts/test_benchmark_qemu_gguf_latency.py
