#!/bin/bash

# run_all_tests.sh - Script principal pour exécuter tous les tests MOHHDY
# Ce script exécute la suite complète de tests de non-régression

set -e  # Exit on any error

# Configuration
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BASE_DIR="$(cd "$SCRIPT_DIR/../.." && pwd)"
TEST_DIR="$BASE_DIR/tests"
BUILD_DIR="$BASE_DIR/build"
LOG_DIR="$BASE_DIR/test_logs"
RESULTS_FILE="$LOG_DIR/test_results_$(date +%Y%m%d_%H%M%S).log"

# Couleurs pour l'affichage
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[0;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Compteurs de résultats
TOTAL_TESTS=0
PASSED_TESTS=0
FAILED_TESTS=0
SKIPPED_TESTS=0

# Créer les répertoires nécessaires avant toute opération
mkdir -p "$LOG_DIR"
mkdir -p "$BUILD_DIR"

echo -e "${BLUE}=================================${NC}"
echo -e "${BLUE} MOHHDY Test Suite Runner v1.0   ${NC}"
echo -e "${BLUE}=================================${NC}"
echo ""

# Fonction d'affichage des résultats
print_header() {
    echo -e "${BLUE}--- $1 ---${NC}"
}

print_success() {
    echo -e "${GREEN}✓ $1${NC}"
}

print_error() {
    echo -e "${RED}✗ $1${NC}"
}

print_warning() {
    echo -e "${YELLOW}⚠ $1${NC}"
}

add_unity_counts() {
    local output=$1
    local fallback=$2
    local run_n pass_n fail_n
    run_n=$(printf '%s\n' "$output" | awk '/^Tests Run:/ {print $3; exit}')
    pass_n=$(printf '%s\n' "$output" | awk '/^Tests Passed:/ {print $3; exit}')
    fail_n=$(printf '%s\n' "$output" | awk '/^Tests Failed:/ {print $3; exit}')
    if [ -n "$run_n" ]; then
        TOTAL_TESTS=$((TOTAL_TESTS + run_n))
        PASSED_TESTS=$((PASSED_TESTS + ${pass_n:-0}))
        FAILED_TESTS=$((FAILED_TESTS + ${fail_n:-0}))
    else
        TOTAL_TESTS=$((TOTAL_TESTS + 1))
        if [ "$fallback" = "pass" ]; then
            PASSED_TESTS=$((PASSED_TESTS + 1))
        else
            FAILED_TESTS=$((FAILED_TESTS + 1))
        fi
    fi
}

# Fonction pour compiler et exécuter un test
run_test() {
    local test_file=$1
    local test_name=$2
    local test_type=$3
    
    echo "DEBUG: run_test $test_name" >> "$RESULTS_FILE"
    echo -n "Running $test_name... "
    
    # Compiler le test
    local test_binary="$BUILD_DIR/$(basename $test_file .c)"
    
    # Flags de compilation spécialisés selon le type de test
    local cflags="-I$TEST_DIR -I$BASE_DIR -I$BASE_DIR/kernel -I$BASE_DIR/include -Wall -Wextra -std=c99 -DKERNEL_TEST=1"
    local extra_src="$BASE_DIR/fs/overlay.c"
    # make coverage exports ENABLE_COVERAGE=1. These are the binaries this
    # runner actually executes (it recompiles; the Makefile copies are not).
    if [ "${ENABLE_COVERAGE:-0}" = "1" ]; then
        cflags="$cflags --coverage"
    fi
    
    if [ "$test_type" == "kernel" ] || [ "$(basename "$test_file")" = "test_fat16.c" ] || [ "$(basename "$test_file")" = "test_gpt2_gguf.c" ] || [ "$(basename "$test_file")" = "test_gpt2_gguf_infer.c" ] || [ "$(basename "$test_file")" = "test_gpt2_quant.c" ]; then
        cflags="$cflags -m32 -fno-pie -msse2 -mfpmath=sse"
        if [ "$test_type" == "kernel" ] && [ "$(basename "$test_file")" != "test_net_wire.c" ] && [ "$(basename "$test_file")" != "test_net_stack_exec.c" ]; then
            cflags="$cflags -ffreestanding -nostdlib"
        fi
    else
        cflags="$cflags -m32"
    fi

    if [ "$(basename "$test_file")" = "test_ramfs.c" ]; then
        extra_src="$extra_src $BASE_DIR/userspace/ramfs.c $BASE_DIR/userspace/procsim.c"
        cflags="$cflags -I$BASE_DIR/userspace"
    fi
    if [ "$(basename "$test_file")" = "test_net_p2p.c" ]; then
        extra_src="$extra_src $BASE_DIR/kernel/net_p2p.c $BASE_DIR/kernel/net_ipv4_udp.c"
    fi
    if [ "$(basename "$test_file")" = "test_collab.c" ]; then
        extra_src="$extra_src $BASE_DIR/userspace/collab.c $BASE_DIR/userspace/csig.c $BASE_DIR/kernel/bigint.c $BASE_DIR/kernel/sha256.c $BASE_DIR/kernel/aes_gcm.c"
    fi
    if [ "$(basename "$test_file")" = "test_grpcwire.c" ]; then
        extra_src="$extra_src $BASE_DIR/userspace/grpcwire.c"
    fi
    if [ "$(basename "$test_file")" = "test_cicd.c" ]; then
        extra_src="$extra_src $BASE_DIR/userspace/cicd.c $BASE_DIR/userspace/promptmessage.c"
    fi
    if [ "$(basename "$test_file")" = "test_dosemu.c" ]; then
        extra_src="$extra_src $BASE_DIR/userspace/dosemu.c"
    fi
    if [ "$(basename "$test_file")" = "test_webeng.c" ]; then
        extra_src="$extra_src $BASE_DIR/userspace/webeng.c $BASE_DIR/userspace/imgdec.c"
    fi
    if [ "$(basename "$test_file")" = "test_key_seal.c" ]; then
        extra_src="$extra_src $BASE_DIR/kernel/key_seal.c $BASE_DIR/kernel/aes_gcm.c $BASE_DIR/kernel/sha256.c"
    fi
    if [ "$(basename "$test_file")" = "test_shell_util.c" ]; then
        extra_src="$extra_src $BASE_DIR/userspace/shell_util.c"
    fi
    if [ "$(basename "$test_file")" = "test_webrt.c" ]; then
        extra_src="$extra_src $BASE_DIR/userspace/webrt.c"
    fi
    if [ "$(basename "$test_file")" = "test_ui_scale.c" ]; then
        extra_src="$extra_src $BASE_DIR/userspace/osui_scale.c"
    fi
    if [ "$(basename "$test_file")" = "test_ps2_decode.c" ]; then
        extra_src="$extra_src $BASE_DIR/userspace/ps2_decode.c"
    fi
    if [ "$(basename "$test_file")" = "test_stun.c" ]; then
        extra_src="$extra_src $BASE_DIR/userspace/stun.c"
    fi
    if [ "$(basename "$test_file")" = "test_p2p.c" ]; then
        extra_src="$extra_src $BASE_DIR/userspace/p2p.c $BASE_DIR/userspace/p2p_bwe.c $BASE_DIR/userspace/p2p_pace.c $BASE_DIR/userspace/p2p_flow.c $BASE_DIR/userspace/p2p_rtx.c $BASE_DIR/userspace/stun.c $BASE_DIR/kernel/sha256.c $BASE_DIR/kernel/aes_gcm.c $BASE_DIR/kernel/x25519.c $BASE_DIR/kernel/bigint.c"
    fi
    if [ "$(basename "$test_file")" = "test_mobile_ui.c" ]; then
        extra_src="$extra_src $BASE_DIR/userspace/mobile_ui.c $BASE_DIR/userspace/platform.c"
    fi
    if [ "$(basename "$test_file")" = "test_devsync.c" ]; then
        extra_src="$extra_src $BASE_DIR/userspace/devsync.c"
    fi
    if [ "$(basename "$test_file")" = "test_plat_form_energy.c" ]; then
        extra_src="$extra_src $BASE_DIR/userspace/plat_form.c $BASE_DIR/userspace/plat_energy.c $BASE_DIR/userspace/platform.c"
    fi
    if [ "$(basename "$test_file")" = "test_platform.c" ]; then
        extra_src="$extra_src $BASE_DIR/userspace/platform.c"
    fi
    if [ "$(basename "$test_file")" = "test_promptmessage.c" ]; then
        extra_src="$extra_src $BASE_DIR/userspace/promptmessage.c"
    fi
    if [ "$(basename "$test_file")" = "test_prod.c" ]; then
        extra_src="$extra_src $BASE_DIR/userspace/prod.c"
    fi
    if [ "$(basename "$test_file")" = "test_osui_form.c" ]; then
        extra_src="$extra_src $BASE_DIR/userspace/osui_form.c $BASE_DIR/userspace/fsops.c $BASE_DIR/userspace/osui_app.c"
        cflags="$cflags -I$BASE_DIR/userspace"
    fi
    if [ "$(basename "$test_file")" = "test_osui_p2p_prod_perf.c" ]; then
        extra_src="$extra_src $BASE_DIR/userspace/osui_runtime.c $BASE_DIR/userspace/webrt.c $BASE_DIR/userspace/osui_gui.c $BASE_DIR/userspace/wsearch.c $BASE_DIR/userspace/osui_scale.c $BASE_DIR/userspace/osui_tools.c $BASE_DIR/userspace/imgdec.c $BASE_DIR/userspace/osui_form.c $BASE_DIR/userspace/osui_parity.c $BASE_DIR/userspace/osui_panes.c $BASE_DIR/userspace/osui_files.c $BASE_DIR/userspace/osui_app.c $BASE_DIR/userspace/fsops.c $BASE_DIR/userspace/webeng.c $BASE_DIR/userspace/osui_ide.c $BASE_DIR/userspace/promptmessage.c $BASE_DIR/userspace/p2p.c $BASE_DIR/userspace/p2p_bwe.c $BASE_DIR/userspace/p2p_pace.c $BASE_DIR/userspace/p2p_flow.c $BASE_DIR/userspace/p2p_rtx.c $BASE_DIR/userspace/stun.c $BASE_DIR/userspace/prod.c $BASE_DIR/userspace/platform.c $BASE_DIR/userspace/csig.c $BASE_DIR/kernel/sha256.c $BASE_DIR/kernel/aes_gcm.c $BASE_DIR/kernel/x25519.c $BASE_DIR/kernel/bigint.c"
        cflags="$cflags -I$BASE_DIR/userspace -I$BASE_DIR/include -DMOHHDY_OSUI_HOST_TEST=1"
    fi
    if [ "$(basename "$test_file")" = "test_osui_runtime.c" ]; then
        extra_src="$extra_src $BASE_DIR/userspace/osui_runtime.c $BASE_DIR/userspace/webrt.c $BASE_DIR/userspace/promptmessage.c $BASE_DIR/userspace/platform.c $BASE_DIR/userspace/csig.c $BASE_DIR/kernel/bigint.c $BASE_DIR/kernel/sha256.c"
        cflags="$cflags -I$BASE_DIR/userspace -DMOHHDY_OSUI_HOST_TEST=1"
    fi
    if [ "$(basename "$test_file")" = "test_osui_gui.c" ]; then
        extra_src="$extra_src $BASE_DIR/userspace/osui_gui.c $BASE_DIR/userspace/wsearch.c $BASE_DIR/userspace/osui_scale.c $BASE_DIR/userspace/osui_tools.c $BASE_DIR/userspace/imgdec.c $BASE_DIR/userspace/osui_form.c $BASE_DIR/userspace/osui_parity.c $BASE_DIR/userspace/umalloc.c $BASE_DIR/userspace/osui_panes.c $BASE_DIR/userspace/osui_files.c $BASE_DIR/userspace/osui_app.c $BASE_DIR/userspace/fsops.c $BASE_DIR/userspace/webeng.c $BASE_DIR/userspace/osui_ide.c $BASE_DIR/userspace/osui_runtime.c $BASE_DIR/userspace/webrt.c $BASE_DIR/userspace/promptmessage.c $BASE_DIR/userspace/platform.c $BASE_DIR/userspace/csig.c $BASE_DIR/kernel/bigint.c $BASE_DIR/kernel/sha256.c"
        cflags="$cflags -I$BASE_DIR/userspace -DMOHHDY_OSUI_HOST_TEST=1"
    fi
    if [ "$(basename "$test_file")" = "test_osui_files.c" ]; then
        extra_src="$extra_src $BASE_DIR/userspace/osui_files.c $BASE_DIR/userspace/osui_app.c $BASE_DIR/userspace/fsops.c"
        cflags="$cflags -I$BASE_DIR/userspace"
    fi
    if [ "$(basename "$test_file")" = "test_imgdec.c" ]; then
        extra_src="$extra_src $BASE_DIR/userspace/imgdec.c"
        cflags="$cflags -I$BASE_DIR/userspace -I$TEST_DIR/unit/userspace"
    fi
    if [ "$(basename "$test_file")" = "test_osui_tools.c" ]; then
        extra_src="$extra_src $BASE_DIR/userspace/osui_tools.c $BASE_DIR/userspace/imgdec.c $BASE_DIR/userspace/osui_form.c $BASE_DIR/userspace/osui_parity.c $BASE_DIR/userspace/osui_panes.c $BASE_DIR/userspace/osui_app.c"
        cflags="$cflags -I$BASE_DIR/userspace"
    fi
    if [ "$(basename "$test_file")" = "test_osui_parity.c" ]; then
        extra_src="$extra_src $BASE_DIR/userspace/osui_tools.c $BASE_DIR/userspace/imgdec.c $BASE_DIR/userspace/osui_form.c $BASE_DIR/userspace/osui_parity.c $BASE_DIR/userspace/osui_panes.c $BASE_DIR/userspace/osui_app.c"
        cflags="$cflags -I$BASE_DIR/userspace"
    fi
    if [ "$(basename "$test_file")" = "test_fsops.c" ]; then
        extra_src="$extra_src $BASE_DIR/userspace/fsops.c"
        cflags="$cflags -I$BASE_DIR/userspace"
    fi
    if [ "$(basename "$test_file")" = "test_osui_panes.c" ]; then
        extra_src="$extra_src $BASE_DIR/userspace/osui_panes.c"
        cflags="$cflags -I$BASE_DIR/userspace"
    fi
    if [ "$(basename "$test_file")" = "test_osui_editor.c" ]; then
        extra_src="$extra_src $BASE_DIR/userspace/osui_ide.c $BASE_DIR/userspace/umalloc.c"
        cflags="$cflags -I$BASE_DIR/userspace"
    fi
    if [ "$(basename "$test_file")" = "test_net_ifcfg.c" ]; then
        extra_src="$extra_src $BASE_DIR/userspace/net_ifcfg.c $BASE_DIR/kernel/net_dhcp.c"
        cflags="$cflags -I$BASE_DIR/userspace -I$BASE_DIR/include"
    fi
    if [ "$(basename "$test_file")" = "test_osui_ide.c" ]; then
        extra_src="$extra_src $BASE_DIR/userspace/osui_ide.c"
        cflags="$cflags -I$BASE_DIR/userspace"
    fi
    # Lot 67: Ring 3 web stack (wf_*), shares kernel SHA-256 / AES-GCM / X25519
    case "$(basename "$test_file")" in
        test_wf_crypto.c|test_wf_x509.c|test_wf_http.c|test_wf_text.c|test_wf_form.c|test_wf_section.c|test_wf_p256.c|test_wf_aead.c)
            extra_src="$extra_src $BASE_DIR/userspace/wf_bn.c $BASE_DIR/userspace/wf_sha512.c $BASE_DIR/userspace/wf_ec.c $BASE_DIR/userspace/wf_p256.c $BASE_DIR/userspace/wf_aead.c $BASE_DIR/userspace/wf_rsa.c $BASE_DIR/kernel/sha256.c"
            cflags="$cflags -I$BASE_DIR/userspace -DMOHHDY_OSUI_HOST_TEST=1 -DWF_ROOTS_PEM=\"$BASE_DIR/initrd_content/etc/ca-roots.pem\""
            ;;
    esac
    case "$(basename "$test_file")" in
        test_wf_x509.c) extra_src="$extra_src $BASE_DIR/userspace/wf_x509.c" ;;
        test_p2p_pace.c) extra_src="$extra_src $BASE_DIR/userspace/p2p_pace.c"; cflags="$cflags -I$BASE_DIR/userspace" ;;
        test_wf_cookie.c) extra_src="$extra_src $BASE_DIR/userspace/wf_cookie.c" ;;
        test_p2p_rtx.c) extra_src="$extra_src $BASE_DIR/userspace/p2p_rtx.c"; cflags="$cflags -I$BASE_DIR/userspace" ;;
        test_p2p_flow.c) extra_src="$extra_src $BASE_DIR/userspace/p2p_flow.c $BASE_DIR/userspace/p2p_pace.c"; cflags="$cflags -I$BASE_DIR/userspace" ;;
        test_p2p_bwe.c) extra_src="$extra_src $BASE_DIR/userspace/p2p_bwe.c $BASE_DIR/userspace/p2p_pace.c"; cflags="$cflags -I$BASE_DIR/userspace" ;;
        test_wf_text.c|test_wf_form.c|test_wf_section.c) extra_src="$extra_src $BASE_DIR/userspace/wf_x509.c $BASE_DIR/userspace/wf_http.c $BASE_DIR/userspace/wf_inflate.c $BASE_DIR/userspace/wf_dns.c $BASE_DIR/userspace/wf_tls.c $BASE_DIR/userspace/wf_fetch.c $BASE_DIR/userspace/wf_cookie.c $BASE_DIR/userspace/wf_html.c $BASE_DIR/userspace/wf_entity.c $BASE_DIR/userspace/wf_form.c $BASE_DIR/userspace/wf_section.c $BASE_DIR/kernel/aes_gcm.c $BASE_DIR/kernel/x25519.c" ;;
        test_wsearch_api.c) extra_src="$extra_src $BASE_DIR/userspace/wsearch_api.c $BASE_DIR/userspace/wsearch.c $BASE_DIR/userspace/wf_entity.c"; cflags="$cflags -I$BASE_DIR/userspace" ;;
        test_wf_http.c) extra_src="$extra_src $BASE_DIR/userspace/wf_x509.c $BASE_DIR/userspace/wf_http.c $BASE_DIR/userspace/wf_inflate.c $BASE_DIR/userspace/wf_dns.c $BASE_DIR/userspace/wf_tls.c $BASE_DIR/userspace/wf_fetch.c $BASE_DIR/userspace/wf_cookie.c $BASE_DIR/userspace/wf_html.c $BASE_DIR/userspace/wf_entity.c $BASE_DIR/userspace/wf_form.c $BASE_DIR/kernel/aes_gcm.c $BASE_DIR/kernel/x25519.c" ;;
    esac
    if [ "$(basename "$test_file")" = "test_tokenizer.c" ]; then
        extra_src="$extra_src $BASE_DIR/kernel/llm/gpt2_tokenizer.c"
    fi
    if [ "$(basename "$test_file")" = "test_gpt2_sample.c" ]; then
        extra_src="$extra_src $BASE_DIR/kernel/llm/gpt2_sample.c"
    fi
    if [ "$(basename "$test_file")" = "test_gpt2_infer.c" ]; then
        extra_src="$extra_src $BASE_DIR/kernel/llm/gpt2_infer.c $BASE_DIR/kernel/llm/gpt2_sample.c"
    fi
    if [ "$(basename "$test_file")" = "test_gpt2_generate.c" ]; then
        # FP32 generation core shared by the Ring 0 fallback and aiworker.
        extra_src="$extra_src $BASE_DIR/kernel/llm/gpt2_generate.c $BASE_DIR/kernel/llm/gpt2_infer.c $BASE_DIR/kernel/llm/gpt2_sample.c $BASE_DIR/kernel/llm/gpt2_tokenizer.c $BASE_DIR/kernel/llm/gpt2_model.c"
        cflags="$cflags -DMOHHDY_RING3"
    fi
    if [ "$(basename "$test_file")" = "test_gpt2_gguf_session.c" ]; then
        # GGUF 109/110 session shared by the Ring 0 fallback and aiworker.
        extra_src="$extra_src $BASE_DIR/kernel/llm/gpt2_gguf_session.c $BASE_DIR/kernel/llm/gpt2_generate.c $BASE_DIR/kernel/llm/gpt2_infer.c $BASE_DIR/kernel/llm/gpt2_sample.c $BASE_DIR/kernel/llm/gpt2_tokenizer.c $BASE_DIR/kernel/llm/gpt2_model.c"
        cflags="$cflags -DMOHHDY_RING3"
    fi
    if [ "$(basename "$test_file")" = "test_gpt2_gguf.c" ]; then
        extra_src="$extra_src $BASE_DIR/kernel/fs/fat16.c $BASE_DIR/kernel/fs/fat32.c $BASE_DIR/kernel/llm/gpt2_gguf.c $BASE_DIR/kernel/llm/gpt2_gguf_loader.c $BASE_DIR/kernel/llm/gpt2_quant.c $BASE_DIR/kernel/llm/gpt2_sample.c"
    fi
    if [ "$(basename "$test_file")" = "test_gpt2_gguf_infer.c" ]; then
        # Host binary: the null-volume checks do not allocate the kernel snapshot.
        extra_src="$extra_src $BASE_DIR/kernel/fs/fat16.c $BASE_DIR/kernel/fs/fat32.c $BASE_DIR/kernel/llm/gpt2_gguf.c $BASE_DIR/kernel/llm/gpt2_gguf_loader.c $BASE_DIR/kernel/llm/gpt2_quant.c $BASE_DIR/kernel/llm/gpt2_gguf_infer.c $BASE_DIR/kernel/llm/gpt2_sample.c"
        cflags="$cflags -DMOHHDY_RING3"
    fi
    if [ "$(basename "$test_file")" = "test_gpt2_quant.c" ]; then
        extra_src="$extra_src $BASE_DIR/kernel/llm/gpt2_quant.c"
    fi
    if [ "$(basename "$test_file")" = "test_fat16.c" ]; then
        extra_src="$extra_src $BASE_DIR/kernel/fs/fat16.c $BASE_DIR/kernel/fs/fat32.c $BASE_DIR/kernel/llm/gpt2_gguf.c $BASE_DIR/kernel/llm/gpt2_gguf_loader.c $BASE_DIR/kernel/llm/gpt2_quant.c $BASE_DIR/kernel/llm/gpt2_sample.c"
    fi
    if [ "$(basename "$test_file")" = "test_gfx_wm.c" ]; then
        extra_src="$extra_src $BASE_DIR/kernel/gfx_wm.c"
    fi
    if [ "$(basename "$test_file")" = "test_line_edit.c" ]; then
        extra_src="$extra_src $BASE_DIR/kernel/line_edit.c"
    fi
    if [ "$(basename "$test_file")" = "test_gfx_term.c" ]; then
        extra_src="$extra_src $BASE_DIR/kernel/gfx_term.c $BASE_DIR/kernel/gfx_wm.c"
    fi
    if [ "$(basename "$test_file")" = "test_vga_console.c" ]; then
        extra_src="$extra_src $BASE_DIR/kernel/vga_console.c"
    fi
    if [ "$(basename "$test_file")" = "test_umalloc.c" ]; then
        extra_src="$extra_src $BASE_DIR/userspace/umalloc.c"
        cflags="$cflags -I$BASE_DIR/userspace"
    fi
    if [ "$(basename "$test_file")" = "test_user_heap.c" ]; then
        extra_src="$extra_src $BASE_DIR/kernel/user_heap.c"
    fi
    if [ "$(basename "$test_file")" = "test_wsearch.c" ]; then
        extra_src="$extra_src $BASE_DIR/userspace/wsearch.c"
    fi
    if [ "$(basename "$test_file")" = "test_arm_portable.c" ]; then
        extra_src="$extra_src $BASE_DIR/arch/arm/arm_portable.c $BASE_DIR/kernel/aes_gcm.c $BASE_DIR/kernel/edid.c $BASE_DIR/userspace/p2p_flow.c $BASE_DIR/userspace/p2p_pace.c $BASE_DIR/userspace/p2p_rtx.c"
        cflags="$cflags -I$BASE_DIR/kernel -I$BASE_DIR/userspace -I$BASE_DIR/include"
    fi
    if [ "$(basename "$test_file")" = "test_edid.c" ]; then
        extra_src="$extra_src $BASE_DIR/kernel/edid.c"
    fi
    if [ "$(basename "$test_file")" = "test_out_capture.c" ]; then
        extra_src="$extra_src $BASE_DIR/kernel/out_capture.c"
    fi
    if [ "$(basename "$test_file")" = "test_gfx_desktop.c" ]; then
        extra_src="$extra_src $BASE_DIR/kernel/gfx_desktop.c $BASE_DIR/kernel/gfx_fb.c $BASE_DIR/kernel/edid.c $BASE_DIR/kernel/clipboard.c $BASE_DIR/kernel/gfx_wm.c $BASE_DIR/kernel/gfx_term.c $BASE_DIR/kernel/pci_service.c $BASE_DIR/kernel/input/usb_tablet.c"
    fi
    if [ "$(basename "$test_file")" = "test_ipc.c" ]; then
        extra_src="$extra_src $BASE_DIR/kernel/ipc.c"
    fi
    if [ "$(basename "$test_file")" = "test_service_registry.c" ]; then
        extra_src="$extra_src $BASE_DIR/kernel/service_registry.c"
    fi
    if [ "$(basename "$test_file")" = "test_net_tls_record.c" ] || [ "$(basename "$test_file")" = "test_tls_rsa_handshake.c" ]; then
        extra_src="$extra_src $BASE_DIR/kernel/net_tls_record.c"
        extra_src="$extra_src $BASE_DIR/kernel/sha256.c"
        extra_src="$extra_src $BASE_DIR/kernel/aes_gcm.c"
        extra_src="$extra_src $BASE_DIR/kernel/x509_der.c"
        extra_src="$extra_src $BASE_DIR/kernel/rsa_verify.c $BASE_DIR/kernel/bigint.c $BASE_DIR/kernel/ecdsa_p256.c"
    fi
    if [ "$(basename "$test_file")" = "test_aes_gcm.c" ]; then
        extra_src="$extra_src $BASE_DIR/kernel/aes_gcm.c"
    fi
    if [ "$(basename "$test_file")" = "test_bigint.c" ]; then
        extra_src="$extra_src $BASE_DIR/kernel/bigint.c"
    fi
    case "$(basename "$test_file")" in
        test_net_tls_record.c|test_tls_rsa_handshake.c|test_net_tcp.c|test_net_http_tls.c|test_ne2k.c)
            extra_src="$extra_src $BASE_DIR/kernel/x25519.c" ;;
    esac
    if [ "$(basename "$test_file")" = "test_ecdsa_p256.c" ]; then
        extra_src="$extra_src $BASE_DIR/kernel/ecdsa_p256.c $BASE_DIR/kernel/bigint.c"
    fi
    if [ "$(basename "$test_file")" = "test_x25519.c" ]; then
        extra_src="$extra_src $BASE_DIR/kernel/x25519.c $BASE_DIR/kernel/bigint.c"
    fi
    if [ "$(basename "$test_file")" = "test_rsa_verify.c" ]; then
        extra_src="$extra_src $BASE_DIR/kernel/rsa_verify.c $BASE_DIR/kernel/bigint.c"
    fi
    if [ "$(basename "$test_file")" = "test_x509_der.c" ]; then
        extra_src="$extra_src $BASE_DIR/kernel/x509_der.c"
        extra_src="$extra_src $BASE_DIR/kernel/sha256.c"
        extra_src="$extra_src $BASE_DIR/kernel/rsa_verify.c $BASE_DIR/kernel/bigint.c $BASE_DIR/kernel/ecdsa_p256.c"
    fi
    if [ "$(basename "$test_file")" = "test_sha256.c" ]; then
        extra_src="$extra_src $BASE_DIR/kernel/sha256.c"
    fi
    if [ "$(basename "$test_file")" = "test_fat32.c" ]; then
        extra_src="$extra_src $BASE_DIR/kernel/fs/fat32.c"
    fi
    if [ "$(basename "$test_file")" = "test_rtc.c" ]; then
        extra_src="$extra_src $BASE_DIR/kernel/rtc.c"
    fi
    if [ "$(basename "$test_file")" = "test_net_socket.c" ] || [ "$(basename "$test_file")" = "test_net_llm_socket.c" ]; then
        extra_src="$extra_src $BASE_DIR/kernel/net_socket.c"
        extra_src="$extra_src $BASE_DIR/kernel/x25519.c"
    fi
    if [ "$(basename "$test_file")" = "test_net_llm_socket.c" ]; then
        extra_src="$extra_src $BASE_DIR/kernel/net_llm_socket.c"
    fi
    if [ "$(basename "$test_file")" = "test_net_socket.c" ] || [ "$(basename "$test_file")" = "test_net_llm_socket.c" ] || [ "$(basename "$test_file")" = "test_net_tcp.c" ] || [ "$(basename "$test_file")" = "test_net_http_tls.c" ]; then
        extra_src="$extra_src $BASE_DIR/kernel/net_tcp.c"
        extra_src="$extra_src $BASE_DIR/kernel/net_tls_record.c"
        extra_src="$extra_src $BASE_DIR/kernel/sha256.c"
        extra_src="$extra_src $BASE_DIR/kernel/aes_gcm.c"
        extra_src="$extra_src $BASE_DIR/kernel/x509_der.c"
        extra_src="$extra_src $BASE_DIR/kernel/rsa_verify.c $BASE_DIR/kernel/bigint.c $BASE_DIR/kernel/ecdsa_p256.c"
    fi
    if [ "$(basename "$test_file")" = "test_net_llm_socket.c" ] || [ "$(basename "$test_file")" = "test_net_tcp.c" ] || [ "$(basename "$test_file")" = "test_net_http_tls.c" ]; then
        extra_src="$extra_src $BASE_DIR/kernel/net_http_tls.c"
    fi
    if [ "$(basename "$test_file")" = "test_net_dns.c" ]; then
        extra_src="$extra_src $BASE_DIR/kernel/net_dns.c"
    fi
    if [ "$(basename "$test_file")" = "test_net_ipv4_udp.c" ]; then
        extra_src="$extra_src $BASE_DIR/kernel/net_ipv4_udp.c"
    fi
    if [ "$(basename "$test_file")" = "test_net_dhcp.c" ]; then
        extra_src="$extra_src $BASE_DIR/kernel/net_dhcp.c"
    fi
    if [ "$(basename "$test_file")" = "test_ne2k.c" ]; then
        extra_src="$extra_src $BASE_DIR/kernel/ne2k.c $BASE_DIR/kernel/ne2k_hw.c $BASE_DIR/kernel/net_socket.c $BASE_DIR/kernel/net_llm_socket.c $BASE_DIR/kernel/rtc.c"
        extra_src="$extra_src $BASE_DIR/kernel/net_nic.c"
        extra_src="$extra_src $BASE_DIR/kernel/net_ethernet_arp.c"
        extra_src="$extra_src $BASE_DIR/kernel/net_ipv4_udp.c"
        extra_src="$extra_src $BASE_DIR/kernel/net_dhcp.c"
        extra_src="$extra_src $BASE_DIR/kernel/net_dns.c"
        extra_src="$extra_src $BASE_DIR/kernel/net_tcp.c"
        extra_src="$extra_src $BASE_DIR/kernel/net_tls_record.c $BASE_DIR/kernel/net_http_tls.c"
        extra_src="$extra_src $BASE_DIR/kernel/sha256.c"
        extra_src="$extra_src $BASE_DIR/kernel/aes_gcm.c"
        extra_src="$extra_src $BASE_DIR/kernel/x509_der.c"
        extra_src="$extra_src $BASE_DIR/kernel/rsa_verify.c $BASE_DIR/kernel/bigint.c $BASE_DIR/kernel/ecdsa_p256.c"
    fi
    if [ "$(basename "$test_file")" = "test_pci.c" ]; then
        extra_src="$extra_src $BASE_DIR/kernel/pci.c"
    fi
    if [ "$(basename "$test_file")" = "test_net_nic.c" ]; then
        extra_src="$extra_src $BASE_DIR/kernel/net_nic.c"
    fi
    if [ "$(basename "$test_file")" = "test_net_ethernet_arp.c" ]; then
        extra_src="$extra_src $BASE_DIR/kernel/net_ethernet_arp.c"
    fi
    if [ "$(basename "$test_file")" = "test_gpt2_gguf_bounds.c" ]; then
        extra_src="$extra_src $BASE_DIR/kernel/llm/gpt2_gguf.c"
    fi
    if [ "$(basename "$test_file")" = "test_pci_service.c" ]; then
        extra_src="$extra_src $BASE_DIR/kernel/pci_service.c"
    fi
    if [ "$(basename "$test_file")" = "test_device_service.c" ]; then
        extra_src="$extra_src $BASE_DIR/kernel/device_service.c"
    fi
    if [ "$(basename "$test_file")" = "test_arm_mmu.c" ]; then
        extra_src="$extra_src $BASE_DIR/arch/arm/arm_mmu.c"
    fi
    if [ "$(basename "$test_file")" = "test_usb_dma.c" ]; then
        extra_src="$extra_src $BASE_DIR/kernel/usb_dma.c $BASE_DIR/kernel/io_bitmap.c"
    fi
    if [ "$(basename "$test_file")" = "test_io_bitmap.c" ]; then
        extra_src="$extra_src $BASE_DIR/kernel/io_bitmap.c"
    fi
    if [ "$(basename "$test_file")" = "test_ata_job.c" ]; then
        extra_src="$extra_src $BASE_DIR/kernel/ata_job.c"
    fi
    # Tranche 4 suite: FS op slot/mirror + shared FAT/overlay codec/dispatch.
    if [ "$(basename "$test_file")" = "test_ata_fsop.c" ]; then
        extra_src="$extra_src $BASE_DIR/kernel/ata_fsop.c $BASE_DIR/kernel/fs/fsop_exec.c"
        extra_src="$extra_src $BASE_DIR/kernel/fs/fat16.c $BASE_DIR/kernel/fs/fat32.c"
    fi
    # Tranche 5 slices 2-3: net relay (pure) and wire bookkeeping/demux.
    if [ "$(basename "$test_file")" = "test_net_relay.c" ] || [ "$(basename "$test_file")" = "test_net_wire.c" ] || [ "$(basename "$test_file")" = "test_net_stack_exec.c" ]; then
        extra_src="$extra_src $BASE_DIR/kernel/net_relay.c $BASE_DIR/kernel/net_socket.c $BASE_DIR/kernel/net_tcp.c"
        extra_src="$extra_src $BASE_DIR/kernel/net_tls_record.c $BASE_DIR/kernel/sha256.c $BASE_DIR/kernel/aes_gcm.c"
        extra_src="$extra_src $BASE_DIR/kernel/x509_der.c $BASE_DIR/kernel/x25519.c $BASE_DIR/kernel/rsa_verify.c"
        extra_src="$extra_src $BASE_DIR/kernel/bigint.c $BASE_DIR/kernel/ecdsa_p256.c"
    fi
    if [ "$(basename "$test_file")" = "test_net_nic_owner.c" ]; then
        extra_src="$extra_src $BASE_DIR/kernel/net_nic_owner.c"
    fi
    if [ "$(basename "$test_file")" = "test_net_wire.c" ] || [ "$(basename "$test_file")" = "test_net_stack_exec.c" ]; then
        extra_src="$extra_src $BASE_DIR/kernel/net_wire.c $BASE_DIR/kernel/ne2k.c $BASE_DIR/kernel/ne2k_hw.c $BASE_DIR/kernel/net_llm_socket.c"
        extra_src="$extra_src $BASE_DIR/kernel/rtc.c $BASE_DIR/kernel/net_nic.c $BASE_DIR/kernel/net_ethernet_arp.c"
        extra_src="$extra_src $BASE_DIR/kernel/net_ipv4_udp.c $BASE_DIR/kernel/net_dhcp.c $BASE_DIR/kernel/net_dns.c"
        extra_src="$extra_src $BASE_DIR/kernel/net_http_tls.c"
    fi
    if [ "$(basename "$test_file")" = "test_net_stack_exec.c" ]; then
        extra_src="$extra_src $BASE_DIR/kernel/net_stack_exec.c $BASE_DIR/kernel/net_llm_client.c $BASE_DIR/kernel/net_p2p.c $BASE_DIR/kernel/net_tls_server.c $BASE_DIR/kernel/net_nic_owner.c"
    fi
    
    # Compiler
    echo "${TEST_CC:-gcc} $cflags -o \"$test_binary\" \"$test_file\" $extra_src \"$TEST_DIR/framework/unity.c\" \"$TEST_DIR/framework/test_kernel.c\" \"$TEST_DIR/framework/kernel_mocks.c\"" >> "$RESULTS_FILE"
    if ${TEST_CC:-gcc} $cflags -o "$test_binary" "$test_file" $extra_src "$TEST_DIR/framework/unity.c" "$TEST_DIR/framework/test_kernel.c" "$TEST_DIR/framework/kernel_mocks.c" 2>> "$RESULTS_FILE"; then
        # Exécuter le test
        local test_output
        # TEST_RUNNER lets a non-x86 host run the i386 ELF (for example
        # TEST_RUNNER="qemu-i386 -L /usr/i686-linux-gnu" on arm64). Empty on x86.
        if test_output=$(${TEST_RUNNER:-} "$test_binary" 2>&1); then
            echo -e "${GREEN}PASS${NC}"
            echo "$test_output" >> "$RESULTS_FILE"
            add_unity_counts "$test_output" pass
            return 0
        else
            echo -e "${RED}FAIL${NC}"
            echo "Test: $test_name" >> "$RESULTS_FILE"
            echo "$test_output" >> "$RESULTS_FILE"
            echo "---" >> "$RESULTS_FILE"
            add_unity_counts "$test_output" fail
            return 1
        fi
    else
        echo -e "${YELLOW}COMPILATION_ERROR${NC}"
        echo "Compilation failed for $test_name" >> "$RESULTS_FILE"
        # A test that does not build is a failure, not a skip: otherwise a
        # broken test silently drops out of make test-all.
        TOTAL_TESTS=$((TOTAL_TESTS + 1))
        FAILED_TESTS=$((FAILED_TESTS + 1))
        return 2
    fi
}

# Fonction pour exécuter une catégorie de tests
run_test_category() {
    local category=$1
    local description=$2
    
    print_header "$description"
    
    local category_passed=0
    local category_failed=0
    local category_total=0
    
    # Trouver tous les fichiers de test dans la catégorie
    for test_file in "$TEST_DIR/$category"/*.c; do
        if [ -f "$test_file" ]; then
            local test_name=$(basename "$test_file" .c)
            category_total=$((category_total + 1))
            
            if run_test "$test_file" "$test_name" "$category"; then
                category_passed=$((category_passed + 1))
            else
                category_failed=$((category_failed + 1))
            fi
        fi
    done
    
    echo ""
    if [ $category_total -eq 0 ]; then
        print_warning "No tests found in $category"
    else
        echo "Category Results: $category_passed/$category_total passed"
        if [ $category_failed -gt 0 ]; then
            print_error "$category_failed tests failed in $category"
        else
            print_success "All tests passed in $category"
        fi
    fi
    echo ""
}

# Couverture : ce lanceur ne mesure rien (pas d'instrumentation gcov/lcov).
# Les anciens pourcentages 85/72/78 etaient des constantes codees en dur ; ils
# ont ete retires pour ne plus afficher de mesure inventee.
generate_coverage_report() {
    print_header "Code Coverage Analysis"

    if [ "${ENABLE_COVERAGE:-0}" = "1" ]; then
        echo "Coverage: --coverage is on for this runner. The line percentage is printed by scripts/gcov_host_summary.py."
    else
        echo "Coverage: not measured (no gcov/lcov instrumentation in this runner)."
        echo "No percentage is reported; see test counts above for what was run."
    fi

    echo ""
}

# Fonction pour nettoyer les anciens fichiers de test
cleanup_old_files() {
    print_header "Cleanup"
    
    # Nettoyer les anciens binaires de test
    rm -f "$BUILD_DIR"/test_*
    
    # Nettoyer les anciens logs (garder les 5 derniers)
    ls -t "$LOG_DIR"/test_results_*.log 2>/dev/null | tail -n +6 | xargs rm -f 2>/dev/null || true
    
    print_success "Cleanup completed"
    echo ""
}

# Fonction principale
main() {
    local start_time=$(date +%s)
    
    echo "Starting test suite at $(date)"
    echo "Results will be logged to: $RESULTS_FILE"
    echo ""
    
    # Initialiser le fichier de résultats
    echo "MOHHDY Test Suite Results - $(date)" > "$RESULTS_FILE"
    echo "=========================================" >> "$RESULTS_FILE"
    echo "" >> "$RESULTS_FILE"
    
    # Nettoyer les anciens fichiers
    cleanup_old_files
    
    # Exécuter les différentes catégories de tests
    
    # 1. Tests unitaires kernel
    run_test_category "unit/kernel" "Unit Tests - Kernel Modules"
    
    # 2. Tests unitaires userspace
    run_test_category "unit/userspace" "Unit Tests - Userspace Modules"
    
    # 3. Tests unitaires filesystem
    run_test_category "unit/fs" "Unit Tests - Filesystem Modules"
    
    # 4. Tests d'intégration
    run_test_category "integration" "Integration Tests"
    
    # 5. Tests système
    run_test_category "system" "System Tests"
    
    # 6. Tests de performance
    run_test_category "performance" "Performance Tests"
    
    # 7. Tests de robustesse
    run_test_category "robustness" "Robustness Tests"
    
    # Générer le rapport de couverture
    generate_coverage_report
    
    # Calculer le temps d'exécution
    local end_time=$(date +%s)
    local duration=$((end_time - start_time))
    
    # Afficher le résumé final
    print_header "Final Results Summary"
    echo "Total Tests: $TOTAL_TESTS"
    echo "Passed: $PASSED_TESTS"
    echo "Failed: $FAILED_TESTS" 
    echo "Skipped: $SKIPPED_TESTS"
    echo "Execution Time: ${duration}s"
    echo ""
    
    # Écrire le résumé dans le fichier de log
    echo "" >> "$RESULTS_FILE"
    echo "=========================================" >> "$RESULTS_FILE"
    echo "FINAL SUMMARY" >> "$RESULTS_FILE"
    echo "=========================================" >> "$RESULTS_FILE"
    echo "Total Tests: $TOTAL_TESTS" >> "$RESULTS_FILE"
    echo "Passed: $PASSED_TESTS" >> "$RESULTS_FILE"
    echo "Failed: $FAILED_TESTS" >> "$RESULTS_FILE"
    echo "Skipped: $SKIPPED_TESTS" >> "$RESULTS_FILE"
    echo "Execution Time: ${duration}s" >> "$RESULTS_FILE"
    echo "Completed at: $(date)" >> "$RESULTS_FILE"
    
    # Déterminer le code de sortie
    if [ $FAILED_TESTS -eq 0 ]; then
        print_success "All tests completed successfully!"
        echo ""
        echo "Detailed results: $RESULTS_FILE"
        exit 0
    else
        print_error "Some tests failed!"
        echo ""
        echo "Check detailed results: $RESULTS_FILE"
        exit 1
    fi
}

# Options de ligne de commande
case "${1:-}" in
    --quick)
        print_header "Quick Test Mode"
        echo "Running only critical tests..."
        run_test_category "unit/kernel" "Critical Kernel Tests"
        ;;
    --kernel-only)
        print_header "Kernel Tests Only"
        run_test_category "unit/kernel" "Kernel Unit Tests"
        ;;
    --userspace-only)
        print_header "Userspace Tests Only" 
        run_test_category "unit/userspace" "Userspace Unit Tests"
        ;;
    --integration)
        print_header "Integration Tests Only"
        run_test_category "integration" "Integration Tests"
        ;;
    --performance)
        print_header "Performance Tests Only"
        run_test_category "performance" "Performance Tests"
        ;;
    --help)
        echo "Usage: $0 [option]"
        echo ""
        echo "Options:"
        echo "  --quick           Run only critical tests"
        echo "  --kernel-only     Run only kernel tests" 
        echo "  --userspace-only  Run only userspace tests"
        echo "  --integration     Run only integration tests"
        echo "  --performance     Run only performance tests"
        echo "  --help            Show this help"
        echo ""
        echo "Default: Run all tests"
        exit 0
        ;;
    "")
        # Mode par défaut - tous les tests
        main
        ;;
    *)
        echo "Unknown option: $1"
        echo "Use --help for usage information"
        exit 1
        ;;
esac
