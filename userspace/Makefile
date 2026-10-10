# Options de compilation pour l'espace utilisateur
CC = gcc
LD = ld
AS = as

CFLAGS = -m32 -nostdlib -nostdinc -fno-builtin -fno-stack-protector -I../kernel -I../include
LDFLAGS = -nostdlib -Ttext=0x40000000
ASFLAGS = --32

# Programmes à compiler
PROGRAMS = shell fake_ai test_program ai_assistant idle spin ipcserver vfsserver vfsvirtual networker vfsflight vfsaliasflight serviceclaim vfsclaim vfscapclaim vfsreleaseclaim vfsreadclaim vfsmutateclaim vfshistclaim waitchild ok
USER_HEADERS = promptmessage.h ../include/os_syscalls.h ../include/os_vfs_service.h ../include/os_ipc_deferred.h mohhdy_osui_bridge.h osui_runtime.h osui_gui.h prod.h
# Tranche 4: Ring 3 ATA driver + proofs.
PROGRAMS += atadriver ataclient atarogue
# Tranche 5: net-driver gate proof client.
PROGRAMS += netclaim
# Tranche 5 slice 2: net IPC relay proof client.
PROGRAMS += netrelay
PROGRAMS += netwire
# Blocking IPC (SYS_IPC_RECV_WAIT) proof programs.
PROGRAMS += ipcwait ipcpoke
# Inventory item 4: Ring 3 GPT-2 worker + proof clients.
PROGRAMS += aiworker aiclient aistat airogue ggufclient ggufpause

all: $(PROGRAMS)

# Inventory item 4: the aiworker links its own copy of the llm.c FP32 engine
# (model probe, tokenizer, top-k sampler, forward pass, generation loop),
# compiled at CPL 3 with the kernel's float flags (-O3 -msse2 -mfpmath=sse)
# so both paths produce the same token ids.
AI_LLM_SRCS = gpt2_model gpt2_tokenizer gpt2_sample gpt2_infer gpt2_generate \
              gpt2_gguf gpt2_gguf_loader gpt2_quant gpt2_gguf_infer gpt2_gguf_session
AI_LLM_OBJS = $(addprefix ai_,$(addsuffix .o,$(AI_LLM_SRCS)))
AI_CFLAGS = $(CFLAGS) -I.. -isystem $(shell $(CC) -print-file-name=include) -D_MM_MALLOC_H_INCLUDED -DMOHHDY_RING3 -ffreestanding -fno-pie -O3 -msse2 -mfpmath=sse -mstackrealign -fomit-frame-pointer
ai_%.o: ../kernel/llm/%.c ../kernel/llm/%.h $(USER_HEADERS)
	$(CC) $(AI_CFLAGS) -c -o $@ $<
ai_worker.o: ai_worker.c ai_common.h ai_fat16_shim.h ../kernel/llm/gpt2_generate.h ../kernel/llm/gpt2_gguf_infer.h ../kernel/llm/gpt2_gguf_session.h $(USER_HEADERS)
	$(CC) $(AI_CFLAGS) -c -o $@ $<
ai_fat16_shim.o: ai_fat16_shim.c ai_fat16_shim.h $(USER_HEADERS)
	$(CC) $(AI_CFLAGS) -c -o $@ $<
aiworker: ai_worker.o ai_fat16_shim.o $(AI_LLM_OBJS) net_libc.o start.o
	$(LD) -m elf_i386 $(LDFLAGS) -o $@ $^
aiclient: ai_client.o start.o
	$(LD) -m elf_i386 $(LDFLAGS) -o $@ $^
aistat: ai_stat.o start.o
	$(LD) -m elf_i386 $(LDFLAGS) -o $@ $^
airogue: ai_rogue.o start.o
	$(LD) -m elf_i386 $(LDFLAGS) -o $@ $^
ggufclient: ai_gguf_client.o start.o
	$(LD) -m elf_i386 $(LDFLAGS) -o $@ $^
ggufpause: ai_gguf_pause.o start.o
	$(LD) -m elf_i386 $(LDFLAGS) -o $@ $^
ai_gguf_pause.o: ai_gguf_client.c
	$(CC) $(CFLAGS) -DGGUF_PAUSE -c -o $@ $<
ai_client.o ai_stat.o ai_rogue.o ai_gguf_client.o ai_gguf_pause.o: ai_common.h $(USER_HEADERS)

shell: shell.o ramfs.o procsim.o osui_runtime.o osui_gui.o promptmessage.o shell_prod.o prod.o start.o
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

ipcwait: ipc_wait_server.o start.o
	@echo "Linking ipcwait..."
	$(LD) -m elf_i386 $(LDFLAGS) -o $@ $^

ipcpoke: ipc_wait_poke.o start.o
	@echo "Linking ipcpoke..."
	$(LD) -m elf_i386 $(LDFLAGS) -o $@ $^

vfsserver: vfs_server.o start.o
	@echo "Linking vfsserver..."
	$(LD) -m elf_i386 $(LDFLAGS) -o $@ $^

p2p_%.o: ../kernel/%.c $(USER_HEADERS)
	$(CC) $(CFLAGS) -I.. -DMOHHDY_RING3 -c -o $@ $<

vfsvirtual: vfs_virtual_worker.o start.o
	@echo "Linking vfsvirtual..."
	$(LD) -m elf_i386 $(LDFLAGS) -o $@ $^

# Tranche 5 suite: the worker links the NE2000 hardware core it drives at
# CPL 3 through the TSS I/O bitmap.
drv_ne2k_hw.o: ../kernel/ne2k_hw.c ../kernel/ne2k.h $(USER_HEADERS)
	$(CC) $(CFLAGS) -I.. -DMOHHDY_RING3 -c -o $@ $<
net_worker.o: net_worker.c ../kernel/ne2k.h ../kernel/net_stack_exec.h ../kernel/net_llm_client.h $(USER_HEADERS)
	$(CC) $(CFLAGS) -I.. -DMOHHDY_RING3 -c -o $@ $<

# Tranche 5 pile: the networker links its own copy of the whole network
# stack (ARP/IPv4/UDP/DHCP/DNS/TCP/socket registry/wire engine/TLS/HTTP)
# and runs every relayed network syscall at CPL 3.
NET_STACK_SRCS = ne2k net_nic net_ethernet_arp net_ipv4_udp net_dhcp net_dns net_tcp net_socket \
                 net_llm_socket net_http_tls net_tls_record net_tls_server net_wire net_stack_exec \
                 net_llm_client net_p2p sha256 aes_gcm x509_der bigint ecdsa_p256 x25519 rsa_verify rtc
NET_STACK_OBJS = $(addprefix drvnet_,$(addsuffix .o,$(NET_STACK_SRCS)))
drvnet_%.o: ../kernel/%.c $(USER_HEADERS)
	$(CC) $(CFLAGS) -I.. -DMOHHDY_RING3 -c -o $@ $<

networker: net_worker.o drv_ne2k_hw.o $(NET_STACK_OBJS) net_libc.o start.o
	@echo "Linking networker..."
	$(LD) -m elf_i386 $(LDFLAGS) -o $@ $^

# Tranche 4 suite: the driver links its own copy of the FAT16/FAT32/overlay
# code (-DMOHHDY_RING3: no cli/sti at CPL 3) plus the shared op dispatch.
DRV_FS_CFLAGS = $(CFLAGS) -I.. -DMOHHDY_RING3
DRV_FS_OBJS = drv_fat16.o drv_fat32.o drv_fsop_exec.o drv_overlay.o
drv_fat16.o: ../kernel/fs/fat16.c ../kernel/fs/fat16.h $(USER_HEADERS)
	$(CC) $(DRV_FS_CFLAGS) -c -o $@ $<
drv_fat32.o: ../kernel/fs/fat32.c ../kernel/fs/fat32.h ../kernel/fs/fat16.h $(USER_HEADERS)
	$(CC) $(DRV_FS_CFLAGS) -c -o $@ $<
drv_fsop_exec.o: ../kernel/fs/fsop_exec.c ../kernel/fs/fsop_exec.h ../fs/overlay.h $(USER_HEADERS)
	$(CC) $(DRV_FS_CFLAGS) -c -o $@ $<
drv_overlay.o: ../fs/overlay.c ../fs/overlay.h $(USER_HEADERS)
	$(CC) $(DRV_FS_CFLAGS) -c -o $@ $<
ata_driver.o: ata_driver.c ../kernel/fs/fsop_exec.h ../fs/overlay.h $(USER_HEADERS)
	$(CC) $(DRV_FS_CFLAGS) -c -o $@ $<

atadriver: ata_driver.o $(DRV_FS_OBJS) start.o
	$(LD) -m elf_i386 $(LDFLAGS) -o $@ $^

ataclient: ata_client.o start.o
	$(LD) -m elf_i386 $(LDFLAGS) -o $@ $^

atarogue: ata_rogue.o start.o
	$(LD) -m elf_i386 $(LDFLAGS) -o $@ $^

netclaim: net_claim.o start.o
	@echo "Linking netclaim..."
	$(LD) -m elf_i386 $(LDFLAGS) -o $@ $^

netrelay: net_relay_client.o start.o
	@echo "Linking netrelay..."
	$(LD) -m elf_i386 $(LDFLAGS) -o $@ $^

netwire: net_wire_client.o start.o
	@echo "Linking netwire..."
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

vfshistclaim: vfs_hist_claim.o start.o
	@echo "Linking vfshistclaim..."
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
ipc_wait_server.o ipc_wait_poke.o: ipc_wait_common.h
