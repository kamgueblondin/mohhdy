# Makefile pour les tests MOHHDY

# Configuration de base
CC = gcc
CFLAGS_BASE = -Wall -Wextra -std=c99 -I. -I.. -I../kernel -I../include -DKERNEL_TEST=1
# Les mocks de syscall reproduisent des registres i386 32 bits. Sans cette
# base non-PIE, des pointeurs hôte PIE 64 bits seraient tronqués dans EBX/ECX.
# ENABLE_COVERAGE=1 passes --coverage (-fprofile-arcs -ftest-coverage).
# Notes land next to the test binary under build/, because -o is ../build/...
ifeq ($(ENABLE_COVERAGE),1)
CFLAGS_COVERAGE = --coverage
else
CFLAGS_COVERAGE =
endif
CFLAGS_KERNEL = $(CFLAGS_BASE) -DUSE_INLINE_ASM -g -m32 -fno-pie -no-pie -msse2 -mfpmath=sse $(CFLAGS_COVERAGE)
CFLAGS_USER = $(CFLAGS_BASE) -DMOHHDY_OSUI_HOST_TEST=1 -g -m32 -fno-pie -no-pie $(CFLAGS_COVERAGE)

# Répertoires
TEST_DIR = .
FRAMEWORK_DIR = $(TEST_DIR)/framework
UNIT_DIR = $(TEST_DIR)/unit
INTEGRATION_DIR = $(TEST_DIR)/integration
SYSTEM_DIR = $(TEST_DIR)/system
PERFORMANCE_DIR = $(TEST_DIR)/performance
ROBUSTNESS_DIR = $(TEST_DIR)/robustness
BUILD_DIR = ../build
LOG_DIR = ../test_logs

# Fichiers du framework
FRAMEWORK_SOURCES = $(FRAMEWORK_DIR)/unity.c $(FRAMEWORK_DIR)/test_kernel.c $(FRAMEWORK_DIR)/kernel_mocks.c ../fs/overlay.c
FRAMEWORK_HEADERS = $(FRAMEWORK_DIR)/unity.h $(FRAMEWORK_DIR)/test_kernel.h

# Listes des tests
KERNEL_TESTS = $(wildcard $(UNIT_DIR)/kernel/test_*.c)
USERSPACE_TESTS = $(wildcard $(UNIT_DIR)/userspace/test_*.c)
FS_TESTS = $(wildcard $(UNIT_DIR)/fs/test_*.c)
INTEGRATION_TESTS = $(wildcard $(INTEGRATION_DIR)/test_*.c)
SYSTEM_TESTS = $(wildcard $(SYSTEM_DIR)/test_*.c)
PERFORMANCE_TESTS = $(wildcard $(PERFORMANCE_DIR)/test_*.c)
ROBUSTNESS_TESTS = $(wildcard $(ROBUSTNESS_DIR)/test_*.c)

# Binaires de test
KERNEL_TEST_BINS = $(KERNEL_TESTS:%.c=$(BUILD_DIR)/%)
USERSPACE_TEST_BINS = $(USERSPACE_TESTS:%.c=$(BUILD_DIR)/%)
FS_TEST_BINS = $(FS_TESTS:%.c=$(BUILD_DIR)/%)
INTEGRATION_TEST_BINS = $(INTEGRATION_TESTS:%.c=$(BUILD_DIR)/%)
SYSTEM_TEST_BINS = $(SYSTEM_TESTS:%.c=$(BUILD_DIR)/%)
PERFORMANCE_TEST_BINS = $(PERFORMANCE_TESTS:%.c=$(BUILD_DIR)/%)
ROBUSTNESS_TEST_BINS = $(ROBUSTNESS_TESTS:%.c=$(BUILD_DIR)/%)

ALL_TEST_BINS = $(KERNEL_TEST_BINS) $(USERSPACE_TEST_BINS) $(FS_TEST_BINS) \
                $(INTEGRATION_TEST_BINS) $(SYSTEM_TEST_BINS) \
                $(PERFORMANCE_TEST_BINS) $(ROBUSTNESS_TEST_BINS)

# Cibles principales
.PHONY: all clean test test-quick test-kernel test-userspace test-integration \
        test-performance test-robustness help setup

all: $(ALL_TEST_BINS)
	@echo "All test binaries compiled successfully"

# Cible par défaut
test: all
	@echo "Running complete test suite..."
	@bash ./scripts/run_all_tests.sh

# Tests rapides (critiques seulement)
test-quick: $(KERNEL_TEST_BINS)
	@echo "Running quick test suite..."
	@bash ./scripts/run_all_tests.sh --quick

# Tests par catégorie
test-kernel: $(KERNEL_TEST_BINS)
	@echo "Running kernel tests..."
	@bash ./scripts/run_all_tests.sh --kernel-only

test-userspace: $(USERSPACE_TEST_BINS)
	@echo "Running userspace tests..."
	@bash ./scripts/run_all_tests.sh --userspace-only

test-integration: $(INTEGRATION_TEST_BINS)
	@echo "Running integration tests..."
	@bash ./scripts/run_all_tests.sh --integration

test-performance: $(PERFORMANCE_TEST_BINS)
	@echo "Running performance tests..."
	@bash ./scripts/run_all_tests.sh --performance

test-robustness: $(ROBUSTNESS_TEST_BINS)
	@echo "Running robustness tests..."
	@bash ./scripts/run_all_tests.sh --robustness

# Compilation des tests kernel
$(BUILD_DIR)/$(UNIT_DIR)/kernel/test_tokenizer: $(UNIT_DIR)/kernel/test_tokenizer.c ../kernel/llm/gpt2_tokenizer.c $(FRAMEWORK_SOURCES) $(FRAMEWORK_HEADERS)
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS_KERNEL) -o $@ $< ../kernel/llm/gpt2_tokenizer.c $(FRAMEWORK_SOURCES)
	@echo "Compiled kernel test: $(notdir $@)"

$(BUILD_DIR)/$(UNIT_DIR)/kernel/test_gpt2_sample: $(UNIT_DIR)/kernel/test_gpt2_sample.c ../kernel/llm/gpt2_sample.c $(FRAMEWORK_SOURCES) $(FRAMEWORK_HEADERS)
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS_KERNEL) -o $@ $< ../kernel/llm/gpt2_sample.c $(FRAMEWORK_SOURCES)
	@echo "Compiled kernel test: $(notdir $@)"

$(BUILD_DIR)/$(UNIT_DIR)/kernel/test_gpt2_gguf_session: $(UNIT_DIR)/kernel/test_gpt2_gguf_session.c ../kernel/llm/gpt2_gguf_session.c ../kernel/llm/gpt2_generate.c ../kernel/llm/gpt2_infer.c ../kernel/llm/gpt2_sample.c ../kernel/llm/gpt2_tokenizer.c ../kernel/llm/gpt2_model.c $(FRAMEWORK_SOURCES) $(FRAMEWORK_HEADERS)
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS_KERNEL) -DMOHHDY_RING3 -o $@ $< ../kernel/llm/gpt2_gguf_session.c ../kernel/llm/gpt2_generate.c ../kernel/llm/gpt2_infer.c ../kernel/llm/gpt2_sample.c ../kernel/llm/gpt2_tokenizer.c ../kernel/llm/gpt2_model.c $(FRAMEWORK_SOURCES)
	@echo "Compiled kernel test: $(notdir $@)"

$(BUILD_DIR)/$(UNIT_DIR)/kernel/test_gpt2_generate: $(UNIT_DIR)/kernel/test_gpt2_generate.c ../kernel/llm/gpt2_generate.c ../kernel/llm/gpt2_infer.c ../kernel/llm/gpt2_sample.c ../kernel/llm/gpt2_tokenizer.c ../kernel/llm/gpt2_model.c $(FRAMEWORK_SOURCES) $(FRAMEWORK_HEADERS)
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS_KERNEL) -DMOHHDY_RING3 -o $@ $< ../kernel/llm/gpt2_generate.c ../kernel/llm/gpt2_infer.c ../kernel/llm/gpt2_sample.c ../kernel/llm/gpt2_tokenizer.c ../kernel/llm/gpt2_model.c $(FRAMEWORK_SOURCES)
	@echo "Compiled kernel test: $(notdir $@)"

$(BUILD_DIR)/$(UNIT_DIR)/kernel/test_gpt2_infer: $(UNIT_DIR)/kernel/test_gpt2_infer.c ../kernel/llm/gpt2_infer.c ../kernel/llm/gpt2_sample.c $(FRAMEWORK_SOURCES) $(FRAMEWORK_HEADERS)
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS_KERNEL) -o $@ $< ../kernel/llm/gpt2_infer.c ../kernel/llm/gpt2_sample.c $(FRAMEWORK_SOURCES)
	@echo "Compiled kernel test: $(notdir $@)"

$(BUILD_DIR)/$(UNIT_DIR)/kernel/test_gpt2_gguf_infer: $(UNIT_DIR)/kernel/test_gpt2_gguf_infer.c ../kernel/fs/fat16.c ../kernel/fs/fat32.c ../kernel/llm/gpt2_gguf.c ../kernel/llm/gpt2_gguf_loader.c ../kernel/llm/gpt2_quant.c ../kernel/llm/gpt2_gguf_infer.c ../kernel/llm/gpt2_sample.c $(FRAMEWORK_SOURCES) $(FRAMEWORK_HEADERS)
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS_KERNEL) -DMOHHDY_RING3 -o $@ $< ../kernel/fs/fat16.c ../kernel/fs/fat32.c ../kernel/llm/gpt2_gguf.c ../kernel/llm/gpt2_gguf_loader.c ../kernel/llm/gpt2_quant.c ../kernel/llm/gpt2_gguf_infer.c ../kernel/llm/gpt2_sample.c $(FRAMEWORK_SOURCES)
	@echo "Compiled kernel test: $(notdir $@)"

$(BUILD_DIR)/$(UNIT_DIR)/kernel/test_gpt2_gguf: $(UNIT_DIR)/kernel/test_gpt2_gguf.c ../kernel/fs/fat16.c ../kernel/fs/fat32.c ../kernel/llm/gpt2_gguf.c ../kernel/llm/gpt2_gguf_loader.c ../kernel/llm/gpt2_quant.c ../kernel/llm/gpt2_sample.c $(FRAMEWORK_SOURCES) $(FRAMEWORK_HEADERS)
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS_KERNEL) -o $@ $< ../kernel/fs/fat16.c ../kernel/fs/fat32.c ../kernel/llm/gpt2_gguf.c ../kernel/llm/gpt2_gguf_loader.c ../kernel/llm/gpt2_quant.c ../kernel/llm/gpt2_sample.c $(FRAMEWORK_SOURCES)
	@echo "Compiled kernel test: $(notdir $@)"

$(BUILD_DIR)/$(UNIT_DIR)/kernel/test_fat16: $(UNIT_DIR)/kernel/test_fat16.c ../kernel/fs/fat16.c ../kernel/fs/fat32.c ../kernel/llm/gpt2_gguf.c ../kernel/llm/gpt2_gguf_loader.c ../kernel/llm/gpt2_quant.c ../kernel/llm/gpt2_sample.c $(FRAMEWORK_SOURCES) $(FRAMEWORK_HEADERS)
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS_KERNEL) -o $@ $< ../kernel/fs/fat16.c ../kernel/fs/fat32.c ../kernel/llm/gpt2_gguf.c ../kernel/llm/gpt2_gguf_loader.c ../kernel/llm/gpt2_quant.c ../kernel/llm/gpt2_sample.c $(FRAMEWORK_SOURCES)
	@echo "Compiled kernel test: $(notdir $@)"

$(BUILD_DIR)/$(UNIT_DIR)/kernel/test_gfx_wm: $(UNIT_DIR)/kernel/test_gfx_wm.c ../kernel/gfx_wm.c ../kernel/gfx_wm.h $(FRAMEWORK_SOURCES) $(FRAMEWORK_HEADERS)
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS_KERNEL) -o $@ $< ../kernel/gfx_wm.c $(FRAMEWORK_SOURCES)
	@echo "Compiled kernel test: $(notdir $@)"

$(BUILD_DIR)/$(UNIT_DIR)/kernel/test_line_edit: $(UNIT_DIR)/kernel/test_line_edit.c ../kernel/line_edit.c ../kernel/line_edit.h $(FRAMEWORK_SOURCES) $(FRAMEWORK_HEADERS)
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS_KERNEL) -o $@ $< ../kernel/line_edit.c $(FRAMEWORK_SOURCES)
	@echo "Compiled kernel test: $(notdir $@)"

$(BUILD_DIR)/$(UNIT_DIR)/kernel/test_gfx_term: $(UNIT_DIR)/kernel/test_gfx_term.c ../kernel/gfx_term.c ../kernel/gfx_term.h ../kernel/gfx_wm.c ../kernel/gfx_wm.h $(FRAMEWORK_SOURCES) $(FRAMEWORK_HEADERS)
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS_KERNEL) -o $@ $< ../kernel/gfx_term.c ../kernel/gfx_wm.c $(FRAMEWORK_SOURCES)
	@echo "Compiled kernel test: $(notdir $@)"

$(BUILD_DIR)/$(UNIT_DIR)/kernel/test_vga_console: $(UNIT_DIR)/kernel/test_vga_console.c ../kernel/vga_console.c $(FRAMEWORK_SOURCES) $(FRAMEWORK_HEADERS)
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS_KERNEL) -o $@ $< ../kernel/vga_console.c $(FRAMEWORK_SOURCES)
	@echo "Compiled kernel test: $(notdir $@)"

$(BUILD_DIR)/$(UNIT_DIR)/kernel/test_gfx_desktop: $(UNIT_DIR)/kernel/test_gfx_desktop.c ../kernel/gfx_desktop.c ../kernel/gfx_fb.c ../kernel/edid.c ../kernel/edid.h ../kernel/clipboard.c ../kernel/gfx_wm.c ../kernel/gfx_term.c ../kernel/pci_service.c ../kernel/input/usb_tablet.c ../kernel/gfx_desktop.h ../kernel/gfx_fb.h ../kernel/gfx_font8.h ../kernel/gfx_font_hd.h $(FRAMEWORK_SOURCES) $(FRAMEWORK_HEADERS) ../kernel/edid.h
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS_KERNEL) -o $@ $< ../kernel/gfx_desktop.c ../kernel/gfx_fb.c ../kernel/edid.c ../kernel/clipboard.c ../kernel/gfx_wm.c ../kernel/gfx_term.c ../kernel/pci_service.c ../kernel/input/usb_tablet.c $(FRAMEWORK_SOURCES)
	@echo "Compiled kernel test: $(notdir $@)"

$(BUILD_DIR)/$(UNIT_DIR)/kernel/test_net_ethernet_arp: $(UNIT_DIR)/kernel/test_net_ethernet_arp.c ../kernel/net_ethernet_arp.c $(FRAMEWORK_SOURCES) $(FRAMEWORK_HEADERS)
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS_KERNEL) -o $@ $< ../kernel/net_ethernet_arp.c $(FRAMEWORK_SOURCES)
	@echo "Compiled kernel test: $(notdir $@)"


$(BUILD_DIR)/$(UNIT_DIR)/kernel/test_net_tls_record: $(UNIT_DIR)/kernel/test_net_tls_record.c ../kernel/net_tls_record.c ../kernel/sha256.c ../kernel/aes_gcm.c ../kernel/x509_der.c ../kernel/rsa_verify.c ../kernel/bigint.c ../kernel/ecdsa_p256.c ../kernel/x25519.c $(FRAMEWORK_SOURCES) $(FRAMEWORK_HEADERS)
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS_KERNEL) -o $@ $< ../kernel/net_tls_record.c ../kernel/sha256.c ../kernel/aes_gcm.c ../kernel/x509_der.c ../kernel/rsa_verify.c ../kernel/bigint.c ../kernel/ecdsa_p256.c ../kernel/x25519.c $(FRAMEWORK_SOURCES)
	@echo "Compiled kernel test: $(notdir $@)"


$(BUILD_DIR)/$(UNIT_DIR)/kernel/test_tls_rsa_handshake: $(UNIT_DIR)/kernel/test_tls_rsa_handshake.c ../kernel/net_tls_record.c ../kernel/sha256.c ../kernel/aes_gcm.c ../kernel/x509_der.c ../kernel/rsa_verify.c ../kernel/bigint.c ../kernel/ecdsa_p256.c ../kernel/x25519.c $(FRAMEWORK_SOURCES) $(FRAMEWORK_HEADERS)
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS_KERNEL) -o $@ $< ../kernel/net_tls_record.c ../kernel/sha256.c ../kernel/aes_gcm.c ../kernel/x509_der.c ../kernel/rsa_verify.c ../kernel/bigint.c ../kernel/ecdsa_p256.c ../kernel/x25519.c $(FRAMEWORK_SOURCES)
	@echo "Compiled kernel test: $(notdir $@)"

$(BUILD_DIR)/$(UNIT_DIR)/kernel/test_aes_gcm: $(UNIT_DIR)/kernel/test_aes_gcm.c ../kernel/aes_gcm.c $(FRAMEWORK_SOURCES) $(FRAMEWORK_HEADERS)
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS_KERNEL) -o $@ $< ../kernel/aes_gcm.c $(FRAMEWORK_SOURCES)
	@echo "Compiled kernel test: $(notdir $@)"

$(BUILD_DIR)/$(UNIT_DIR)/kernel/test_bigint: $(UNIT_DIR)/kernel/test_bigint.c ../kernel/bigint.c $(FRAMEWORK_SOURCES) $(FRAMEWORK_HEADERS)
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS_KERNEL) -o $@ $< ../kernel/bigint.c $(FRAMEWORK_SOURCES)
	@echo "Compiled kernel test: $(notdir $@)"

$(BUILD_DIR)/$(UNIT_DIR)/kernel/test_ecdsa_p256: $(UNIT_DIR)/kernel/test_ecdsa_p256.c ../kernel/ecdsa_p256.c ../kernel/bigint.c $(FRAMEWORK_SOURCES) $(FRAMEWORK_HEADERS)
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS_KERNEL) -o $@ $< ../kernel/ecdsa_p256.c ../kernel/bigint.c $(FRAMEWORK_SOURCES)
	@echo "Compiled kernel test: $(notdir $@)"

$(BUILD_DIR)/$(UNIT_DIR)/kernel/test_x25519: $(UNIT_DIR)/kernel/test_x25519.c ../kernel/x25519.c ../kernel/bigint.c $(FRAMEWORK_SOURCES) $(FRAMEWORK_HEADERS)
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS_KERNEL) -o $@ $< ../kernel/x25519.c ../kernel/bigint.c $(FRAMEWORK_SOURCES)
	@echo "Compiled kernel test: $(notdir $@)"

$(BUILD_DIR)/$(UNIT_DIR)/kernel/test_rsa_verify: $(UNIT_DIR)/kernel/test_rsa_verify.c ../kernel/rsa_verify.c ../kernel/bigint.c $(FRAMEWORK_SOURCES) $(FRAMEWORK_HEADERS)
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS_KERNEL) -o $@ $< ../kernel/rsa_verify.c ../kernel/bigint.c $(FRAMEWORK_SOURCES)
	@echo "Compiled kernel test: $(notdir $@)"

$(BUILD_DIR)/$(UNIT_DIR)/kernel/test_x509_der: $(UNIT_DIR)/kernel/test_x509_der.c ../kernel/x509_der.c ../kernel/sha256.c ../kernel/rsa_verify.c ../kernel/bigint.c ../kernel/ecdsa_p256.c $(FRAMEWORK_SOURCES) $(FRAMEWORK_HEADERS)
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS_KERNEL) -o $@ $< ../kernel/x509_der.c ../kernel/sha256.c ../kernel/rsa_verify.c ../kernel/bigint.c ../kernel/ecdsa_p256.c $(FRAMEWORK_SOURCES)
	@echo "Compiled kernel test: $(notdir $@)"

$(BUILD_DIR)/$(UNIT_DIR)/kernel/test_sha256: $(UNIT_DIR)/kernel/test_sha256.c ../kernel/sha256.c $(FRAMEWORK_SOURCES) $(FRAMEWORK_HEADERS)
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS_KERNEL) -o $@ $< ../kernel/sha256.c $(FRAMEWORK_SOURCES)
	@echo "Compiled kernel test: $(notdir $@)"


$(BUILD_DIR)/$(UNIT_DIR)/kernel/test_net_socket: $(UNIT_DIR)/kernel/test_net_socket.c ../kernel/net_socket.c ../kernel/net_tcp.c ../kernel/net_tls_record.c ../kernel/sha256.c ../kernel/aes_gcm.c ../kernel/x509_der.c ../kernel/x25519.c ../kernel/rsa_verify.c ../kernel/bigint.c ../kernel/ecdsa_p256.c $(FRAMEWORK_SOURCES) $(FRAMEWORK_HEADERS)
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS_KERNEL) -o $@ $< ../kernel/net_socket.c ../kernel/net_tcp.c ../kernel/net_tls_record.c ../kernel/sha256.c ../kernel/aes_gcm.c ../kernel/x509_der.c ../kernel/x25519.c ../kernel/rsa_verify.c ../kernel/bigint.c ../kernel/ecdsa_p256.c $(FRAMEWORK_SOURCES)
	@echo "Compiled kernel test: $(notdir $@)"

$(BUILD_DIR)/$(UNIT_DIR)/kernel/test_net_llm_socket: $(UNIT_DIR)/kernel/test_net_llm_socket.c ../kernel/net_llm_socket.c ../kernel/net_socket.c ../kernel/net_http_tls.c ../kernel/net_tcp.c ../kernel/net_tls_record.c ../kernel/sha256.c ../kernel/aes_gcm.c ../kernel/x509_der.c ../kernel/x25519.c ../kernel/rsa_verify.c ../kernel/bigint.c ../kernel/ecdsa_p256.c $(FRAMEWORK_SOURCES) $(FRAMEWORK_HEADERS)
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS_KERNEL) -o $@ $< ../kernel/net_llm_socket.c ../kernel/net_socket.c ../kernel/net_http_tls.c ../kernel/net_tcp.c ../kernel/net_tls_record.c ../kernel/sha256.c ../kernel/aes_gcm.c ../kernel/x509_der.c ../kernel/x25519.c ../kernel/rsa_verify.c ../kernel/bigint.c ../kernel/ecdsa_p256.c $(FRAMEWORK_SOURCES)
	@echo "Compiled kernel test: $(notdir $@)"

$(BUILD_DIR)/$(UNIT_DIR)/kernel/test_net_tcp: $(UNIT_DIR)/kernel/test_net_tcp.c ../kernel/net_http_tls.c ../kernel/net_tcp.c ../kernel/net_tls_record.c ../kernel/sha256.c ../kernel/aes_gcm.c ../kernel/x509_der.c ../kernel/rsa_verify.c ../kernel/bigint.c ../kernel/ecdsa_p256.c ../kernel/x25519.c $(FRAMEWORK_SOURCES) $(FRAMEWORK_HEADERS)
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS_KERNEL) -o $@ $< ../kernel/net_http_tls.c ../kernel/net_tcp.c ../kernel/net_tls_record.c ../kernel/sha256.c ../kernel/aes_gcm.c ../kernel/x509_der.c ../kernel/rsa_verify.c ../kernel/bigint.c ../kernel/ecdsa_p256.c ../kernel/x25519.c $(FRAMEWORK_SOURCES)
	@echo "Compiled kernel test: $(notdir $@)"

$(BUILD_DIR)/$(UNIT_DIR)/kernel/test_net_http_tls: $(UNIT_DIR)/kernel/test_net_http_tls.c ../kernel/net_http_tls.c ../kernel/net_tcp.c ../kernel/net_tls_record.c ../kernel/sha256.c ../kernel/aes_gcm.c ../kernel/x509_der.c ../kernel/rsa_verify.c ../kernel/bigint.c ../kernel/ecdsa_p256.c ../kernel/x25519.c $(FRAMEWORK_SOURCES) $(FRAMEWORK_HEADERS)
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS_KERNEL) -o $@ $< ../kernel/net_http_tls.c ../kernel/net_tcp.c ../kernel/net_tls_record.c ../kernel/sha256.c ../kernel/aes_gcm.c ../kernel/x509_der.c ../kernel/rsa_verify.c ../kernel/bigint.c ../kernel/ecdsa_p256.c ../kernel/x25519.c $(FRAMEWORK_SOURCES)
	@echo "Compiled kernel test: $(notdir $@)"


$(BUILD_DIR)/$(UNIT_DIR)/kernel/test_fat32: $(UNIT_DIR)/kernel/test_fat32.c ../kernel/fs/fat32.c ../kernel/fs/fat16.c $(FRAMEWORK_SOURCES) $(FRAMEWORK_HEADERS)
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS_KERNEL) -o $@ $< ../kernel/fs/fat32.c ../kernel/fs/fat16.c $(FRAMEWORK_SOURCES)
	@echo "Compiled kernel test: $(notdir $@)"

$(BUILD_DIR)/$(UNIT_DIR)/kernel/test_net_dns: $(UNIT_DIR)/kernel/test_net_dns.c ../kernel/net_dns.c $(FRAMEWORK_SOURCES) $(FRAMEWORK_HEADERS)
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS_KERNEL) -o $@ $< ../kernel/net_dns.c $(FRAMEWORK_SOURCES)
	@echo "Compiled kernel test: $(notdir $@)"


$(BUILD_DIR)/$(UNIT_DIR)/kernel/test_net_ipv4_udp: $(UNIT_DIR)/kernel/test_net_ipv4_udp.c ../kernel/net_ipv4_udp.c $(FRAMEWORK_SOURCES) $(FRAMEWORK_HEADERS)
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS_KERNEL) -o $@ $< ../kernel/net_ipv4_udp.c $(FRAMEWORK_SOURCES)
	@echo "Compiled kernel test: $(notdir $@)"


$(BUILD_DIR)/$(UNIT_DIR)/kernel/test_net_dhcp: $(UNIT_DIR)/kernel/test_net_dhcp.c ../kernel/net_dhcp.c $(FRAMEWORK_SOURCES) $(FRAMEWORK_HEADERS)
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS_KERNEL) -o $@ $< ../kernel/net_dhcp.c $(FRAMEWORK_SOURCES)
	@echo "Compiled kernel test: $(notdir $@)"


$(BUILD_DIR)/$(UNIT_DIR)/kernel/test_ne2k: $(UNIT_DIR)/kernel/test_ne2k.c ../kernel/ne2k.c ../kernel/ne2k_hw.c ../kernel/net_socket.c ../kernel/net_llm_socket.c ../kernel/rtc.c ../kernel/net_nic.c ../kernel/net_ethernet_arp.c ../kernel/net_ipv4_udp.c ../kernel/net_dhcp.c ../kernel/net_dns.c ../kernel/net_tcp.c ../kernel/net_tls_record.c ../kernel/net_http_tls.c ../kernel/sha256.c ../kernel/aes_gcm.c ../kernel/x509_der.c ../kernel/rsa_verify.c ../kernel/bigint.c ../kernel/ecdsa_p256.c ../kernel/x25519.c $(FRAMEWORK_SOURCES) $(FRAMEWORK_HEADERS)
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS_KERNEL) -o $@ $< ../kernel/ne2k.c ../kernel/ne2k_hw.c ../kernel/net_socket.c ../kernel/net_llm_socket.c ../kernel/rtc.c ../kernel/net_nic.c ../kernel/net_ethernet_arp.c ../kernel/net_ipv4_udp.c ../kernel/net_dhcp.c ../kernel/net_dns.c ../kernel/net_tcp.c ../kernel/net_tls_record.c ../kernel/net_http_tls.c ../kernel/sha256.c ../kernel/aes_gcm.c ../kernel/x509_der.c ../kernel/rsa_verify.c ../kernel/bigint.c ../kernel/ecdsa_p256.c ../kernel/x25519.c $(FRAMEWORK_SOURCES)
	@echo "Compiled kernel test: $(notdir $@)"


$(BUILD_DIR)/$(UNIT_DIR)/kernel/test_pci: $(UNIT_DIR)/kernel/test_pci.c ../kernel/pci.c $(FRAMEWORK_SOURCES) $(FRAMEWORK_HEADERS)
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS_KERNEL) -o $@ $< ../kernel/pci.c $(FRAMEWORK_SOURCES)
	@echo "Compiled kernel test: $(notdir $@)"


$(BUILD_DIR)/$(UNIT_DIR)/kernel/test_net_nic: $(UNIT_DIR)/kernel/test_net_nic.c ../kernel/net_nic.c $(FRAMEWORK_SOURCES) $(FRAMEWORK_HEADERS)
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS_KERNEL) -o $@ $< ../kernel/net_nic.c $(FRAMEWORK_SOURCES)
	@echo "Compiled kernel test: $(notdir $@)"


$(BUILD_DIR)/$(UNIT_DIR)/kernel/test_gpt2_quant: $(UNIT_DIR)/kernel/test_gpt2_quant.c ../kernel/llm/gpt2_quant.c $(FRAMEWORK_SOURCES) $(FRAMEWORK_HEADERS)
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS_KERNEL) -o $@ $< ../kernel/llm/gpt2_quant.c $(FRAMEWORK_SOURCES)
	@echo "Compiled kernel test: $(notdir $@)"

$(BUILD_DIR)/$(UNIT_DIR)/kernel/test_ipc: $(UNIT_DIR)/kernel/test_ipc.c ../kernel/ipc.c $(FRAMEWORK_SOURCES) $(FRAMEWORK_HEADERS)
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS_KERNEL) -o $@ $< ../kernel/ipc.c $(FRAMEWORK_SOURCES)
	@echo "Compiled kernel test: $(notdir $@)"

$(BUILD_DIR)/$(UNIT_DIR)/kernel/test_service_registry: $(UNIT_DIR)/kernel/test_service_registry.c ../kernel/service_registry.c $(FRAMEWORK_SOURCES) $(FRAMEWORK_HEADERS)
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS_KERNEL) -o $@ $< ../kernel/service_registry.c $(FRAMEWORK_SOURCES)
	@echo "Compiled kernel test: $(notdir $@)"

$(BUILD_DIR)/$(ROBUSTNESS_DIR)/test_gpt2_gguf_bounds: $(ROBUSTNESS_DIR)/test_gpt2_gguf_bounds.c ../kernel/llm/gpt2_gguf.c $(FRAMEWORK_SOURCES) $(FRAMEWORK_HEADERS)
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS_KERNEL) -o $@ $< ../kernel/llm/gpt2_gguf.c $(FRAMEWORK_SOURCES)
	@echo "Compiled robustness test: $(notdir $@)"

$(BUILD_DIR)/$(UNIT_DIR)/kernel/test_rtc: $(UNIT_DIR)/kernel/test_rtc.c ../kernel/rtc.c $(FRAMEWORK_SOURCES) $(FRAMEWORK_HEADERS)
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS_KERNEL) -o $@ $< ../kernel/rtc.c $(FRAMEWORK_SOURCES)
	@echo "Compiled kernel test: $(notdir $@)"

$(BUILD_DIR)/$(UNIT_DIR)/kernel/test_pci_service: $(UNIT_DIR)/kernel/test_pci_service.c ../kernel/pci_service.c $(FRAMEWORK_SOURCES) $(FRAMEWORK_HEADERS)
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS_KERNEL) -o $@ $< ../kernel/pci_service.c $(FRAMEWORK_SOURCES)
	@echo "Compiled kernel test: $(notdir $@)"

$(BUILD_DIR)/$(UNIT_DIR)/kernel/test_key_seal: $(UNIT_DIR)/kernel/test_key_seal.c ../kernel/key_seal.c ../kernel/key_seal.h ../kernel/aes_gcm.c ../kernel/sha256.c $(FRAMEWORK_SOURCES) $(FRAMEWORK_HEADERS)
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS_KERNEL) -o $@ $< ../kernel/key_seal.c ../kernel/aes_gcm.c ../kernel/sha256.c $(FRAMEWORK_SOURCES)
	@echo "Compiled kernel test: $(notdir $@)"

$(BUILD_DIR)/$(UNIT_DIR)/kernel/test_device_service: $(UNIT_DIR)/kernel/test_device_service.c ../kernel/device_service.c ../kernel/device_service.h $(FRAMEWORK_SOURCES) $(FRAMEWORK_HEADERS)
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS_KERNEL) -o $@ $< ../kernel/device_service.c $(FRAMEWORK_SOURCES)
	@echo "Compiled kernel test: $(notdir $@)"

$(BUILD_DIR)/$(UNIT_DIR)/kernel/test_arm_mmu: $(UNIT_DIR)/kernel/test_arm_mmu.c ../arch/arm/arm_mmu.c ../arch/arm/arm_mmu.h $(FRAMEWORK_SOURCES) $(FRAMEWORK_HEADERS)
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS_KERNEL) -o $@ $< ../arch/arm/arm_mmu.c $(FRAMEWORK_SOURCES)
	@echo "Compiled kernel test: $(notdir $@)"

$(BUILD_DIR)/$(UNIT_DIR)/kernel/test_usb_dma: $(UNIT_DIR)/kernel/test_usb_dma.c ../kernel/usb_dma.c ../kernel/usb_dma.h ../kernel/io_bitmap.c $(FRAMEWORK_SOURCES) $(FRAMEWORK_HEADERS)
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS_KERNEL) -o $@ $< ../kernel/usb_dma.c ../kernel/io_bitmap.c $(FRAMEWORK_SOURCES)
	@echo "Compiled kernel test: $(notdir $@)"

$(BUILD_DIR)/$(UNIT_DIR)/kernel/test_io_bitmap: $(UNIT_DIR)/kernel/test_io_bitmap.c ../kernel/io_bitmap.c $(FRAMEWORK_SOURCES) $(FRAMEWORK_HEADERS)
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS_KERNEL) -o $@ $< ../kernel/io_bitmap.c $(FRAMEWORK_SOURCES)
	@echo "Compiled kernel test: $(notdir $@)"

$(BUILD_DIR)/$(UNIT_DIR)/kernel/test_net_nic_owner: $(UNIT_DIR)/kernel/test_net_nic_owner.c ../kernel/net_nic_owner.c $(FRAMEWORK_SOURCES) $(FRAMEWORK_HEADERS)
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS_KERNEL) -o $@ $< ../kernel/net_nic_owner.c $(FRAMEWORK_SOURCES)
	@echo "Compiled kernel test: $(notdir $@)"

$(BUILD_DIR)/$(UNIT_DIR)/kernel/test_net_wire: $(UNIT_DIR)/kernel/test_net_wire.c ../kernel/net_wire.c ../kernel/net_relay.c ../kernel/ne2k.c ../kernel/ne2k_hw.c ../kernel/net_socket.c ../kernel/net_llm_socket.c ../kernel/rtc.c ../kernel/net_nic.c ../kernel/net_ethernet_arp.c ../kernel/net_ipv4_udp.c ../kernel/net_dhcp.c ../kernel/net_dns.c ../kernel/net_tcp.c ../kernel/net_tls_record.c ../kernel/net_http_tls.c ../kernel/sha256.c ../kernel/aes_gcm.c ../kernel/x509_der.c ../kernel/rsa_verify.c ../kernel/bigint.c ../kernel/ecdsa_p256.c ../kernel/x25519.c $(FRAMEWORK_SOURCES) $(FRAMEWORK_HEADERS)
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS_KERNEL) -o $@ $< ../kernel/net_wire.c ../kernel/net_relay.c ../kernel/ne2k.c ../kernel/ne2k_hw.c ../kernel/net_socket.c ../kernel/net_llm_socket.c ../kernel/rtc.c ../kernel/net_nic.c ../kernel/net_ethernet_arp.c ../kernel/net_ipv4_udp.c ../kernel/net_dhcp.c ../kernel/net_dns.c ../kernel/net_tcp.c ../kernel/net_tls_record.c ../kernel/net_http_tls.c ../kernel/sha256.c ../kernel/aes_gcm.c ../kernel/x509_der.c ../kernel/rsa_verify.c ../kernel/bigint.c ../kernel/ecdsa_p256.c ../kernel/x25519.c $(FRAMEWORK_SOURCES)
	@echo "Compiled kernel test: $(notdir $@)"

$(BUILD_DIR)/$(UNIT_DIR)/kernel/test_net_p2p: $(UNIT_DIR)/kernel/test_net_p2p.c ../kernel/net_p2p.c ../kernel/net_ipv4_udp.c $(FRAMEWORK_SOURCES) $(FRAMEWORK_HEADERS)
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS_KERNEL) -o $@ $< ../kernel/net_p2p.c ../kernel/net_ipv4_udp.c $(FRAMEWORK_SOURCES)

$(BUILD_DIR)/$(UNIT_DIR)/kernel/test_net_stack_exec: $(UNIT_DIR)/kernel/test_net_stack_exec.c ../kernel/net_stack_exec.c ../kernel/net_llm_client.c ../kernel/net_p2p.c ../kernel/net_tls_server.c ../kernel/net_wire.c ../kernel/net_relay.c ../kernel/net_nic_owner.c ../kernel/ne2k.c ../kernel/ne2k_hw.c ../kernel/net_socket.c ../kernel/net_llm_socket.c ../kernel/rtc.c ../kernel/net_nic.c ../kernel/net_ethernet_arp.c ../kernel/net_ipv4_udp.c ../kernel/net_dhcp.c ../kernel/net_dns.c ../kernel/net_tcp.c ../kernel/net_tls_record.c ../kernel/net_http_tls.c ../kernel/sha256.c ../kernel/aes_gcm.c ../kernel/x509_der.c ../kernel/rsa_verify.c ../kernel/bigint.c ../kernel/ecdsa_p256.c ../kernel/x25519.c $(FRAMEWORK_SOURCES) $(FRAMEWORK_HEADERS)
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS_KERNEL) -o $@ $< ../kernel/net_stack_exec.c ../kernel/net_llm_client.c ../kernel/net_p2p.c ../kernel/net_tls_server.c ../kernel/net_wire.c ../kernel/net_relay.c ../kernel/net_nic_owner.c ../kernel/ne2k.c ../kernel/ne2k_hw.c ../kernel/net_socket.c ../kernel/net_llm_socket.c ../kernel/rtc.c ../kernel/net_nic.c ../kernel/net_ethernet_arp.c ../kernel/net_ipv4_udp.c ../kernel/net_dhcp.c ../kernel/net_dns.c ../kernel/net_tcp.c ../kernel/net_tls_record.c ../kernel/net_http_tls.c ../kernel/sha256.c ../kernel/aes_gcm.c ../kernel/x509_der.c ../kernel/rsa_verify.c ../kernel/bigint.c ../kernel/ecdsa_p256.c ../kernel/x25519.c $(FRAMEWORK_SOURCES)
	@echo "Compiled kernel test: $(notdir $@)"

$(BUILD_DIR)/$(UNIT_DIR)/kernel/test_net_relay: $(UNIT_DIR)/kernel/test_net_relay.c ../kernel/net_relay.c ../kernel/net_socket.c ../kernel/net_tcp.c ../kernel/net_tls_record.c ../kernel/sha256.c ../kernel/aes_gcm.c ../kernel/x509_der.c ../kernel/x25519.c ../kernel/rsa_verify.c ../kernel/bigint.c ../kernel/ecdsa_p256.c $(FRAMEWORK_SOURCES) $(FRAMEWORK_HEADERS)
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS_KERNEL) -o $@ $< ../kernel/net_relay.c ../kernel/net_socket.c ../kernel/net_tcp.c ../kernel/net_tls_record.c ../kernel/sha256.c ../kernel/aes_gcm.c ../kernel/x509_der.c ../kernel/x25519.c ../kernel/rsa_verify.c ../kernel/bigint.c ../kernel/ecdsa_p256.c $(FRAMEWORK_SOURCES)
	@echo "Compiled kernel test: $(notdir $@)"

$(BUILD_DIR)/$(UNIT_DIR)/kernel/test_ata_job: $(UNIT_DIR)/kernel/test_ata_job.c ../kernel/ata_job.c $(FRAMEWORK_SOURCES) $(FRAMEWORK_HEADERS)
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS_KERNEL) -o $@ $< ../kernel/ata_job.c $(FRAMEWORK_SOURCES)
	@echo "Compiled kernel test: $(notdir $@)"

# Tranche 4 suite: FS op slot/mirror + shared FAT/overlay codec and dispatch.
ATA_FSOP_SOURCES = ../kernel/ata_fsop.c ../kernel/fs/fsop_exec.c ../kernel/fs/fat16.c ../kernel/fs/fat32.c
$(BUILD_DIR)/$(UNIT_DIR)/kernel/test_ata_fsop: $(UNIT_DIR)/kernel/test_ata_fsop.c $(ATA_FSOP_SOURCES) $(FRAMEWORK_SOURCES) $(FRAMEWORK_HEADERS)
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS_KERNEL) -o $@ $< $(ATA_FSOP_SOURCES) $(FRAMEWORK_SOURCES)
	@echo "Compiled kernel test: $(notdir $@)"

$(BUILD_DIR)/$(UNIT_DIR)/kernel/test_%: $(UNIT_DIR)/kernel/test_%.c $(FRAMEWORK_SOURCES) $(FRAMEWORK_HEADERS)
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS_KERNEL) -o $@ $< $(FRAMEWORK_SOURCES)
	@echo "Compiled kernel test: $(notdir $@)"

# Compilation des tests userspace
$(BUILD_DIR)/$(UNIT_DIR)/userspace/test_ramfs: $(UNIT_DIR)/userspace/test_ramfs.c ../userspace/ramfs.c ../userspace/procsim.c $(FRAMEWORK_SOURCES) $(FRAMEWORK_HEADERS)
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS_USER) -I../userspace -o $@ $< ../userspace/ramfs.c ../userspace/procsim.c $(FRAMEWORK_SOURCES)
	@echo "Compiled userspace test: $(notdir $@)"

$(BUILD_DIR)/$(UNIT_DIR)/userspace/test_ui_scale: $(UNIT_DIR)/userspace/test_ui_scale.c ../userspace/osui_scale.c ../userspace/osui_scale.h ../include/os_ui_scale.h $(FRAMEWORK_SOURCES) $(FRAMEWORK_HEADERS)
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS_USER) -I../userspace -I../include -o $@ $< ../userspace/osui_scale.c $(FRAMEWORK_SOURCES)
	@echo "Compiled userspace test: $(notdir $@)"

$(BUILD_DIR)/$(UNIT_DIR)/userspace/test_ps2_decode: $(UNIT_DIR)/userspace/test_ps2_decode.c ../userspace/ps2_decode.c ../userspace/ps2_decode.h $(FRAMEWORK_SOURCES) $(FRAMEWORK_HEADERS)
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS_USER) -I../userspace -I../include -o $@ $< ../userspace/ps2_decode.c $(FRAMEWORK_SOURCES)
	@echo "Compiled userspace test: $(notdir $@)"

$(BUILD_DIR)/$(UNIT_DIR)/userspace/test_webeng: $(UNIT_DIR)/userspace/test_webeng.c $(UNIT_DIR)/userspace/webeng_lot52.inc $(UNIT_DIR)/userspace/webeng_lot53.inc $(UNIT_DIR)/userspace/webeng_lot54.inc $(wildcard $(UNIT_DIR)/userspace/webeng_lot55_*.inc) $(UNIT_DIR)/userspace/webeng_lot56.inc $(UNIT_DIR)/userspace/webeng_lot56b.inc $(wildcard $(UNIT_DIR)/userspace/webeng_lot57_*.inc) $(wildcard $(UNIT_DIR)/userspace/webeng_lot58_*.inc) $(wildcard $(UNIT_DIR)/userspace/webeng_lot59_*.inc) $(wildcard $(UNIT_DIR)/userspace/webeng_lot60_*.inc) $(wildcard $(UNIT_DIR)/userspace/webeng_lot61*.inc) $(wildcard $(UNIT_DIR)/userspace/webeng_lot62*.inc) $(wildcard $(UNIT_DIR)/userspace/webeng_lot63*.inc) $(wildcard $(UNIT_DIR)/userspace/webeng_lot64*.inc) $(wildcard $(UNIT_DIR)/userspace/webeng_lot69*.inc) ../include/os_ui_scale.h ../userspace/webeng.c ../userspace/webeng.h ../userspace/imgdec.c ../userspace/imgdec.h ../userspace/imgdec_jpeg.inc ../include/os_syscalls.h $(FRAMEWORK_SOURCES) $(FRAMEWORK_HEADERS)
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS_USER) -I../userspace -I../include -o $@ $< ../userspace/webeng.c ../userspace/imgdec.c $(FRAMEWORK_SOURCES)
	@echo "Compiled userspace test: $(notdir $@)"

$(BUILD_DIR)/$(UNIT_DIR)/userspace/test_grpcwire: $(UNIT_DIR)/userspace/test_grpcwire.c ../userspace/grpcwire.c ../userspace/grpcwire.h $(FRAMEWORK_SOURCES) $(FRAMEWORK_HEADERS)
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS_USER) -I../userspace -I../include -o $@ $< ../userspace/grpcwire.c $(FRAMEWORK_SOURCES)
	@echo "Compiled userspace test: $(notdir $@)"

$(BUILD_DIR)/$(UNIT_DIR)/userspace/test_dosemu: $(UNIT_DIR)/userspace/test_dosemu.c ../userspace/dosemu.c ../userspace/dosemu.h $(FRAMEWORK_SOURCES) $(FRAMEWORK_HEADERS)
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS_USER) -I../userspace -I../include -o $@ $< ../userspace/dosemu.c $(FRAMEWORK_SOURCES)
	@echo "Compiled userspace test: $(notdir $@)"

$(BUILD_DIR)/$(UNIT_DIR)/userspace/test_cicd: $(UNIT_DIR)/userspace/test_cicd.c ../userspace/cicd.c ../userspace/cicd.h ../userspace/promptmessage.c $(FRAMEWORK_SOURCES) $(FRAMEWORK_HEADERS)
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS_USER) -I../userspace -I../include -o $@ $< ../userspace/cicd.c ../userspace/promptmessage.c $(FRAMEWORK_SOURCES)
	@echo "Compiled userspace test: $(notdir $@)"

$(BUILD_DIR)/$(UNIT_DIR)/userspace/test_webrt: $(UNIT_DIR)/userspace/test_webrt.c ../userspace/webrt.c ../userspace/webrt.h $(FRAMEWORK_SOURCES) $(FRAMEWORK_HEADERS)
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS_USER) -I../userspace -I../include -o $@ $< ../userspace/webrt.c $(FRAMEWORK_SOURCES)
	@echo "Compiled userspace test: $(notdir $@)"

$(BUILD_DIR)/$(UNIT_DIR)/userspace/test_shell_util: $(UNIT_DIR)/userspace/test_shell_util.c ../userspace/shell_util.c ../userspace/shell_util.h $(FRAMEWORK_SOURCES) $(FRAMEWORK_HEADERS)
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS_USER) -I../userspace -o $@ $< ../userspace/shell_util.c $(FRAMEWORK_SOURCES)
	@echo "Compiled userspace test: $(notdir $@)"

$(BUILD_DIR)/$(UNIT_DIR)/userspace/test_stun: $(UNIT_DIR)/userspace/test_stun.c ../userspace/stun.c ../userspace/stun.h $(FRAMEWORK_SOURCES) $(FRAMEWORK_HEADERS)
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS_USER) -I../userspace -o $@ $< ../userspace/stun.c $(FRAMEWORK_SOURCES)
	@echo "Compiled userspace test: $(notdir $@)"

$(BUILD_DIR)/$(UNIT_DIR)/userspace/test_p2p: $(UNIT_DIR)/userspace/test_p2p.c ../userspace/p2p.c ../userspace/p2p_bwe.c ../userspace/p2p_pace.c ../userspace/p2p_flow.c ../userspace/p2p_rtx.c ../userspace/stun.c ../userspace/p2p.h $(FRAMEWORK_SOURCES) $(FRAMEWORK_HEADERS)
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS_USER) -I../userspace -o $@ $< ../userspace/p2p.c ../userspace/p2p_bwe.c ../userspace/p2p_pace.c ../userspace/p2p_flow.c ../userspace/p2p_rtx.c ../userspace/stun.c ../kernel/sha256.c ../kernel/aes_gcm.c ../kernel/x25519.c ../kernel/bigint.c $(FRAMEWORK_SOURCES)
	@echo "Compiled userspace test: $(notdir $@)"

$(BUILD_DIR)/$(UNIT_DIR)/userspace/test_collab: $(UNIT_DIR)/userspace/test_collab.c ../userspace/collab.c ../userspace/collab.h ../userspace/csig.c ../userspace/csig.h $(FRAMEWORK_SOURCES) $(FRAMEWORK_HEADERS)
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS_USER) -I../userspace -o $@ $< ../userspace/collab.c ../userspace/csig.c ../kernel/bigint.c ../kernel/sha256.c ../kernel/aes_gcm.c $(FRAMEWORK_SOURCES)
$(BUILD_DIR)/$(UNIT_DIR)/userspace/test_mobile_ui: $(UNIT_DIR)/userspace/test_mobile_ui.c ../userspace/mobile_ui.c ../userspace/mobile_ui.h ../userspace/platform.c $(FRAMEWORK_SOURCES) $(FRAMEWORK_HEADERS)
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS_USER) -I../userspace -o $@ $< ../userspace/mobile_ui.c ../userspace/platform.c $(FRAMEWORK_SOURCES)
	@echo "Compiled userspace test: $(notdir $@)"

$(BUILD_DIR)/$(UNIT_DIR)/userspace/test_devsync: $(UNIT_DIR)/userspace/test_devsync.c ../userspace/devsync.c ../userspace/devsync.h $(FRAMEWORK_SOURCES) $(FRAMEWORK_HEADERS)
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS_USER) -I../userspace -o $@ $< ../userspace/devsync.c $(FRAMEWORK_SOURCES)
	@echo "Compiled userspace test: $(notdir $@)"

$(BUILD_DIR)/$(UNIT_DIR)/userspace/test_wsearch: $(UNIT_DIR)/userspace/test_wsearch.c ../userspace/wsearch.c ../userspace/wsearch.h $(wildcard fixtures/search/*.html) $(FRAMEWORK_SOURCES) $(FRAMEWORK_HEADERS)
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS_USER) -I../userspace -o $@ $< ../userspace/wsearch.c $(FRAMEWORK_SOURCES)

$(BUILD_DIR)/$(UNIT_DIR)/userspace/test_plat_form_energy: $(UNIT_DIR)/userspace/test_plat_form_energy.c ../userspace/plat_form.c ../userspace/plat_energy.c ../userspace/platform.c ../userspace/plat_form.h ../userspace/plat_energy.h $(FRAMEWORK_SOURCES) $(FRAMEWORK_HEADERS)
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS_USER) -I../userspace -I../include -o $@ $< ../userspace/plat_form.c ../userspace/plat_energy.c ../userspace/platform.c $(FRAMEWORK_SOURCES)
	@echo "Compiled userspace test: $(notdir $@)"

$(BUILD_DIR)/$(UNIT_DIR)/userspace/test_platform: $(UNIT_DIR)/userspace/test_platform.c ../userspace/platform.c ../userspace/platform.h $(FRAMEWORK_SOURCES) $(FRAMEWORK_HEADERS)
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS_USER) -I../userspace -o $@ $< ../userspace/platform.c $(FRAMEWORK_SOURCES)
	@echo "Compiled userspace test: $(notdir $@)"

$(BUILD_DIR)/$(UNIT_DIR)/userspace/test_promptmessage: $(UNIT_DIR)/userspace/test_promptmessage.c ../userspace/promptmessage.c ../userspace/promptmessage.h $(FRAMEWORK_SOURCES) $(FRAMEWORK_HEADERS)
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS_USER) -I../userspace -o $@ $< ../userspace/promptmessage.c $(FRAMEWORK_SOURCES)
	@echo "Compiled userspace test: $(notdir $@)"

$(BUILD_DIR)/$(UNIT_DIR)/userspace/test_prod: $(UNIT_DIR)/userspace/test_prod.c ../userspace/prod.c ../userspace/prod.h $(FRAMEWORK_SOURCES) $(FRAMEWORK_HEADERS)
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS_USER) -I../userspace -o $@ $< ../userspace/prod.c $(FRAMEWORK_SOURCES)
	@echo "Compiled userspace test: $(notdir $@)"

$(BUILD_DIR)/$(PERFORMANCE_DIR)/test_osui_p2p_prod_perf: $(PERFORMANCE_DIR)/test_osui_p2p_prod_perf.c ../userspace/osui_runtime.c ../userspace/webrt.c ../userspace/osui_gui.c ../userspace/wsearch.c ../userspace/osui_tools.c ../userspace/imgdec.c ../userspace/osui_form.c ../userspace/osui_parity.c ../userspace/osui_panes.c ../userspace/osui_files.c ../userspace/osui_app.c ../userspace/fsops.c ../userspace/webeng.c ../userspace/osui_ide.c ../userspace/osui_ide.h ../userspace/promptmessage.c ../userspace/p2p.c ../userspace/p2p_bwe.c ../userspace/p2p_pace.c ../userspace/p2p_flow.c ../userspace/p2p_rtx.c ../userspace/stun.c ../userspace/prod.c ../userspace/platform.c $(FRAMEWORK_SOURCES) $(FRAMEWORK_HEADERS)
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS_USER) -I../userspace -I../include -o $@ $< ../userspace/osui_runtime.c ../userspace/webrt.c ../userspace/osui_gui.c ../userspace/wsearch.c ../userspace/osui_scale.c ../userspace/osui_tools.c ../userspace/imgdec.c ../userspace/osui_form.c ../userspace/osui_parity.c ../userspace/osui_panes.c ../userspace/osui_files.c ../userspace/osui_app.c ../userspace/fsops.c ../userspace/webeng.c ../userspace/osui_ide.c ../userspace/promptmessage.c ../userspace/p2p.c ../userspace/p2p_bwe.c ../userspace/p2p_pace.c ../userspace/p2p_flow.c ../userspace/p2p_rtx.c ../userspace/stun.c ../userspace/prod.c ../userspace/platform.c ../userspace/csig.c ../kernel/sha256.c ../kernel/aes_gcm.c ../kernel/x25519.c ../kernel/bigint.c $(FRAMEWORK_SOURCES)
	@echo "Compiled performance test: $(notdir $@)"

$(BUILD_DIR)/$(UNIT_DIR)/userspace/test_osui_runtime: $(UNIT_DIR)/userspace/test_osui_runtime.c ../userspace/osui_runtime.c ../userspace/webrt.c ../userspace/promptmessage.c ../userspace/platform.c ../userspace/osui_runtime.h ../userspace/mohhdy_osui_bridge.h $(FRAMEWORK_SOURCES) $(FRAMEWORK_HEADERS)
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS_USER) -I../userspace -I../include -o $@ $< ../userspace/osui_runtime.c ../userspace/webrt.c ../userspace/promptmessage.c ../userspace/platform.c ../userspace/csig.c ../kernel/bigint.c ../kernel/sha256.c $(FRAMEWORK_SOURCES)
	@echo "Compiled userspace test: $(notdir $@)"

$(BUILD_DIR)/$(UNIT_DIR)/userspace/test_fsops: $(UNIT_DIR)/userspace/test_fsops.c $(UNIT_DIR)/userspace/mockfs.h ../userspace/fsops.c ../userspace/fsops.h $(FRAMEWORK_SOURCES) $(FRAMEWORK_HEADERS)
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS_USER) -I../userspace -I../include -o $@ $< ../userspace/fsops.c $(FRAMEWORK_SOURCES)

$(BUILD_DIR)/$(UNIT_DIR)/userspace/test_osui_form: $(UNIT_DIR)/userspace/test_osui_form.c ../userspace/osui_form.c ../userspace/osui_form.h ../userspace/fsops.c ../userspace/osui_app.c $(FRAMEWORK_SOURCES) $(FRAMEWORK_HEADERS)
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS_USER) -I../userspace -I../include -o $@ $< ../userspace/osui_form.c ../userspace/fsops.c ../userspace/osui_app.c $(FRAMEWORK_SOURCES)
	@echo "Compiled userspace test: $(notdir $@)"

$(BUILD_DIR)/$(UNIT_DIR)/userspace/test_imgdec: $(UNIT_DIR)/userspace/test_imgdec.c $(UNIT_DIR)/userspace/imgdec_samples.h $(UNIT_DIR)/userspace/imgdec_gif_samples.h ../userspace/imgdec.c ../userspace/imgdec_jpeg.inc ../userspace/imgdec_gif.inc ../userspace/imgdec_ico.inc ../userspace/imgdec.h $(FRAMEWORK_SOURCES) $(FRAMEWORK_HEADERS)
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS_USER) -I../userspace -I../include -o $@ $< ../userspace/imgdec.c $(FRAMEWORK_SOURCES)
	@echo "Compiled userspace test: $(notdir $@)"

$(BUILD_DIR)/$(UNIT_DIR)/userspace/test_osui_tools: $(UNIT_DIR)/userspace/test_osui_tools.c ../userspace/osui_tools.c ../userspace/imgdec.c ../userspace/osui_form.c ../userspace/osui_parity.c ../userspace/osui_tools.h ../userspace/osui_panes.c ../userspace/osui_app.c $(FRAMEWORK_SOURCES) $(FRAMEWORK_HEADERS)
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS_USER) -I../userspace -I../include -o $@ $< ../userspace/osui_tools.c ../userspace/imgdec.c ../userspace/osui_form.c ../userspace/osui_parity.c ../userspace/osui_panes.c ../userspace/osui_app.c $(FRAMEWORK_SOURCES)
	@echo "Compiled userspace test: $(notdir $@)"

$(BUILD_DIR)/$(UNIT_DIR)/userspace/test_osui_parity: $(UNIT_DIR)/userspace/test_osui_parity.c ../userspace/osui_tools.c ../userspace/imgdec.c ../userspace/osui_form.c ../userspace/osui_parity.c ../userspace/osui_parity.h ../userspace/osui_tools.h ../userspace/osui_panes.c ../userspace/osui_app.c ../userspace/mohhdy_osui_bridge.h $(FRAMEWORK_SOURCES) $(FRAMEWORK_HEADERS)
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS_USER) -I../userspace -I../include -o $@ $< ../userspace/osui_tools.c ../userspace/imgdec.c ../userspace/osui_form.c ../userspace/osui_parity.c ../userspace/osui_panes.c ../userspace/osui_app.c $(FRAMEWORK_SOURCES)
	@echo "Compiled userspace test: $(notdir $@)"

$(BUILD_DIR)/$(UNIT_DIR)/userspace/test_osui_files: $(UNIT_DIR)/userspace/test_osui_files.c $(UNIT_DIR)/userspace/mockfs.h ../userspace/osui_files.c ../userspace/osui_files.h ../userspace/osui_app.c ../userspace/fsops.c $(FRAMEWORK_SOURCES) $(FRAMEWORK_HEADERS)
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS_USER) -I../userspace -I../include -o $@ $< ../userspace/osui_files.c ../userspace/osui_app.c ../userspace/fsops.c $(FRAMEWORK_SOURCES)

$(BUILD_DIR)/$(UNIT_DIR)/userspace/test_osui_panes: $(UNIT_DIR)/userspace/test_osui_panes.c ../userspace/osui_panes.c ../userspace/osui_panes.h $(FRAMEWORK_SOURCES) $(FRAMEWORK_HEADERS)
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS_USER) -I../userspace -I../include -o $@ $< ../userspace/osui_panes.c $(FRAMEWORK_SOURCES)
	@echo "Compiled userspace test: $(notdir $@)"

$(BUILD_DIR)/$(UNIT_DIR)/userspace/test_osui_editor: $(UNIT_DIR)/userspace/test_osui_editor.c ../userspace/osui_ide.c ../userspace/osui_ide.h ../userspace/umalloc.c $(FRAMEWORK_SOURCES) $(FRAMEWORK_HEADERS)
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS_USER) -I../userspace -I../include -o $@ $< ../userspace/osui_ide.c ../userspace/umalloc.c $(FRAMEWORK_SOURCES)

$(BUILD_DIR)/$(UNIT_DIR)/userspace/test_umalloc: $(UNIT_DIR)/userspace/test_umalloc.c ../userspace/umalloc.c ../userspace/umalloc.h $(FRAMEWORK_SOURCES) $(FRAMEWORK_HEADERS)
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS_USER) -I../userspace -I../include -o $@ $< ../userspace/umalloc.c $(FRAMEWORK_SOURCES)

$(BUILD_DIR)/$(UNIT_DIR)/kernel/test_user_heap: $(UNIT_DIR)/kernel/test_user_heap.c ../kernel/user_heap.c ../kernel/user_heap.h $(FRAMEWORK_SOURCES) $(FRAMEWORK_HEADERS)
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS_KERNEL) -I.. -o $@ $< ../kernel/user_heap.c $(FRAMEWORK_SOURCES)

# Lot 69 (US-076): portable self-tests shared with the ARM boot
$(BUILD_DIR)/$(UNIT_DIR)/kernel/test_arm_portable: $(UNIT_DIR)/kernel/test_arm_portable.c ../arch/arm/arm_portable.c ../arch/arm/arm_portable.h ../kernel/aes_gcm.c ../kernel/edid.c ../userspace/p2p_flow.c ../userspace/p2p_pace.c ../userspace/p2p_rtx.c $(FRAMEWORK_SOURCES) $(FRAMEWORK_HEADERS)
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS_KERNEL) -I.. -I../kernel -I../userspace -I../include -o $@ $< ../arch/arm/arm_portable.c ../kernel/aes_gcm.c ../kernel/edid.c ../userspace/p2p_flow.c ../userspace/p2p_pace.c ../userspace/p2p_rtx.c $(FRAMEWORK_SOURCES)

$(BUILD_DIR)/$(UNIT_DIR)/kernel/test_edid: $(UNIT_DIR)/kernel/test_edid.c ../kernel/edid.c ../kernel/edid.h $(FRAMEWORK_SOURCES) $(FRAMEWORK_HEADERS)
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS_KERNEL) -I.. -o $@ $< ../kernel/edid.c $(FRAMEWORK_SOURCES)

$(BUILD_DIR)/$(UNIT_DIR)/kernel/test_out_capture: $(UNIT_DIR)/kernel/test_out_capture.c ../kernel/out_capture.c ../kernel/out_capture.h $(FRAMEWORK_SOURCES) $(FRAMEWORK_HEADERS)
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS_KERNEL) -I.. -o $@ $< ../kernel/out_capture.c $(FRAMEWORK_SOURCES)

$(BUILD_DIR)/$(UNIT_DIR)/userspace/test_osui_ide: $(UNIT_DIR)/userspace/test_osui_ide.c ../userspace/osui_ide.c ../userspace/osui_ide.h $(FRAMEWORK_SOURCES) $(FRAMEWORK_HEADERS)
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS_USER) -I../userspace -I../include -o $@ $< ../userspace/osui_ide.c $(FRAMEWORK_SOURCES)
	@echo "Compiled userspace test: $(notdir $@)"

$(BUILD_DIR)/$(UNIT_DIR)/userspace/test_osui_gui: $(UNIT_DIR)/userspace/test_osui_gui.c $(UNIT_DIR)/userspace/mockfs.h ../userspace/osui_gui.c ../userspace/wsearch.c ../userspace/osui_tools.c ../userspace/imgdec.c ../userspace/osui_form.c ../userspace/osui_parity.c ../userspace/umalloc.c ../userspace/osui_panes.c ../userspace/osui_files.c ../userspace/osui_app.c ../userspace/fsops.c ../userspace/webeng.c ../userspace/osui_ide.c ../userspace/osui_ide.h ../userspace/osui_runtime.c ../userspace/webrt.c ../userspace/promptmessage.c ../userspace/platform.c ../userspace/osui_gui.h ../userspace/osui_runtime.h ../userspace/mohhdy_osui_bridge.h $(FRAMEWORK_SOURCES) $(FRAMEWORK_HEADERS)
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS_USER) -I../userspace -I../include -o $@ $< ../userspace/osui_gui.c ../userspace/wsearch.c ../userspace/osui_scale.c ../userspace/osui_tools.c ../userspace/imgdec.c ../userspace/osui_form.c ../userspace/osui_parity.c ../userspace/umalloc.c ../userspace/osui_panes.c ../userspace/osui_files.c ../userspace/osui_app.c ../userspace/fsops.c ../userspace/webeng.c ../userspace/osui_ide.c ../userspace/osui_runtime.c ../userspace/webrt.c ../userspace/promptmessage.c ../userspace/platform.c ../userspace/csig.c ../kernel/bigint.c ../kernel/sha256.c $(FRAMEWORK_SOURCES)
	@echo "Compiled userspace test: $(notdir $@)"

$(BUILD_DIR)/$(UNIT_DIR)/userspace/test_%: $(UNIT_DIR)/userspace/test_%.c $(FRAMEWORK_SOURCES) $(FRAMEWORK_HEADERS)
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS_USER) -o $@ $< $(FRAMEWORK_SOURCES)
	@echo "Compiled userspace test: $(notdir $@)"

# Compilation des tests filesystem
$(BUILD_DIR)/$(UNIT_DIR)/fs/test_%: $(UNIT_DIR)/fs/test_%.c $(FRAMEWORK_SOURCES) $(FRAMEWORK_HEADERS)
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS_KERNEL) -o $@ $< $(FRAMEWORK_SOURCES)
	@echo "Compiled filesystem test: $(notdir $@)"

# Compilation des tests d'intégration
$(BUILD_DIR)/$(INTEGRATION_DIR)/test_%: $(INTEGRATION_DIR)/test_%.c $(FRAMEWORK_SOURCES) $(FRAMEWORK_HEADERS)
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS_KERNEL) -o $@ $< $(FRAMEWORK_SOURCES)
	@echo "Compiled integration test: $(notdir $@)"

# Compilation des tests système
$(BUILD_DIR)/$(SYSTEM_DIR)/test_%: $(SYSTEM_DIR)/test_%.c $(FRAMEWORK_SOURCES) $(FRAMEWORK_HEADERS)
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS_KERNEL) -o $@ $< $(FRAMEWORK_SOURCES)
	@echo "Compiled system test: $(notdir $@)"

# Compilation des tests de performance
$(BUILD_DIR)/$(PERFORMANCE_DIR)/test_%: $(PERFORMANCE_DIR)/test_%.c $(FRAMEWORK_SOURCES) $(FRAMEWORK_HEADERS)
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS_KERNEL) -o $@ $< $(FRAMEWORK_SOURCES)
	@echo "Compiled performance test: $(notdir $@)"

# Compilation des tests de robustesse
$(BUILD_DIR)/$(ROBUSTNESS_DIR)/test_%: $(ROBUSTNESS_DIR)/test_%.c $(FRAMEWORK_SOURCES) $(FRAMEWORK_HEADERS)
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS_KERNEL) -o $@ $< $(FRAMEWORK_SOURCES)
	@echo "Compiled robustness test: $(notdir $@)"

# Tests individuels
run-test-%: $(BUILD_DIR)/*/test_%
	@echo "Running individual test: $*"
	@$<

# Test avec Valgrind (si disponible)
test-valgrind: $(USERSPACE_TEST_BINS)
	@echo "Running tests with Valgrind..."
	@for test in $(USERSPACE_TEST_BINS); do \
		echo "Valgrind: $$test"; \
		valgrind --leak-check=full --show-leak-kinds=all $$test || true; \
	done

# Génération de rapports
coverage-report:
	@echo "Generating coverage report..."
	@mkdir -p $(LOG_DIR)
	@if [ "$(ENABLE_COVERAGE)" = "1" ]; then \
		python3 ../scripts/gcov_host_summary.py > $(LOG_DIR)/coverage_report.txt; rc=$$?; \
		cat $(LOG_DIR)/coverage_report.txt; exit $$rc; \
	else \
		echo "Coverage: not measured (no gcov instrumentation by default). Rebuild with ENABLE_COVERAGE=1." > $(LOG_DIR)/coverage_report.txt; \
		cat $(LOG_DIR)/coverage_report.txt; \
	fi

test-report:
	@echo "Generating test report..."
	@mkdir -p $(LOG_DIR)
	@./scripts/generate_test_report.sh > $(LOG_DIR)/test_summary.html

# Benchmarks
benchmark: $(PERFORMANCE_TEST_BINS)
	@echo "Running performance benchmarks..."
	@mkdir -p $(LOG_DIR)
	@echo "Benchmark Results - $(shell date)" > $(LOG_DIR)/benchmark_results.txt
	@for test in $(PERFORMANCE_TEST_BINS); do \
		echo "Running benchmark: $$test"; \
		$$test >> $(LOG_DIR)/benchmark_results.txt 2>&1 || true; \
	done
	@echo "Benchmark results saved to $(LOG_DIR)/benchmark_results.txt"

# Installation et setup
setup:
	@echo "Setting up test environment..."
	@mkdir -p $(BUILD_DIR) $(LOG_DIR)
	@chmod +x scripts/*.sh
	@echo "Test environment ready"

install-deps:
	@echo "Installing test dependencies..."
	@sudo apt-get update
	@sudo apt-get install -y build-essential gcc-multilib valgrind
	@echo "Dependencies installed"

# Nettoyage
clean:
	@echo "Cleaning test binaries and logs..."
	@rm -rf $(BUILD_DIR)/unit $(BUILD_DIR)/integration $(BUILD_DIR)/system $(BUILD_DIR)/performance $(BUILD_DIR)/robustness
	@rm -f $(BUILD_DIR)/test_* $(BUILD_DIR)/*.gcda $(BUILD_DIR)/*.gcno
	@rm -f $(LOG_DIR)/test_results_*.log
	@echo "Clean completed"

clean-all: clean
	@echo "Cleaning all generated files..."
	@rm -rf $(BUILD_DIR) $(LOG_DIR)
	@echo "Complete clean finished"

# Debug et développement
debug-test-%: $(BUILD_DIR)/*/test_%
	@echo "Running test $* with debug output..."
	@gdb -batch -ex run -ex bt --args $<

list-tests:
	@echo "Available tests:"
	@echo "Kernel Tests:"
	@for test in $(KERNEL_TESTS); do echo "  - $$(basename $$test .c)"; done
	@echo "Userspace Tests:"
	@for test in $(USERSPACE_TESTS); do echo "  - $$(basename $$test .c)"; done
	@echo "Integration Tests:"
	@for test in $(INTEGRATION_TESTS); do echo "  - $$(basename $$test .c)"; done
	@echo "Performance Tests:"
	@for test in $(PERFORMANCE_TESTS); do echo "  - $$(basename $$test .c)"; done

check-deps:
	@echo "Checking dependencies..."
	@which gcc >/dev/null || (echo "ERROR: gcc not found" && exit 1)
	@which gcc-multilib >/dev/null 2>&1 || echo "WARNING: gcc-multilib not found (needed for 32-bit compilation)"
	@which valgrind >/dev/null 2>&1 || echo "INFO: valgrind not found (optional for memory testing)"
	@echo "Dependencies check completed"

# Intégration continue
ci-test: setup clean all test
	@echo "Continuous Integration test completed"

# Aide
help:
	@echo "MOHHDY Test Framework - Available targets:"
	@echo ""
	@echo "Building:"
	@echo "  all              - Compile all test binaries"
	@echo "  setup            - Setup test environment"
	@echo "  install-deps     - Install required dependencies"
	@echo ""
	@echo "Testing:"
	@echo "  test             - Run complete test suite"
	@echo "  test-quick       - Run critical tests only"
	@echo "  test-kernel      - Run kernel tests only"
	@echo "  test-userspace   - Run userspace tests only"
	@echo "  test-integration - Run integration tests only"
	@echo "  test-performance - Run performance tests only"
	@echo "  test-robustness  - Run robustness tests only"
	@echo "  test-valgrind    - Run tests with Valgrind"
	@echo ""
	@echo "Individual tests:"
	@echo "  run-test-<name>  - Run specific test"
	@echo "  debug-test-<name>- Debug specific test with GDB"
	@echo ""
	@echo "Reports:"
	@echo "  coverage-report  - Generate coverage report"
	@echo "  test-report      - Generate HTML test report"
	@echo "  benchmark        - Run performance benchmarks"
	@echo ""
	@echo "Utilities:"
	@echo "  list-tests       - List all available tests"
	@echo "  check-deps       - Check dependencies"
	@echo "  clean            - Clean test binaries"
	@echo "  clean-all        - Clean all generated files"
	@echo "  ci-test          - Continuous integration test"
	@echo "  help             - Show this help"
	@echo ""
	@echo "Examples:"
	@echo "  make test                    # Run all tests"
	@echo "  make test-kernel             # Run only kernel tests"
	@echo "  make run-test-pmm           # Run PMM test specifically"
	@echo "  make debug-test-task        # Debug task test with GDB"

# Validation des fichiers
validate:
	@echo "Validating test files..."
	@for test_file in $(KERNEL_TESTS) $(USERSPACE_TESTS) $(FS_TESTS); do \
		echo "Checking syntax: $$test_file"; \
		$(CC) $(CFLAGS_BASE) -fsyntax-only $$test_file 2>/dev/null || \
		echo "WARNING: Syntax issues in $$test_file"; \
	done
	@echo "Validation completed"

# Statistiques
stats:
	@echo "Test Suite Statistics:"
	@echo "  Kernel tests: $(words $(KERNEL_TESTS))"
	@echo "  Userspace tests: $(words $(USERSPACE_TESTS))"
	@echo "  Filesystem tests: $(words $(FS_TESTS))"
	@echo "  Integration tests: $(words $(INTEGRATION_TESTS))"
	@echo "  System tests: $(words $(SYSTEM_TESTS))"
	@echo "  Performance tests: $(words $(PERFORMANCE_TESTS))"
	@echo "  Robustness tests: $(words $(ROBUSTNESS_TESTS))"
	@echo "  Total tests: $(words $(ALL_TEST_BINS))"
	@echo ""
	@echo "Framework files:"
	@echo "  Sources: $(words $(FRAMEWORK_SOURCES))"
	@echo "  Headers: $(words $(FRAMEWORK_HEADERS))"

# Règles de dépendance
$(BUILD_DIR):
	@mkdir -p $(BUILD_DIR)

$(LOG_DIR):
	@mkdir -p $(LOG_DIR)

# S'assurer que les répertoires existent
$(ALL_TEST_BINS): | $(BUILD_DIR) $(LOG_DIR)

# Empêcher la suppression des fichiers intermédiaires
.PRECIOUS: $(ALL_TEST_BINS)

# Marquer les cibles qui ne créent pas de fichiers
.PHONY: $(filter-out $(ALL_TEST_BINS), $(MAKECMDGOALS))

# Lot 47 (US-G04): net-if command, static lease validation, rights predicate
$(BUILD_DIR)/$(UNIT_DIR)/userspace/test_net_ifcfg: $(UNIT_DIR)/userspace/test_net_ifcfg.c ../userspace/net_ifcfg.c ../userspace/net_ifcfg.h ../kernel/net_dhcp.c ../kernel/net_dhcp.h ../kernel/net_relay.h $(FRAMEWORK_SOURCES) $(FRAMEWORK_HEADERS)
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS_USER) -I../userspace -I../include -o $@ $< ../userspace/net_ifcfg.c ../kernel/net_dhcp.c $(FRAMEWORK_SOURCES)
	@echo "Compiled userspace test: $(notdir $@)"

# Lot 67: Ring 3 web stack (crypto, X.509, TLS 1.2 client, HTTP/1.1).
WF_CRYPTO_SRCS = ../userspace/wf_bn.c ../userspace/wf_sha512.c ../userspace/wf_ec.c ../userspace/wf_p256.c ../userspace/wf_aead.c ../userspace/wf_rsa.c ../kernel/sha256.c
$(BUILD_DIR)/$(UNIT_DIR)/userspace/test_wf_crypto: $(UNIT_DIR)/userspace/test_wf_crypto.c $(UNIT_DIR)/userspace/wf_vectors.h $(WF_CRYPTO_SRCS) $(wildcard ../userspace/wf_*.h) $(FRAMEWORK_SOURCES) $(FRAMEWORK_HEADERS)
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS_USER) -I../userspace -o $@ $< $(WF_CRYPTO_SRCS) $(FRAMEWORK_SOURCES)
	@echo "Compiled userspace test: $(notdir $@)"

# Lot 67: X.509 / chain / root store
WF_X509_SRCS = ../userspace/wf_x509.c $(WF_CRYPTO_SRCS)
$(BUILD_DIR)/$(UNIT_DIR)/userspace/test_wf_x509: $(UNIT_DIR)/userspace/test_wf_x509.c $(UNIT_DIR)/userspace/wf_x509_fixtures.h $(UNIT_DIR)/userspace/wf_real_chain.h $(WF_X509_SRCS) $(wildcard ../userspace/wf_*.h) $(FRAMEWORK_SOURCES) $(FRAMEWORK_HEADERS)
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS_USER) -I../userspace -o $@ $< $(WF_X509_SRCS) $(FRAMEWORK_SOURCES)

# Lot 67 (US-068/US-073): bandwidth estimator against a simulated link
$(BUILD_DIR)/$(UNIT_DIR)/userspace/test_p2p_pace: $(UNIT_DIR)/userspace/test_p2p_pace.c ../userspace/p2p_pace.c ../userspace/p2p_pace.h ../userspace/p2p_bwe.h $(FRAMEWORK_SOURCES) $(FRAMEWORK_HEADERS)
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS_USER) -I../userspace -o $@ $< ../userspace/p2p_pace.c $(FRAMEWORK_SOURCES)
	@echo "Compiled userspace test: $(notdir $@)"

# Lot 69 (US-068/US-073): per-peer flow and data acks
$(BUILD_DIR)/$(UNIT_DIR)/userspace/test_p2p_flow: $(UNIT_DIR)/userspace/test_p2p_flow.c ../userspace/p2p_flow.c ../userspace/p2p_flow.h ../userspace/p2p_pace.c ../userspace/p2p_pace.h $(FRAMEWORK_SOURCES) $(FRAMEWORK_HEADERS)
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS_USER) -I../userspace -o $@ $< ../userspace/p2p_flow.c ../userspace/p2p_pace.c $(FRAMEWORK_SOURCES)
	@echo "Compiled userspace test: $(notdir $@)"

# Lot 71: cookie jar
$(BUILD_DIR)/$(UNIT_DIR)/userspace/test_wf_cookie: $(UNIT_DIR)/userspace/test_wf_cookie.c ../userspace/wf_cookie.c ../userspace/wf_cookie.h $(FRAMEWORK_SOURCES) $(FRAMEWORK_HEADERS)
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS_USER) -I../userspace -o $@ $< ../userspace/wf_cookie.c $(FRAMEWORK_SOURCES)
	@echo "Compiled userspace test: $(notdir $@)"

# Lot 71 (US-068/US-073): retransmission of lost user data
$(BUILD_DIR)/$(UNIT_DIR)/userspace/test_p2p_rtx: $(UNIT_DIR)/userspace/test_p2p_rtx.c ../userspace/p2p_rtx.c ../userspace/p2p_rtx.h $(FRAMEWORK_SOURCES) $(FRAMEWORK_HEADERS)
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS_USER) -I../userspace -o $@ $< ../userspace/p2p_rtx.c $(FRAMEWORK_SOURCES)
	@echo "Compiled userspace test: $(notdir $@)"

$(BUILD_DIR)/$(UNIT_DIR)/userspace/test_p2p_bwe: $(UNIT_DIR)/userspace/test_p2p_bwe.c ../userspace/p2p_bwe.c ../userspace/p2p_pace.c ../userspace/p2p_bwe.h $(FRAMEWORK_SOURCES) $(FRAMEWORK_HEADERS)
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS_USER) -I../userspace -o $@ $< ../userspace/p2p_bwe.c ../userspace/p2p_pace.c $(FRAMEWORK_SOURCES)

# Lot 71: display charset, entities, forms, sections, search engines
$(BUILD_DIR)/$(UNIT_DIR)/userspace/test_wf_text: $(UNIT_DIR)/userspace/test_wf_text.c $(WF_NET_SRCS) ../include/os_charset.h $(wildcard ../userspace/wf_*.h) $(FRAMEWORK_SOURCES) $(FRAMEWORK_HEADERS)
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS_USER) -I../userspace -o $@ $< $(WF_NET_SRCS) $(FRAMEWORK_SOURCES)
$(BUILD_DIR)/$(UNIT_DIR)/userspace/test_wf_form: $(UNIT_DIR)/userspace/test_wf_form.c $(WF_NET_SRCS) ../userspace/wf_section.c $(wildcard ../userspace/wf_*.h) $(FRAMEWORK_SOURCES) $(FRAMEWORK_HEADERS)
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS_USER) -I../userspace -o $@ $< $(WF_NET_SRCS) ../userspace/wf_section.c $(FRAMEWORK_SOURCES)
$(BUILD_DIR)/$(UNIT_DIR)/userspace/test_wf_section: $(UNIT_DIR)/userspace/test_wf_section.c $(WF_NET_SRCS) ../userspace/wf_section.c $(wildcard ../userspace/wf_*.h) $(FRAMEWORK_SOURCES) $(FRAMEWORK_HEADERS)
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS_USER) -I../userspace -o $@ $< $(WF_NET_SRCS) ../userspace/wf_section.c $(FRAMEWORK_SOURCES)
$(BUILD_DIR)/$(UNIT_DIR)/userspace/test_wsearch_api: $(UNIT_DIR)/userspace/test_wsearch_api.c ../userspace/wsearch_api.c ../userspace/wsearch.c ../userspace/wf_entity.c ../userspace/wsearch_api.h $(wildcard fixtures/search/*) $(FRAMEWORK_SOURCES) $(FRAMEWORK_HEADERS)
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS_USER) -I../userspace -o $@ $< ../userspace/wsearch_api.c ../userspace/wsearch.c ../userspace/wf_entity.c $(FRAMEWORK_SOURCES)

# Lot 72: browser images (wf_html <img>, wf_webimg atlas, engine atlas item)
$(BUILD_DIR)/$(UNIT_DIR)/userspace/test_wf_p256: $(UNIT_DIR)/userspace/test_wf_p256.c $(WF_CRYPTO_SRCS) $(wildcard ../userspace/wf_*.h) $(FRAMEWORK_SOURCES) $(FRAMEWORK_HEADERS)
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS_USER) -I../userspace -o $@ $< $(WF_CRYPTO_SRCS) $(FRAMEWORK_SOURCES)

$(BUILD_DIR)/$(UNIT_DIR)/userspace/test_wf_aead: $(UNIT_DIR)/userspace/test_wf_aead.c ../userspace/wf_aead.c ../userspace/wf_aead.h $(UNIT_DIR)/userspace/wf_aead_vectors.h $(FRAMEWORK_SOURCES) $(FRAMEWORK_HEADERS)
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS_USER) -I../userspace -I../include -o $@ $< ../userspace/wf_aead.c $(FRAMEWORK_SOURCES)

$(BUILD_DIR)/$(UNIT_DIR)/userspace/test_wf_webimg: $(UNIT_DIR)/userspace/test_wf_webimg.c $(WF_NET_SRCS) ../userspace/wf_webimg.c ../userspace/webeng.c ../userspace/imgdec.c ../userspace/imgdec_gif.inc ../userspace/imgdec_ico.inc ../userspace/imgdec_jpeg.inc $(wildcard ../userspace/wf_*.h) ../userspace/webeng.h $(FRAMEWORK_SOURCES) $(FRAMEWORK_HEADERS)
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS_USER) -I../userspace -I../include -o $@ $< $(WF_NET_SRCS) ../userspace/wf_webimg.c ../userspace/webeng.c ../userspace/imgdec.c $(FRAMEWORK_SOURCES)

# Lot 67: URL / HTTP / DNS / TLS PRF / fetch
WF_NET_SRCS = ../userspace/wf_html.c ../userspace/wf_entity.c ../userspace/wf_form.c ../userspace/wf_http.c ../userspace/wf_dns.c ../userspace/wf_tls.c ../userspace/wf_fetch.c ../userspace/wf_cookie.c ../userspace/wf_inflate.c ../kernel/aes_gcm.c ../kernel/x25519.c $(WF_X509_SRCS)
$(BUILD_DIR)/$(UNIT_DIR)/userspace/test_wf_http: $(UNIT_DIR)/userspace/test_wf_http.c $(WF_NET_SRCS) $(wildcard ../userspace/wf_*.h) $(FRAMEWORK_SOURCES) $(FRAMEWORK_HEADERS)
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS_USER) -I../userspace -o $@ $< $(WF_NET_SRCS) $(FRAMEWORK_SOURCES)
