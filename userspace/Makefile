# Options de compilation pour l'espace utilisateur
CC = gcc
LD = ld
AS = as

CFLAGS = -m32 -nostdlib -nostdinc -fno-builtin -fno-stack-protector -I../kernel -I../include
LDFLAGS = -nostdlib -Ttext=0x40000000
ASFLAGS = --32

# Programmes à compiler
PROGRAMS = shell fake_ai test_program ai_assistant idle spin ipcserver vfsserver vfsvirtual vfsflight vfsaliasflight serviceclaim vfsclaim vfscapclaim vfsreleaseclaim vfsreadclaim vfsmutateclaim waitchild ok
USER_HEADERS = ../include/os_syscalls.h ../include/os_vfs_service.h ../include/os_ipc_deferred.h

all: $(PROGRAMS)

shell: shell.o ramfs.o procsim.o start.o
	@echo "Linking shell..."
	$(LD) -m elf_i386 $(LDFLAGS) -o $@ $^

fake_ai: fake_ai.o start.o
	@echo "Linking fake_ai..."
	$(LD) -m elf_i386 $(LDFLAGS) -o $@ $^

ai_assistant: ai_assistant.o start.o
	@echo "Linking ai_assistant..."
	$(LD) -m elf_i386 $(LDFLAGS) -o $@ $^

test_program: test_program.o start.o
	@echo "Linking test_program..."
	$(LD) -m elf_i386 $(LDFLAGS) -o $@ $^

idle: idle.o start.o
	@echo "Linking idle..."
	$(LD) -m elf_i386 $(LDFLAGS) -o $@ $^

spin: spin.o start.o
	@echo "Linking spin..."
	$(LD) -m elf_i386 $(LDFLAGS) -o $@ $^

ipcserver: ipc_server.o start.o
	@echo "Linking ipcserver..."
	$(LD) -m elf_i386 $(LDFLAGS) -o $@ $^

vfsserver: vfs_server.o start.o
	@echo "Linking vfsserver..."
	$(LD) -m elf_i386 $(LDFLAGS) -o $@ $^

vfsvirtual: vfs_virtual_worker.o start.o
	@echo "Linking vfsvirtual..."
	$(LD) -m elf_i386 $(LDFLAGS) -o $@ $^

vfsflight: vfs_flight_client.o start.o
	@echo "Linking vfsflight..."
	$(LD) -m elf_i386 $(LDFLAGS) -o $@ $^

vfsaliasflight: vfs_alias_flight_client.o start.o
	@echo "Linking vfsaliasflight..."
	$(LD) -m elf_i386 $(LDFLAGS) -o $@ $^

serviceclaim: service_claim.o start.o
	@echo "Linking serviceclaim..."
	$(LD) -m elf_i386 $(LDFLAGS) -o $@ $^

vfsclaim: vfs_claim.o start.o
	@echo "Linking vfsclaim..."
	$(LD) -m elf_i386 $(LDFLAGS) -o $@ $^

vfscapclaim: vfs_backend_claim.o start.o
	@echo "Linking vfscapclaim..."
	$(LD) -m elf_i386 $(LDFLAGS) -o $@ $^

vfsreleaseclaim: vfs_backend_release_claim.o start.o
	@echo "Linking vfsreleaseclaim..."
	$(LD) -m elf_i386 $(LDFLAGS) -o $@ $^

vfsreadclaim: vfs_read_claim.o start.o
	@echo "Linking vfsreadclaim..."
	$(LD) -m elf_i386 $(LDFLAGS) -o $@ $^

vfsmutateclaim: vfs_mutate_claim.o start.o
	@echo "Linking vfsmutateclaim..."
	$(LD) -m elf_i386 $(LDFLAGS) -o $@ $^

waitchild: wait_child.o start.o
	@echo "Linking waitchild..."
	$(LD) -m elf_i386 $(LDFLAGS) -o $@ $^

ok: ok.o start.o
	@echo "Linking ok..."
	$(LD) -m elf_i386 $(LDFLAGS) -o $@ $^

%.o: %.c $(USER_HEADERS)
	@echo "Compiling C: $<"
	$(CC) $(CFLAGS) -c -o $@ $<

start.o: start.s
	@echo "Assembling with AS: $<"
	$(AS) $(ASFLAGS) $< -o $@

clean:
	@echo "Cleaning userspace..."
	rm -f *.o $(PROGRAMS)
