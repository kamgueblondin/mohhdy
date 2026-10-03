#include "syscall.h"
#include "kernel.h"
#include "../interrupts.h"
#include "../task/task.h"
#include "../keyboard.h"
#include "../elf.h"
#include "../../fs/initrd.h"
#include "../../fs/overlay.h"
#include "../mem/string.h"
#include "../mem/vmm.h"
#include "../mem/pmm.h"
#include "../timer.h"
#include "../llm/gpt2_infer.h"
#include "../llm/gpt2_gguf_infer.h"
#include "../llm/gpt2_gguf_session.h"
#include "../llm/gpt2_model.h"
#include "../llm/gpt2_tokenizer.h"
#include "../llm/gpt2_generate.h"
#include "../ai_relay.h"
#include "../service_registry.h"
#include "../ata_job.h"
#include "../ata_fsop.h"
#include "../net_nic_owner.h"
#include "../fs/fsop_exec.h"
#include "../net_relay.h"
#include "../net_wire.h"
#include "../net_stack_exec.h"
#include "../ata.h"
#include "../gdt.h"
#include "../fs/fat16.h"
#include "../fs/fat32.h"
#include "../net_socket.h"
#include "../vga_console.h"
#include "../gfx_fb.h"
/* Completions locales : BPE, top-k basse temperature, arret EOT ou repetition,
 * et newline ou fin de phrase une fois 80 caracteres ecrits. */

static void sys_gets_cooperative(char* buffer, uint32_t size, cpu_state_t* cpu);

// Externs VMM
extern vmm_directory_t* current_directory;
extern void vmm_switch_page_directory(uint32_t phys_addr);

// Fonctions externes
extern void print_string_serial(const char* str);
extern uint32_t kernel_net_status(void);
extern uint32_t kernel_llm_session_status(void);
extern int kernel_llm_acquire_start(const os_llm_acquire_start_request_t* request);
extern int kernel_llm_poll_tls(void);
extern int kernel_llm_request(const os_llm_request_t* request);
extern int kernel_llm_poll_text(os_llm_text_result_t* result);
extern int kernel_llm_poll_sse(os_llm_text_result_t* result);
extern int kernel_llm_reset_for_request(void);
extern int kernel_llm_close(void);
extern int kernel_llm_configure_openai(const os_llm_openai_credential_request_t* request);
extern int kernel_peer_listen(const os_peer_listen_request_t* request);
extern int kernel_peer_accept(const os_peer_accept_request_t* request);
extern int kernel_peer_tls_poll(const os_peer_tls_poll_request_t* request);
extern int kernel_llm_dhcp_maintenance(uint32_t now);
/* Tranche 5 suite: NE2000 owned by the Ring 3 worker (kernel/kernel.c). */
extern int kernel_net_nic_port_mode(void);
extern int kernel_net_utc(char* out, uint16_t capacity);
extern int kernel_net_nic_pump(os_net_nic_pump_t* pump);
extern int kernel_net_nic_info(os_net_nic_info_t* info);
extern int kernel_net_nic_present(void);
extern int kernel_net_nic_reclaim(void);
extern int kernel_net_wire_connect(const os_net_wire_connect_t* request);
extern int kernel_net_wire_send(int socket_id, const uint8_t* data, uint16_t length,
                                uint8_t* segment, uint16_t capacity, uint16_t* out_length,
                                uint16_t attempts);
extern int kernel_net_wire_recv(int socket_id, uint8_t* buffer, uint16_t capacity,
                                uint16_t* out_length, uint16_t attempts);
extern int kernel_net_wire_close(int socket_id);
extern void print_char(char c, int x, int y, char color);
extern void write_serial(char c);

static void service_notify_change(const char* name, int32_t old_owner_pid,
                                  int32_t new_owner_pid, uint32_t reason) {
    os_ipc_payload_t payload;
    int32_t watchers[SERVICE_REGISTRY_WATCH_CAPACITY];
    int count;
    int i;
    if (os_service_make_event(&payload, name, old_owner_pid, new_owner_pid, reason) != 0) return;
    count = service_registry_collect_watchers(name, watchers, SERVICE_REGISTRY_WATCH_CAPACITY);
    if (count < 0) return;
    for (i = 0; i < count && i < (int)SERVICE_REGISTRY_WATCH_CAPACITY; i++) {
        task_t* watcher = get_task_by_id(watchers[i]);
        if (!watcher || watcher->type != TASK_TYPE_USER || watcher->state == TASK_TERMINATED) continue;
        /* Best effort non bloquant : une boîte pleine ne retarde jamais un changement de registre. */
        (void)service_registry_notify_record(name, watchers[i], old_owner_pid,
                                             new_owner_pid, reason, (uint32_t*)0);
        /* La boite de quatre places ne perd plus la copie : le surplus attend
         * le prochain ipc-recv. Le pull disque reste le journal durable. */
        if (ipc_endpoint_send(&watcher->ipc_endpoint, 0, &payload) == 0 ||
            service_registry_ipc_spill_push(watchers[i], &payload) == 0) {
            task_ipc_message_queued(watcher);
        }
    }
}

/* ==========================================================================
 * Tranche 4 slice 2: bridge between the kernel overlay snapshot, the kernel
 * PIO path and the Ring 3 atadriver (see kernel/ata_job.c).
 * ========================================================================== */
static int32_t ata_live_driver(void) {
    int32_t pid = service_registry_lookup("ata-driver");
    return pid > 0 ? pid : 0;
}

static int ata_bridge_snapshot(uint8_t* buf, uint32_t capacity) {
    uint32_t size = 0U;
    return overlay_snapshot(buf, capacity, &size);
}

static int ata_bridge_restore(const uint8_t* buf, uint32_t size) {
    return overlay_restore(buf, size);
}

static void ata_rpc_wait_flush(void);

/* Overlay flush: queue for the live driver instead of Ring 0 PIO. Slice 3:
 * from a user syscall the caller then waits until the driver wrote the whole
 * snapshot, so "write ok" still means "on disk" with the boot driver. */
static int ata_bridge_overlay_redirect(void) {
    if (ata_live_driver() <= 0) return 0;
    ata_job_request_flush();
    ata_rpc_wait_flush();
    return 1;
}

/* Kernel PIO gate: refused while the live driver holds the controller. */
static int ata_bridge_kernel_gate(void) {
    return ata_owner_kernel_may_pio(ata_live_driver());
}

static void ata_rpc_sched_hook(uint32_t now);

void syscall_ata_bridge_init(void) {
    ata_job_init(ata_bridge_snapshot, ata_bridge_restore);
    ata_fsop_init();
    ata_set_kernel_gate(ata_bridge_kernel_gate);
    overlay_set_disk_hooks(ata_bridge_overlay_redirect, ata_job_note_kernel_overlay_write);
    task_sched_hook = ata_rpc_sched_hook;
}

static int32_t sys_ai_engine(cpu_state_t* cpu);

/* Tranche 4 slice 3: synchronous FAT sector RPC through the Ring 3 driver.
 *
 * FAT16/FAT32 sector callbacks run inside a syscall of some user task T (the
 * VFS worker, the shell...). When the driver is live, the callback submits a
 * job (<= 8 sectors), saves T's kernel continuation (kctx on T's own kernel
 * stack), marks T TASK_BLOCKED_KERNEL and schedules the driver. The driver
 * fetches the job, runs the PIO at CPL 3 and completes it; SYS_ATA_JOB_DONE
 * marks T ready and the scheduler resumes T inside its syscall. While T is
 * blocked only the driver is scheduled, and any other task entering a
 * syscall is rewound and retried later, so FAT static buffers are never
 * re-entered. If the driver dies (purge) or stalls (no progress for
 * ATA_RPC_TIMEOUT_TICKS), the RPC is aborted and the callback falls back to
 * the Ring 0 PIO path (only possible once the driver no longer holds the
 * controller claim). */
#define ATA_RPC_TIMEOUT_TICKS 300U
#define ATA_RPC_IO 1
#define ATA_RPC_FLUSH 2
/* Tranche 4 suite: a whole FAT16/FAT32/overlay operation served by the
 * driver's own filesystem code (kernel/ata_fsop.c). */
#define ATA_RPC_FSOP 3
static task_t* g_rpc_waiter;
static int g_rpc_kind;
static uint32_t g_rpc_started;
static int g_rpc_aborted;
static int32_t g_boot_driver_pid;
static int g_boot_driver_registered;

static void ata_rpc_abort(void) {
    task_t* waiter = g_rpc_waiter;
    if (!waiter || waiter->state != TASK_BLOCKED_KERNEL) return;
    g_rpc_aborted = 1;
    if (g_rpc_kind == ATA_RPC_IO) ata_job_io_cancel();
    if (g_rpc_kind == ATA_RPC_FSOP) {
        ata_fsop_abort();
        ata_job_note_rpc_abort();
    }
    waiter->state = TASK_READY;
    task_sched_only = waiter;
}

static void ata_rpc_sched_hook(uint32_t now) {
    task_t* waiter = g_rpc_waiter;
    task_t* driver;
    if (!waiter || waiter->state != TASK_BLOCKED_KERNEL) return;
    driver = task_sched_only;
    if (now - g_rpc_started > ATA_RPC_TIMEOUT_TICKS ||
        !driver || driver->state == TASK_TERMINATED ||
        (driver->state != TASK_READY && driver->state != TASK_RUNNING)) {
        print_string_serial("[ATA] sector rpc aborted\n");
        ata_rpc_abort();
    }
}

/* Blocks the current task until the driver completed the submitted job.
 * Returns 0 on success, -1 on driver failure, -2 if aborted. */
static int ata_rpc_block(task_t* driver, int kind) {
    task_t* self = current_task;
    g_rpc_kind = kind;
    g_rpc_waiter = self;
    g_rpc_aborted = 0;
    g_rpc_started = timer_get_ticks();
    task_sched_only = driver;
    self->state = TASK_BLOCKED_KERNEL;
    self->kctx_valid = 1U;
    if (kctx_save(self->kctx) == 0) {
        schedule(self->syscall_frame); /* never returns; resumed below */
    }
    g_rpc_waiter = NULL;
    task_sched_only = NULL;
    self->kctx_valid = 0U;
    if (g_rpc_aborted) return -2;
    if (kind == ATA_RPC_FLUSH) return 0;
    if (kind == ATA_RPC_FSOP) return ata_fsop_state() == ATA_FSOP_DONE ? 0 : -1;
    return ata_job_io_state() == ATA_IO_DONE ? 0 : -1;
}

/* 1 if the current task may block on a driver RPC right now. */
static task_t* ata_rpc_driver_for_current(void) {
    int32_t driver_pid = ata_live_driver();
    task_t* driver;
    if (driver_pid <= 0 || !current_task || current_task->type != TASK_TYPE_USER ||
        (int32_t)current_task->id == driver_pid || !current_task->syscall_frame || g_rpc_waiter)
        return NULL;
    driver = get_task_by_id(driver_pid);
    if (!driver || driver->type != TASK_TYPE_USER || driver->state != TASK_READY) return NULL;
    return driver;
}

/* If the driver dies meanwhile, the purge path persists the snapshot through
 * Ring 0 PIO; if it stalls, the flush stays queued (asynchronous). */
static void ata_rpc_wait_flush(void) {
    task_t* driver = ata_rpc_driver_for_current();
    if (!driver) return;
    (void)ata_rpc_block(driver, ATA_RPC_FLUSH);
}

int syscall_ata_fat_io(uint32_t drive, uint32_t lba, uint32_t count, void* buf,
                       int write, int* out_rc) {
    int32_t driver_pid;
    task_t* driver = ata_rpc_driver_for_current();
    uint32_t done = 0U;
    if (!driver || !buf || count == 0U) return 0;
    while (done < count) {
        uint32_t n = count - done;
        uint8_t* p = (uint8_t*)buf + done * 512U;
        int st;
        if (n > OS_ATA_JOB_MAX_SECTORS) n = OS_ATA_JOB_MAX_SECTORS;
        if (ata_job_io_submit(drive, lba + done, n, write, write ? p : 0) != 0) return 0;
        st = ata_rpc_block(driver, ATA_RPC_IO);
        if (st == -2) return 0; /* driver gone/stalled: caller uses Ring 0 PIO */
        if (ata_job_io_take(write ? 0 : p, write ? 0U : n * 512U) != 0 || st != 0) {
            *out_rc = -1;
            return 1;
        }
        done += n;
        driver_pid = ata_live_driver();
        driver = driver_pid > 0 ? get_task_by_id(driver_pid) : NULL;
        if (done < count && (!driver || driver->state != TASK_READY)) {
            /* Remaining sectors through the fallback path. */
            int rc = write ? ata_write_sectors_drive((uint8_t)drive, lba + done, count - done,
                                                     (const uint8_t*)buf + done * 512U)
                           : ata_read_sectors_drive((uint8_t)drive, lba + done, count - done,
                                                    (uint8_t*)buf + done * 512U);
            if (rc == 0) ata_job_note_fat_kernel_pio(count - done, ata_live_driver() > 0);
            *out_rc = rc;
            return 1;
        }
    }
    *out_rc = 0;
    return 1;
}

void syscall_ata_note_fat_kernel_pio(uint32_t sectors) {
    ata_job_note_fat_kernel_pio(sectors, ata_live_driver() > 0);
}

void syscall_ata_set_boot_driver(int32_t pid) {
    g_boot_driver_pid = pid;
    g_boot_driver_registered = 0;
    ata_job_set_boot_driver(pid);
}

/* Called after a service purge: if the driver vanished with a queued or
 * in-flight flush, persist through the Ring 0 PIO fallback right away; a
 * task blocked on a sector RPC is resumed and falls back too. */
static void ata_bridge_after_purge(void) {
    int in_use;
    int persist = 0;
    if (ata_live_driver() > 0) return;
    ata_rpc_abort();
    in_use = ata_job_controller_in_use();
    if (in_use) {
        /* The dead driver may have stopped mid-transfer: reset the channel. */
        if (ata_channel_reset() == 0) print_string_serial("[ATA] channel reset after driver loss\n");
        ata_job_note_channel_reset();
    }
    /* Tranche 4 suite: the store served by the dead driver comes back to the
     * kernel (fallback). The overlay is restored from the last image the
     * driver published (published before its disk write, so a torn write is
     * repaired by the Ring 0 persist below); the kernel FAT copy drops its
     * caches since the driver changed the volumes behind it. */
    if (ata_fsop_store_pid() > 0) {
        uint32_t size = 0U;
        const uint8_t* image = ata_fsop_mirror(&size);
        uint32_t flags = ata_fsop_store_flags(ata_fsop_store_pid());
        ata_fsop_store_drop();
        fat16_invalidate_caches(fat16_root());
        if ((flags & OS_ATA_FS_STORE_OVERLAY) != 0U) {
            if (image && overlay_restore(image, size) == 0) {
                ata_fsop_note_restore();
                print_string_serial("[ATA] store back in Ring 0 (overlay from driver mirror)\n");
            } else {
                print_string_serial("[ATA] store back in Ring 0 (mirror invalid, kernel copy kept)\n");
            }
            persist = 1;
        } else {
            print_string_serial("[ATA] store back in Ring 0 (FAT only)\n");
        }
    }
    if (ata_job_driver_gone()) persist = 1;
    if (persist) {
        if (overlay_save_disk() == 0) ata_job_note_fallback_flush();
    }
}

/* ==========================================================================
 * Tranche 4 suite: FAT16/FAT32/overlay operations served by the driver.
 *
 * When the live driver announced a store (OS_ATA_FS_READY), the FAT and
 * overlay syscalls below no longer run the kernel filesystem code: the
 * request is encoded (paths, input bytes) into the kernel slot, the caller
 * blocks on the same synchronous RPC as the slice 3 sector jobs, and the
 * driver executes it with its own FAT16/FAT32/overlay code and its own PIO.
 * The kernel only copies bytes back. Failure policy:
 *  - driver cannot serve (suspended, RPC busy): OS_ATA_FS_UNAVAILABLE, the
 *    stale kernel copy is never used instead (fail closed);
 *  - driver stalled past the timeout: OS_ATA_FS_TIMEOUT (outcome unknown);
 *  - driver died: the store is back in Ring 0 (ata_bridge_after_purge); an
 *    overlay op the driver already published returns its published result,
 *    anything else that committed nothing is redone by Ring 0 once; a FAT
 *    mutation that completed at least one sector write returns
 *    OS_ATA_FS_ABORTED (no replay of an uncertain mutation).
 * ========================================================================== */
#define ATA_FS_LOCAL 0
#define ATA_FS_DONE 1
#define ATA_FS_OUT_MAX (OS_ATA_FSOP_BUFFER_SIZE - (uint32_t)sizeof(os_ata_fsop_reply_t))
static uint8_t g_fsop_out[OS_ATA_FSOP_BUFFER_SIZE];

static int ata_fs_relay(uint32_t op, uint32_t arg0, uint32_t arg1, const char* path,
                        const char* path2, const void* in, uint32_t in_len, void* out,
                        uint32_t out_cap, int32_t* rc) {
    int32_t driver_pid = ata_live_driver();
    uint32_t store = fsop_store_for(op);
    uint32_t flags = 0U;
    task_t* driver;
    os_ata_fsop_reply_t reply;
    int len, st, committed, published;
    int32_t published_result = 0;
    if (store == 0U || (ata_fsop_store_flags(driver_pid) & store) == 0U) return ATA_FS_LOCAL;
    driver = ata_rpc_driver_for_current();
    if (!driver) {
        ata_fsop_note_unavailable();
        *rc = OS_ATA_FS_UNAVAILABLE;
        return ATA_FS_DONE;
    }
    if (out_cap > ATA_FS_OUT_MAX) out_cap = ATA_FS_OUT_MAX;
    len = fsop_encode(ata_fsop_request_buffer(), ata_fsop_request_capacity(), op, arg0, arg1,
                      path, path2, in, in_len, out_cap);
    if (len < 0) {
        if (store == OS_ATA_FS_STORE_OVERLAY) {
            *rc = OV_ERR_INVAL; /* the driver's overlay would refuse it too */
            return ATA_FS_DONE;
        }
        /* Oversized FAT request (no current caller): kernel FAT code on the
         * disk itself, counted as kernel FS work while the store is live. */
        fat16_invalidate_caches(fat16_root());
        ata_fsop_note_kernel_live();
        return ATA_FS_LOCAL;
    }
    if (fsop_is_fat_mutation(op) && ata_job_debug_take_crash()) flags = OS_ATA_JOB_FLAG_DEBUG_CRASH;
    if (ata_fsop_submit((uint32_t)len, op, driver_pid, flags) < 0) {
        ata_fsop_note_unavailable();
        *rc = OS_ATA_FS_UNAVAILABLE;
        return ATA_FS_DONE;
    }
    st = ata_rpc_block(driver, ATA_RPC_FSOP);
    if (st == 0 && ata_fsop_take(g_fsop_out, sizeof(g_fsop_out), &reply) == 0) {
        if (store != OS_ATA_FS_STORE_OVERLAY) {
            ata_job_note_fat_driver_io(reply.sectors_read, reply.sectors_written);
            if (reply.sectors_written) fat16_invalidate_caches(fat16_root());
        }
        if (reply.out_len > out_cap || (reply.out_len && !out)) {
            *rc = -1;
            return ATA_FS_DONE;
        }
        memcpy(out, g_fsop_out, reply.out_len);
        *rc = reply.result;
        return ATA_FS_DONE;
    }
    committed = ata_fsop_committed();
    published = ata_fsop_published(&published_result);
    ata_fsop_release();
    if (ata_live_driver() == driver_pid && driver_pid > 0) {
        print_string_serial("[ATA] fs op timeout (driver stalled)\n");
        *rc = OS_ATA_FS_TIMEOUT;
        return ATA_FS_DONE;
    }
    /* Driver died: ata_bridge_after_purge already took the store back. */
    if (store == OS_ATA_FS_STORE_OVERLAY) {
        if (published && fsop_is_mutation(op)) {
            *rc = published_result;
            return ATA_FS_DONE;
        }
        ata_fsop_note_redone();
        print_string_serial("[ATA] fs op redone in Ring 0 after driver loss\n");
        return ATA_FS_LOCAL;
    }
    fat16_invalidate_caches(fat16_root());
    if (committed && fsop_is_mutation(op)) {
        print_string_serial("[ATA] fs op aborted after a committed sector\n");
        *rc = OS_ATA_FS_ABORTED;
        return ATA_FS_DONE;
    }
    ata_fsop_note_redone();
    print_string_serial("[ATA] fs op redone in Ring 0 after driver loss\n");
    return ATA_FS_LOCAL;
}

/* Overlay store entry points used by every syscall below: the driver's
 * store when it serves one, the kernel overlay otherwise. */
#define OVS_DIRENT_MAX (ATA_FS_OUT_MAX / (uint32_t)sizeof(os_dirent_t))

static int ovs_read(const char* path, char* buf, uint32_t max) {
    int32_t rc;
    uint32_t cap = max < ATA_FS_OUT_MAX ? max : ATA_FS_OUT_MAX;
    if (path && ata_fs_relay(OS_ATA_FSOP_OVL_READ, cap, 0U, path, 0, 0, 0U, buf, cap, &rc))
        return rc;
    return overlay_read(path, buf, max);
}

static int ovs_write(const char* path, const char* data, uint32_t n) {
    int32_t rc;
    if (path && ata_fs_relay(OS_ATA_FSOP_OVL_WRITE, 0U, 0U, path, 0, data, n, 0, 0U, &rc)) return rc;
    return overlay_write(path, data, n);
}

static int ovs_append(const char* path, const char* data, uint32_t n) {
    int32_t rc;
    if (path && ata_fs_relay(OS_ATA_FSOP_OVL_APPEND, 0U, 0U, path, 0, data, n, 0, 0U, &rc)) return rc;
    return overlay_append(path, data, n);
}

static int ovs_path_op(uint32_t op, const char* path, const char* path2, int* handled) {
    int32_t rc = -1;
    int two = op == OS_ATA_FSOP_OVL_RENAME || op == OS_ATA_FSOP_OVL_COPY;
    *handled = 0;
    if (!path || (two && !path2)) return rc;
    *handled = ata_fs_relay(op, 0U, 0U, path, path2, 0, 0U, 0, 0U, &rc);
    return rc;
}

static int ovs_mkdir(const char* path) {
    int handled, rc = ovs_path_op(OS_ATA_FSOP_OVL_MKDIR, path, 0, &handled);
    return handled ? rc : overlay_mkdir(path);
}

static int ovs_unlink(const char* path) {
    int handled, rc = ovs_path_op(OS_ATA_FSOP_OVL_UNLINK, path, 0, &handled);
    return handled ? rc : overlay_unlink(path);
}

static int ovs_rename(const char* oldpath, const char* newpath) {
    int handled, rc = ovs_path_op(OS_ATA_FSOP_OVL_RENAME, oldpath, newpath, &handled);
    return handled ? rc : overlay_rename(oldpath, newpath);
}

static int ovs_copy(const char* src, const char* dst) {
    int handled, rc = ovs_path_op(OS_ATA_FSOP_OVL_COPY, src, dst, &handled);
    return handled ? rc : overlay_copy(src, dst);
}

static int ovs_is_dir(const char* path) {
    int handled, rc = ovs_path_op(OS_ATA_FSOP_OVL_IS_DIR, path, 0, &handled);
    return handled ? rc : overlay_is_dir(path);
}

static int ovs_stat(const char* path, os_dirent_t* out) {
    int32_t rc;
    if (path && out && ata_fs_relay(OS_ATA_FSOP_OVL_STAT, 0U, 0U, path, 0, 0, 0U, out,
                                    (uint32_t)sizeof(*out), &rc))
        return rc;
    return overlay_stat(path, out);
}

static int ovs_listdir(const char* path, os_dirent_t* out, int start, int max_n) {
    int32_t rc;
    uint32_t n = max_n > 0 ? (uint32_t)max_n : 0U;
    uint32_t first = start > 0 ? (uint32_t)start : 0U;
    if (n > OVS_DIRENT_MAX) n = OVS_DIRENT_MAX;
    if (path && out && n > 0U && first <= n &&
        ata_fs_relay(OS_ATA_FSOP_OVL_LISTDIR, first, n, path, 0, out,
                     first * (uint32_t)sizeof(os_dirent_t), out, n * (uint32_t)sizeof(os_dirent_t), &rc))
        return rc;
    return overlay_listdir(path, out, start, max_n);
}

static int ovs_listdir_page(const char* path, os_dirent_t* out, uint32_t start, int max_n) {
    int32_t rc;
    uint32_t n = max_n > 0 ? (uint32_t)max_n : 0U;
    if (path && out && n > 0U && n <= OVS_DIRENT_MAX &&
        ata_fs_relay(OS_ATA_FSOP_OVL_LISTDIR_PAGE, start, n, path, 0, 0, 0U, out,
                     n * (uint32_t)sizeof(os_dirent_t), &rc))
        return rc;
    return overlay_listdir_page(path, out, start, max_n);
}

/* FAT entry points: relayed when the driver serves the volume, else the
 * kernel code (boot, fallback, no driver). */
static int fsr_fat_read(uint32_t op, const char* name, char* buffer, uint32_t max, int* handled) {
    int32_t rc = -1;
    uint32_t cap = max;
    /* A read larger than one reply window stays on the kernel path (the
     * relay would truncate it); no Ring 3 caller asks for more today. */
    *handled = 0;
    if (cap > ATA_FS_OUT_MAX) {
        if (ata_fsop_store_flags(ata_live_driver()) & fsop_store_for(op)) {
            fat16_invalidate_caches(fat16_root());
            ata_fsop_note_kernel_live();
        }
        return 0;
    }
    *handled = ata_fs_relay(op, cap, 0U, name, 0, 0, 0U, buffer, cap, &rc);
    return rc;
}

static int fsr_fat_list(uint32_t op, const char* path, os_fat16_dirent_t* out, uint32_t capacity,
                        uint32_t start, int* handled) {
    int32_t rc = -1;
    uint32_t cap = capacity;
    uint32_t each = (uint32_t)sizeof(os_fat16_dirent_t);
    *handled = 0;
    if (cap > ATA_FS_OUT_MAX / each) {
        if (ata_fsop_store_flags(ata_live_driver()) & fsop_store_for(op)) {
            fat16_invalidate_caches(fat16_root());
            ata_fsop_note_kernel_live();
        }
        return 0;
    }
    *handled = ata_fs_relay(op, cap, start, path, 0, 0, 0U, out, cap * each, &rc);
    return rc;
}

static int fsr_fat_create(uint32_t op, const char* name, const char* data, uint32_t size, int* handled) {
    int32_t rc = -1;
    int is_dir = !data && size == 0U;
    *handled = ata_fs_relay(op, is_dir ? 1U : 0U, 0U, name, 0, data, size, 0, 0U, &rc);
    return rc;
}

static int fsr_fat_path(uint32_t op, const char* a, const char* b, int* handled) {
    int32_t rc = -1;
    *handled = ata_fs_relay(op, 0U, 0U, a, b, 0, 0U, 0, 0U, &rc);
    return rc;
}

static int sys_ata_claim(void) {
    int rc;
    if (!current_task || current_task->type != TASK_TYPE_USER) return OS_ATA_DRIVER_REQUIRED;
    rc = ata_owner_claim(current_task->id, ata_live_driver());
    /* Returning to Ring 3 without a task switch: apply the IOPB now. */
    if (rc == 0) tss_set_ata_io(1);
    return rc;
}

static int sys_ata_release(void) {
    int rc;
    if (!current_task || current_task->type != TASK_TYPE_USER) return OS_ATA_DRIVER_REQUIRED;
    rc = ata_owner_release(current_task->id);
    tss_set_ata_io(0);
    return rc;
}

static int syscall_user_range(const void* pointer, uint32_t length, int write);

static int sys_ata_job_fetch(os_ata_job_t* job, uint8_t* data) {
    if (!current_task || current_task->type != TASK_TYPE_USER ||
        !service_registry_ata_ports_granted(current_task->id))
        return OS_ATA_DRIVER_REQUIRED;
    if (!syscall_user_range(job, sizeof(*job), 1) ||
        !syscall_user_range(data, OS_ATA_JOB_MAX_SECTORS * 512U, 1))
        return OS_ATA_JOB_STALE;
    /* Tranche 4 suite: a filesystem op (caller blocked) goes first. */
    if (ata_fsop_fetch((int32_t)current_task->id, job) == 1) return 1;
    return ata_job_fetch(job, data, OS_ATA_JOB_MAX_SECTORS * 512U);
}

static int sys_ata_job_done(const os_ata_job_t* job, int32_t status, const uint8_t* data) {
    if (!current_task || current_task->type != TASK_TYPE_USER ||
        !service_registry_ata_ports_granted(current_task->id))
        return OS_ATA_DRIVER_REQUIRED;
    if (!syscall_user_range(job, sizeof(*job), 0) ||
        (data && !syscall_user_range(data, OS_ATA_JOB_MAX_SECTORS * 512U, 0)))
        return OS_ATA_JOB_STALE;
    return ata_job_done(job, status, data, data ? OS_ATA_JOB_MAX_SECTORS * 512U : 0U);
}

static int sys_ata_status(os_ata_status_t* out) {
    if (!syscall_user_range(out, sizeof(*out), 1)) return OS_SERVICE_BAD_NAME;
    ata_job_fill_status(out, ata_live_driver());
    ata_fsop_fill_status(out, ata_live_driver());
    return 0;
}

/* Tranche 4 suite: SYS_ATA_FS driver side. */
static int ata_fs_caller_is_driver(void) {
    return current_task && current_task->type == TASK_TYPE_USER &&
           (int32_t)current_task->id == ata_live_driver() &&
           service_registry_ata_ports_granted(current_task->id);
}

/* Store handover kernel -> driver. Refused while a slice 2 snapshot job is
 * queued (the driver serves it first). With the overlay, the kernel
 * serialises its store once into the mirror and the driver's buffer, then
 * stops running its own overlay code for the syscalls (same syscall, so no
 * kernel mutation can slip in between). */
static int sys_ata_fs_ready(uint32_t flags, uint8_t* image, uint32_t capacity) {
    uint32_t size = 0U;
    int32_t pid;
    if (!ata_fs_caller_is_driver()) return OS_ATA_DRIVER_REQUIRED;
    pid = (int32_t)current_task->id;
    if (flags == 0U || (flags & ~OS_ATA_FS_STORE_ALL) != 0U) return OS_ATA_JOB_STALE;
    if (ata_job_pending() || ata_fsop_state() != ATA_FSOP_FREE) return OS_ATA_FS_BUSY;
    if ((flags & OS_ATA_FS_STORE_FAT16) && !fat16_is_mounted(fat16_root())) flags &= ~OS_ATA_FS_STORE_FAT16;
    if ((flags & OS_ATA_FS_STORE_FAT32) && !fat32_is_mounted(fat32_root())) flags &= ~OS_ATA_FS_STORE_FAT32;
    if (flags & OS_ATA_FS_STORE_OVERLAY) {
        if (!syscall_user_range(image, capacity, 1)) return OS_ATA_JOB_STALE;
        if (overlay_snapshot(ata_fsop_mirror_buffer(), ATA_FSOP_MIRROR_BYTES, &size) != 0 ||
            size > capacity || ata_fsop_mirror_commit(size) != 0) return OS_ATA_JOB_STALE;
        memcpy(image, ata_fsop_mirror_buffer(), size);
    }
    if (ata_fsop_store_ready(pid, flags) != 0) return OS_ATA_JOB_STALE;
    fat16_invalidate_caches(fat16_root());
    ata_fsop_note_handover();
    print_string_serial("[ATA] store handed to the Ring 3 driver\n");
    return (int)size;
}

static void ata_fs_wake_waiter(cpu_state_t* cpu) {
    if (g_rpc_waiter && g_rpc_waiter->state == TASK_BLOCKED_KERNEL && g_rpc_kind == ATA_RPC_FSOP &&
        ata_fsop_state() == ATA_FSOP_DONE) {
        g_rpc_waiter->state = TASK_READY;
        task_sched_only = g_rpc_waiter;
        schedule(cpu);
    }
}

static int32_t sys_ata_fs(cpu_state_t* cpu) {
    int32_t pid = current_task ? (int32_t)current_task->id : 0;
    int rc;
    switch (cpu->ebx) {
        case OS_ATA_FS_STATUS:
            return sys_ata_status((os_ata_status_t*)cpu->ecx);
        case OS_ATA_FS_READY:
            return sys_ata_fs_ready(cpu->ecx, (uint8_t*)cpu->edx, cpu->esi);
        default:
            break;
    }
    if (!ata_fs_caller_is_driver()) return OS_ATA_DRIVER_REQUIRED;
    switch (cpu->ebx) {
        case OS_ATA_FS_REQUEST: {
            const os_ata_job_t* job = (const os_ata_job_t*)cpu->ecx;
            if (!syscall_user_range(job, sizeof(*job), 0) ||
                !syscall_user_range((void*)cpu->edx, cpu->esi, 1)) return OS_ATA_JOB_STALE;
            return ata_fsop_copy_request(pid, job->generation, (uint8_t*)cpu->edx, cpu->esi);
        }
        case OS_ATA_FS_NOTE:
            rc = ata_fsop_note(pid, cpu->ecx, cpu->edx);
            if (rc == 0) {
                if (g_rpc_waiter) g_rpc_started = timer_get_ticks(); /* progress */
                if (cpu->edx == OS_ATA_FS_NOTE_PERSISTED) ata_job_note_driver_flush();
            }
            return rc;
        case OS_ATA_FS_PUBLISH:
            if (!syscall_user_range((const void*)cpu->edx, cpu->esi, 0)) return OS_ATA_JOB_STALE;
            rc = ata_fsop_publish(pid, cpu->ecx, (const uint8_t*)cpu->edx, cpu->esi, (int32_t)cpu->edi);
            if (rc == 0 && g_rpc_waiter) g_rpc_started = timer_get_ticks();
            return rc;
        case OS_ATA_FS_DONE: {
            const os_ata_job_t* job = (const os_ata_job_t*)cpu->ecx;
            if (!syscall_user_range(job, sizeof(*job), 0) ||
                !syscall_user_range((const void*)cpu->edx, cpu->esi, 0)) return OS_ATA_JOB_STALE;
            rc = ata_fsop_done(pid, job->generation, (const uint8_t*)cpu->edx, cpu->esi);
            if (rc == OS_ATA_JOB_FS_DONE) {
                cpu->eax = (uint32_t)rc;
                ata_fs_wake_waiter(cpu); /* returns here only if nobody waits */
            }
            return rc;
        }
        case OS_ATA_FS_LOG: {
            /* Same discipline as OS_NET_NIC_LOG: the whole counter line is
             * printed inside one syscall, so IRQ0 can no longer cut shell
             * output into it (qemu-ata-driver flake, run 37059589131). */
            const char* text = (const char*)cpu->ecx;
            uint32_t i, n = cpu->edx;
            if (n > OS_ATA_FS_LOG_MAX || !syscall_user_range(text, n, 0)) return OS_ATA_JOB_STALE;
            for (i = 0U; i < n; i++) {
                print_char(text[i], -1, -1, 0x0F);
                write_serial(text[i]);
            }
            return 0;
        }
        case OS_ATA_FS_INITRD_STAT: {
            const char* path = (const char*)cpu->ecx;
            if (!syscall_user_range(path, 1U, 0)) return -1;
            return (initrd_is_file(path) ? OS_ATA_FS_INITRD_FILE : 0) |
                   (initrd_is_dir(path) ? OS_ATA_FS_INITRD_DIR : 0);
        }
        case OS_ATA_FS_INITRD_READ: {
            const char* path = (const char*)cpu->ecx;
            if (!syscall_user_range(path, 1U, 0) || cpu->esi == 0U ||
                !syscall_user_range((void*)cpu->edx, cpu->esi, 1)) return -1;
            return initrd_read_into(path, (char*)cpu->edx, cpu->esi);
        }
        default:
            return OS_ATA_JOB_STALE;
    }
}

static void service_notify_purge_pid(int32_t pid) {
    service_registry_entry_t owned[SERVICE_REGISTRY_CAPACITY];
    int count = service_registry_collect_owned(pid, owned, SERVICE_REGISTRY_CAPACITY);
    int i;
    if (count > 0) {
        for (i = 0; i < count && i < (int)SERVICE_REGISTRY_CAPACITY; i++) {
            service_notify_change(owned[i].name, pid, 0, OS_SERVICE_EVENT_PURGED);
        }
        (void)service_registry_remove_pid(pid);
    }
    ata_bridge_after_purge();
    /* Tranche 5 suite: a dead NIC owner gives the card back to Ring 0. */
    if (nic_owner_drop_if_gone(sys_service_lookup("net-driver"))) {
        tss_set_nic_io(0);
        (void)kernel_net_nic_reclaim();
    }
}

/* Tranche 4: a Ring 3 fault (e.g. #GP from an IN/OUT on a port denied by the
 * TSS IOPB) terminates only the faulting task, like SYS_EXIT with the killed
 * exit code, instead of halting the kernel. Never returns. */
void syscall_kill_current_on_user_fault(cpu_state_t* cpu) {
    if (!current_task) return;
    service_notify_purge_pid(current_task->id);
    service_registry_backend_remove_pid(current_task->id);
    (void)service_registry_remove_watcher_pid(current_task->id);
    task_report_parent_exit(current_task, OS_TASK_EXIT_KILLED, OS_TASK_EVENT_KILLED);
    task_wake_waiter(current_task);
    task_reparent_children(current_task);
    current_task->state = TASK_TERMINATED;
    schedule(cpu);
}

// ==============================================================================
// GESTIONNAIRE D'APPELS SYSTÈME
// ==============================================================================

/* AOS-2178: historical overlay mutations (SYS_MKDIR, SYS_UNLINK, SYS_RENAME,
 * SYS_COPY, SYS_APPEND) only for the live vfs-virtual PID; degraded mode
 * without the worker keeps local exercise. */
static int historical_overlay_mutation_allowed(void) {
    return current_task && service_registry_ata_overlay_io_via_worker(current_task->id);
}


/* ------------------------------------------------------------------------
 * Tranche 5 slice 2: net IPC relay (socket syscalls 99-108).
 * The caller keeps re-entering its own syscall (rewind over int 0x80 and
 * yield) until the worker reply is stored; user buffers are only touched
 * in the caller's own context. One request in flight at a time. */
static void net_relay_wait(cpu_state_t* cpu) {
    cpu->eip -= 2U;
    schedule(cpu);
}

static int32_t net_relay_live_worker(void) {
    int32_t worker = sys_service_lookup("net-driver");
    return worker > 0 ? worker : 0;
}

static uint16_t net_relay_cap(uint32_t capacity) {
    return (uint16_t)(capacity < OS_NET_RELAY_MAX_OUT ? capacity : OS_NET_RELAY_MAX_OUT);
}

static void net_relay_copy(uint8_t* dst, const uint8_t* src, uint32_t n) {
    uint32_t i;
    for (i = 0U; i < n; i++) dst[i] = src[i];
}

/* Fills req from the caller registers; 0 or an OS_SOCKET_* error. */
static int net_relay_marshal(const cpu_state_t* cpu, os_net_relay_request_t* req) {
    req->op = cpu->eax;
    if (net_relay_llm_supported(cpu->eax)) {
        uint32_t in = net_stack_bulk_in_size(cpu->eax), out = net_stack_bulk_out_size(cpu->eax);
        if ((in && !syscall_user_range((const void*)cpu->ebx, in, 0)) ||
            (out && !syscall_user_range((void*)cpu->ebx, out, 1)))
            return cpu->eax == SYS_LLM_ACQUIRE_START ? OS_LLM_ACQUIRE_BAD_REQUEST : OS_LLM_REQUEST_BAD_REQUEST;
        req->arg0 = in;
        req->out_capacity = (uint16_t)0U;
        req->arg1 = out;
        return 0;
    }
    switch (cpu->eax) {
        case SYS_SOCKET_OPEN:
            req->arg0 = cpu->ebx & 0xFFFFU; req->arg1 = cpu->ecx & 0xFFFFU; req->arg2 = cpu->edx;
            return 0;
        case SYS_SOCKET_LISTEN:
            req->arg0 = cpu->ebx & 0xFFFFU; req->arg1 = cpu->ecx;
            return 0;
        case SYS_SOCKET_CLOSE:
            req->arg0 = cpu->ebx;
            return 0;
        case SYS_SOCKET_ACCEPT_SYN_ACK:
            if (!syscall_user_range((const void*)cpu->ecx, sizeof(os_socket_syn_ack_t), 0))
                return OS_SOCKET_BAD_ARGUMENT;
            req->arg0 = cpu->ebx;
            req->in_length = (uint16_t)sizeof(os_socket_syn_ack_t);
            net_relay_copy(req->in, (const uint8_t*)cpu->ecx, req->in_length);
            return 0;
        case SYS_SOCKET_ACCEPT_SYN:
        case SYS_SOCKET_ACCEPT_ACK:
            if (!syscall_user_range((const void*)cpu->ecx, sizeof(os_socket_passive_view_t), 0))
                return OS_SOCKET_BAD_ARGUMENT;
            req->arg0 = cpu->ebx;
            req->in_length = (uint16_t)sizeof(os_socket_passive_view_t);
            net_relay_copy(req->in, (const uint8_t*)cpu->ecx, req->in_length);
            return 0;
        case SYS_SOCKET_BUILD_SYN_ACK:
            if (!syscall_user_range((void*)cpu->ecx, cpu->edx & 0xFFFFU, 1) ||
                !syscall_user_range((void*)cpu->esi, sizeof(uint16_t), 1))
                return OS_SOCKET_BAD_ARGUMENT;
            req->arg0 = cpu->ebx;
            req->out_capacity = net_relay_cap(cpu->edx & 0xFFFFU);
            return 0;
        case SYS_SOCKET_SEND: {
            const os_socket_send_request_t* r = (const os_socket_send_request_t*)cpu->ebx;
            if (!syscall_user_range(r, sizeof(*r), 0) ||
                !syscall_user_range(r->payload, r->length, 0) ||
                !syscall_user_range(r->segment, r->capacity, 1) ||
                !syscall_user_range(r->out_length, sizeof(*r->out_length), 1))
                return OS_SOCKET_BAD_ARGUMENT;
            if (r->length > OS_NET_RELAY_MAX_IN) return OS_SOCKET_BUFFER_SMALL;
            req->arg0 = (uint32_t)r->socket_id;
            req->in_length = r->length;
            net_relay_copy(req->in, r->payload, r->length);
            req->out_capacity = net_relay_cap(r->capacity);
            return 0;
        }
        case SYS_SOCKET_FEED: {
            const os_socket_feed_request_t* r = (const os_socket_feed_request_t*)cpu->ebx;
            if (!syscall_user_range(r, sizeof(*r), 0) ||
                !syscall_user_range(r->segment, r->length, 0))
                return OS_SOCKET_BAD_ARGUMENT;
            if (r->length > OS_NET_RELAY_MAX_IN) return OS_SOCKET_BUFFER_SMALL;
            req->arg0 = (uint32_t)r->socket_id;
            req->in_length = r->length;
            net_relay_copy(req->in, r->segment, r->length);
            return 0;
        }
        case SYS_SOCKET_RECEIVE: {
            const os_socket_receive_request_t* r = (const os_socket_receive_request_t*)cpu->ebx;
            if (!syscall_user_range(r, sizeof(*r), 0) ||
                !syscall_user_range(r->buffer, r->capacity, 1) ||
                !syscall_user_range(r->out_length, sizeof(*r->out_length), 1))
                return OS_SOCKET_BAD_ARGUMENT;
            req->arg0 = (uint32_t)r->socket_id;
            req->out_capacity = net_relay_cap(r->capacity);
            return 0;
        }
        case SYS_SOCKET_CONNECT:
            if (!syscall_user_range((const void*)cpu->ebx, sizeof(os_socket_connect_request_t), 0))
                return OS_SOCKET_BAD_ARGUMENT;
            req->in_length = (uint16_t)sizeof(os_socket_connect_request_t);
            net_relay_copy(req->in, (const uint8_t*)cpu->ebx, req->in_length);
            return 0;
        case SYS_PEER_LISTEN:
            if (!syscall_user_range((const void*)cpu->ebx, sizeof(os_peer_listen_request_t), 0))
                return OS_PEER_BAD_REQUEST;
            req->in_length = (uint16_t)sizeof(os_peer_listen_request_t);
            net_relay_copy(req->in, (const uint8_t*)cpu->ebx, req->in_length);
            return 0;
        case SYS_PEER_ACCEPT:
            if (!syscall_user_range((const void*)cpu->ebx, sizeof(os_peer_accept_request_t), 0))
                return OS_PEER_BAD_REQUEST;
            req->in_length = (uint16_t)sizeof(os_peer_accept_request_t);
            net_relay_copy(req->in, (const uint8_t*)cpu->ebx, req->in_length);
            return 0;
        case SYS_PEER_TLS_POLL:
            if (cpu->ebx && !syscall_user_range((const void*)cpu->ebx, sizeof(os_peer_tls_poll_request_t), 0))
                return OS_PEER_BAD_REQUEST;
            if (cpu->ebx) {
                req->in_length = (uint16_t)sizeof(os_peer_tls_poll_request_t);
                net_relay_copy(req->in, (const uint8_t*)cpu->ebx, req->in_length);
            } else {
                req->in_length = 0U;
            }
            return 0;
        default:
            return OS_SOCKET_BAD_ARGUMENT;
    }
}

/* Caller side of a DONE slot: copy the worker output back, return result. */
static int32_t net_relay_deliver(const cpu_state_t* cpu) {
    static uint8_t out[OS_NET_RELAY_MAX_OUT];
    uint32_t op = 0U, n = 0U;
    uint8_t* dst = 0;
    uint16_t* dst_len = 0;
    uint32_t cap = 0U;
    int32_t result;
    if (net_relay_llm_supported(cpu->eax)) {
        uint32_t want = net_stack_bulk_out_size(cpu->eax);
        if (want && syscall_user_range((void*)cpu->ebx, want, 1))
            (void)net_relay_bulk_result((int32_t)current_task->id, (uint8_t*)cpu->ebx, want);
        return net_relay_take((int32_t)current_task->id, &op, 0, 0U, &n);
    }
    result = net_relay_take((int32_t)current_task->id, &op, out, sizeof(out), &n);
    if (result != 0 || op != cpu->eax) return result;
    if (op == SYS_SOCKET_BUILD_SYN_ACK) {
        dst = (uint8_t*)cpu->ecx; cap = cpu->edx & 0xFFFFU; dst_len = (uint16_t*)cpu->esi;
    } else if (op == SYS_SOCKET_SEND) {
        const os_socket_send_request_t* r = (const os_socket_send_request_t*)cpu->ebx;
        if (!syscall_user_range(r, sizeof(*r), 0)) return OS_SOCKET_BAD_ARGUMENT;
        dst = r->segment; cap = r->capacity; dst_len = r->out_length;
    } else if (op == SYS_SOCKET_RECEIVE) {
        const os_socket_receive_request_t* r = (const os_socket_receive_request_t*)cpu->ebx;
        if (!syscall_user_range(r, sizeof(*r), 0)) return OS_SOCKET_BAD_ARGUMENT;
        dst = r->buffer; cap = r->capacity; dst_len = r->out_length;
    } else {
        return result;
    }
    if (n > cap || !syscall_user_range(dst, n, 1) ||
        !syscall_user_range(dst_len, sizeof(*dst_len), 1))
        return OS_SOCKET_BAD_ARGUMENT;
    net_relay_copy(dst, out, n);
    *dst_len = (uint16_t)n;
    return result;
}

/* 1 = handled (eax set or task rescheduled), 0 = run the syscall locally
 * (no worker any more and nothing in flight for this task). */
static int syscall_net_relay(cpu_state_t* cpu) {
    int32_t pid = (int32_t)current_task->id;
    uint32_t now = timer_get_ticks();
    uint32_t state = net_relay_state_for(pid);
    int32_t owner, worker, job;
    task_t* target;
    os_net_relay_request_t req;
    os_ipc_payload_t payload;
    int rc;

    if (state == NET_RELAY_SENT) {
        net_relay_note_poll();
        worker = net_relay_live_worker();
        if (worker != net_relay_worker()) {
            net_relay_fail(OS_NET_RELAY_ABORTED);
            print_string_serial("[NET] relay aborted: net-driver lost\n");
        } else if (net_relay_expired(now)) {
            net_relay_fail(OS_NET_RELAY_TIMEOUT);
            print_string_serial("[NET] relay timeout\n");
        } else {
            net_relay_wait(cpu);
            return 1;
        }
        state = NET_RELAY_DONE;
    }
    if (state == NET_RELAY_DONE) {
        cpu->eax = (uint32_t)net_relay_deliver(cpu);
        return 1;
    }
    owner = net_relay_owner();
    if (owner != 0) {
        target = get_task_by_id(owner);
        if (!target || target->state == TASK_TERMINATED) {
            net_relay_drop_owner();
        } else {
            net_relay_wait(cpu); /* one request in flight at a time */
            return 1;
        }
    }
    worker = net_relay_live_worker();
    if (worker <= 0 || worker == pid) return 0;
    target = get_task_by_id(worker);
    if (!target) return 0;
    memset(&req, 0, sizeof(req));
    rc = net_relay_marshal(cpu, &req);
    if (rc != 0) {
        cpu->eax = (uint32_t)rc;
        return 1;
    }
    job = net_relay_begin(pid, worker, cpu->eax, now);
    if (job <= 0) {
        net_relay_wait(cpu);
        return 1;
    }
    req.job_id = (uint32_t)job;
    if (req.arg0 && net_relay_llm_supported(req.op) &&
        net_relay_bulk_stage(pid, (const uint8_t*)cpu->ebx, req.arg0) != 0) {
        net_relay_cancel();
        cpu->eax = (uint32_t)OS_LLM_REQUEST_BAD_REQUEST;
        return 1;
    }
    memset(&payload, 0, sizeof(payload));
    payload.type = net_relay_peer_supported(req.op) ? OS_IPC_NET_PEER_RELAY_REQUEST : OS_IPC_NET_RELAY_REQUEST;
    payload.size = (uint32_t)sizeof(req);
    payload.request_id = (uint32_t)job;
    net_relay_copy(payload.data, (const uint8_t*)&req, sizeof(req));
    rc = ipc_endpoint_send(&target->ipc_endpoint, 0, &payload);
    if (rc == 0) task_ipc_message_queued(target);
    if (rc != 0) {
        net_relay_cancel();
        cpu->eax = (uint32_t)rc;
        return 1;
    }
    net_relay_wait(cpu);
    return 1;
}

/* Tranche 5 slice 3: SYS_NET_WIRE_* are reserved to the live net-driver
 * PID, even in degraded mode (no worker = nobody drives raw frames from a
 * syscall). Everything else is refused with OS_NET_WORKER_REQUIRED and
 * counted in os_net_wire_status_t.refused. */
static int net_wire_caller_is_worker(void) {
    int32_t worker = net_relay_live_worker();
    if (current_task && worker > 0 && (int32_t)current_task->id == worker) return 1;
    net_wire_note_refused();
    return 0;
}

/* Tranche 5 suite: SYS_NET_NIC. */
static int32_t sys_net_nic(cpu_state_t* cpu) {
    int32_t pid = current_task ? (int32_t)current_task->id : 0;
    int32_t worker = sys_service_lookup("net-driver");
    int rc;
    switch (cpu->ebx) {
        case OS_NET_NIC_STATUS: {
            os_net_nic_status_t* out = (os_net_nic_status_t*)cpu->ecx;
            if (!syscall_user_range(out, sizeof(*out), 1)) return OS_SOCKET_BAD_ARGUMENT;
            nic_owner_fill_status(out);
            return 0;
        }
        case OS_NET_NIC_CLAIM: {
            os_net_nic_info_t* info = (os_net_nic_info_t*)cpu->ecx;
            if (!current_task || current_task->type != TASK_TYPE_USER) return OS_NET_WORKER_REQUIRED;
            if (info && !syscall_user_range(info, sizeof(*info), 1)) return OS_SOCKET_BAD_ARGUMENT;
            rc = nic_owner_claim(pid, worker, kernel_net_nic_present());
            if (rc != 0) return rc;
            if (info) (void)kernel_net_nic_info(info);
            tss_set_nic_io(1); /* open now; task switches keep it per owner */
            print_string_serial("[NET] NE2000 ports 0x300-0x31F handed to the Ring 3 worker\n");
            return 0;
        }
        case OS_NET_NIC_IRQ:
            if (pid <= 0 || pid != worker || nic_owner_pid() != pid) return OS_NET_WORKER_REQUIRED;
            return (int32_t)nic_owner_irq_take(pid);
        case OS_NET_NIC_PUMP: {
            os_net_nic_pump_t* pump = (os_net_nic_pump_t*)cpu->ecx;
            if (pid <= 0 || pid != worker || nic_owner_pid() != pid) return OS_NET_WORKER_REQUIRED;
            if (!syscall_user_range(pump, sizeof(*pump), 1) ||
                !syscall_user_range(pump->tx, OS_NET_NIC_PUMP_TX_MAX * OS_NET_NIC_FRAME_MAX, 1) ||
                (pump->mode == OS_NET_NIC_PUMP_FRAME &&
                 !syscall_user_range(pump->rx, pump->rx_length, 0)))
                return OS_SOCKET_BAD_ARGUMENT;
            return kernel_net_nic_pump(pump);
        }
        case OS_NET_NIC_LOG: {
            const char* text = (const char*)cpu->ecx;
            uint32_t i, n = cpu->edx;
            if (pid <= 0 || pid != worker) return OS_NET_WORKER_REQUIRED;
            if (n > OS_NET_NIC_LOG_MAX || !syscall_user_range(text, n, 0)) return OS_SOCKET_BAD_ARGUMENT;
            for (i = 0U; i < n; i++) {
                print_char(text[i], -1, -1, 0x0F);
                write_serial(text[i]);
            }
            return 0;
        }
        case OS_NET_NIC_UTC: {
            char* out = (char*)cpu->ecx;
            if (pid <= 0 || pid != worker || nic_owner_pid() != pid) return OS_NET_WORKER_REQUIRED;
            if (!syscall_user_range(out, 16U, 1)) return OS_SOCKET_BAD_ARGUMENT;
            return kernel_net_utc(out, 16U);
        }
        case OS_NET_NIC_PUBLISH: {
            const os_net_stack_report_t* report = (const os_net_stack_report_t*)cpu->ecx;
            if (pid <= 0 || pid != worker) return OS_NET_WORKER_REQUIRED;
            if (!syscall_user_range(report, sizeof(*report), 0)) return OS_SOCKET_BAD_ARGUMENT;
            return nic_owner_publish(pid, report);
        }
        case OS_NET_NIC_STACK: {
            os_net_stack_report_t* out = (os_net_stack_report_t*)cpu->ecx;
            if (!syscall_user_range(out, sizeof(*out), 1)) return OS_SOCKET_BAD_ARGUMENT;
            return nic_owner_stack(out) ? 0 : OS_NET_NIC_ABSENT;
        }
        default:
            return OS_SOCKET_BAD_ARGUMENT;
    }
}

/* Tranche 5 pile: bulk side of a relayed LLM op (worker only). */
static int32_t sys_net_relay_bulk(cpu_state_t* cpu) {
    int32_t worker = net_relay_live_worker();
    int32_t pid = current_task ? (int32_t)current_task->id : 0;
    uint32_t n = cpu->esi;
    if (pid <= 0 || worker <= 0 || pid != worker) return OS_NET_WORKER_REQUIRED;
    if (n > OS_NET_RELAY_BULK_MAX) return OS_SOCKET_BUFFER_SMALL;
    if (cpu->ebx == OS_NET_RELAY_BULK_FETCH) {
        int got;
        if (!syscall_user_range((void*)cpu->edx, n, 1)) return OS_SOCKET_BAD_ARGUMENT;
        got = net_relay_bulk_fetch(pid, cpu->ecx, (uint8_t*)cpu->edx, n);
        return got < 0 ? OS_NET_RELAY_ABORTED : got;
    }
    if (cpu->ebx == OS_NET_RELAY_BULK_PUT) {
        if (!syscall_user_range((const void*)cpu->edx, n, 0)) return OS_SOCKET_BAD_ARGUMENT;
        return net_relay_bulk_put(pid, cpu->ecx, (const uint8_t*)cpu->edx, n) == 0 ? 0 : OS_NET_RELAY_ABORTED;
    }
    return OS_SOCKET_BAD_ARGUMENT;
}

static int sys_net_wire_connect(const os_net_wire_connect_t* user) {
    os_net_wire_connect_t request;
    if (!net_wire_caller_is_worker()) return OS_NET_WORKER_REQUIRED;
    if (!syscall_user_range(user, sizeof(*user), 0)) return OS_SOCKET_BAD_ARGUMENT;
    request = *user;
    return kernel_net_wire_connect(&request);
}

static int sys_net_wire_io(uint32_t op, const os_net_wire_io_t* user) {
    os_net_wire_io_t io;
    uint16_t length = 0U;
    uint16_t* lenp = &length;
    int rc;
    if (!net_wire_caller_is_worker()) return OS_NET_WORKER_REQUIRED;
    if (!syscall_user_range(user, sizeof(*user), 0)) return OS_SOCKET_BAD_ARGUMENT;
    io = *user;
    /* Tranche 5 suite: on the worker's NIC a RECV completes in a later pump,
     * so the engine writes the length straight into the worker's variable. */
    if (kernel_net_nic_port_mode() && io.rx_length) lenp = io.rx_length;
    if ((io.rx_capacity && !syscall_user_range(io.rx, io.rx_capacity, 1)) ||
        (io.rx_length && !syscall_user_range(io.rx_length, sizeof(*io.rx_length), 1)))
        return OS_SOCKET_BAD_ARGUMENT;
    if (op == SYS_NET_WIRE_SEND) {
        if (io.length > OS_NET_WIRE_MAX_IO) return OS_SOCKET_BUFFER_SMALL;
        if (!syscall_user_range(io.data, io.length, 0)) return OS_SOCKET_BAD_ARGUMENT;
        rc = kernel_net_wire_send(io.socket_id, io.data, io.length, io.rx, io.rx_capacity,
                                  lenp, io.attempts);
    } else {
        if (!io.rx || !io.rx_length) return OS_SOCKET_BAD_ARGUMENT;
        rc = kernel_net_wire_recv(io.socket_id, io.rx, io.rx_capacity, lenp, io.attempts);
    }
    if (io.rx_length && lenp == &length) *io.rx_length = length;
    return rc;
}

static int sys_net_wire_close(int socket_id) {
    if (!net_wire_caller_is_worker()) return OS_NET_WORKER_REQUIRED;
    return kernel_net_wire_close(socket_id);
}

static int sys_net_wire_status(os_net_wire_status_t* out) {
    if (!syscall_user_range(out, sizeof(*out), 1)) return OS_SOCKET_BAD_ARGUMENT;
    net_wire_fill_status(out, net_relay_live_worker());
    nic_owner_merge_wire(out); /* Tranche 5 pile: Ring 3 stack counters */
    return 0;
}

/* Public relayed connect. Reached here only by the worker itself (other
 * tasks are relayed) or, without a worker, by anyone: refused then. */
static int sys_socket_connect(const os_socket_connect_request_t* user) {
    os_net_wire_connect_t request;
    if (!net_wire_caller_is_worker()) return OS_NET_WORKER_REQUIRED;
    if (!syscall_user_range(user, sizeof(*user), 0)) return OS_SOCKET_BAD_ARGUMENT;
    request.local_port = user->local_port;
    request.remote_port = user->remote_port;
    request.local_ip[0] = user->local_ip[0]; request.local_ip[1] = user->local_ip[1];
    request.local_ip[2] = user->local_ip[2]; request.local_ip[3] = user->local_ip[3];
    request.remote_ip[0] = user->remote_ip[0]; request.remote_ip[1] = user->remote_ip[1];
    request.remote_ip[2] = user->remote_ip[2]; request.remote_ip[3] = user->remote_ip[3];
    request.local_sequence = user->local_sequence;
    request.attempts = user->attempts;
    return kernel_net_wire_connect(&request);
}

/* 1 in the default build (degraded mode runs the kernel stack), 0 when
 * built with NET_RING0_FALLBACK=0. kernel.c prints it at boot. */
int syscall_net_ring0_fallback_enabled(void) {
#ifdef MOHHDY_NET_NO_RING0_FALLBACK
    return 0;
#else
    return 1;
#endif
}

/* 1 when 109/110 may run the Ring 0 tokenizer and session. 0 in the default
 * strict build: those syscalls return OS_AI_GGUF_NO_WORKER instead. */
int syscall_gguf_ring0_fallback_enabled(void) {
#ifdef MOHHDY_GGUF_NO_RING0_FALLBACK
    return 0;
#else
    return 1;
#endif
}

static void serial_print_u32(uint32_t v) {
    char digits[12];
    int n = 0;
    do {
        digits[n++] = (char)('0' + (v % 10U));
        v /= 10U;
    } while (v != 0U && n < 11);
    while (n > 0) {
        char c[2];
        c[0] = digits[--n];
        c[1] = '\0';
        print_string_serial(c);
    }
}

#ifdef MOHHDY_NET_NO_RING0_FALLBACK
static uint32_t g_net_strict_refused;

/* Counts every refusal; the first 16 are logged on the serial line. */
static void syscall_net_strict_note_refused(uint32_t number) {
    char digits[12];
    int n = 0;
    uint32_t v;
    g_net_strict_refused++;
    if (g_net_strict_refused > 16U) return;
    print_string_serial("[NET] ring0 fallback absent: syscall ");
    v = number;
    do { digits[n++] = (char)('0' + v % 10U); v /= 10U; } while (v && n < 11);
    while (n > 0) { char c[2] = { digits[--n], 0 }; print_string_serial(c); }
    print_string_serial(" refused (-59) count ");
    v = g_net_strict_refused; n = 0;
    do { digits[n++] = (char)('0' + v % 10U); v /= 10U; } while (v && n < 11);
    while (n > 0) { char c[2] = { digits[--n], 0 }; print_string_serial(c); }
    print_string_serial("\n");
}
#endif

int sys_net_relay_reply(const os_net_relay_reply_t* reply) {
    int32_t worker = net_relay_live_worker();
    if (!current_task || worker <= 0 || (int32_t)current_task->id != worker)
        return OS_NET_WORKER_REQUIRED;
    if (!syscall_user_range(reply, sizeof(*reply), 0)) return OS_SOCKET_BAD_ARGUMENT;
    if (reply->out_length > OS_NET_RELAY_MAX_OUT) return OS_SOCKET_BAD_ARGUMENT;
    return net_relay_complete(worker, reply->job_id, reply->result, reply->out,
                              reply->out_length) == 0 ? 0 : OS_IPC_BAD_MESSAGE;
}

int sys_net_relay_status(os_net_relay_status_t* out) {
    if (!syscall_user_range(out, sizeof(*out), 1)) return OS_SOCKET_BAD_ARGUMENT;
    net_relay_fill_status(out, net_relay_live_worker());
    return 0;
}

void syscall_handler(cpu_state_t* cpu) {
    // Réactive les interruptions pour permettre au clavier de fonctionner
    asm volatile("sti");

    if (current_task) current_task->syscall_frame = cpu;
    /* Tranche 4 slice 3: while a task is blocked mid-FAT on a sector RPC,
     * any other task (except the driver) retries its syscall later: rewind
     * over "int 0x80" (2 bytes) and yield. */
    if (g_rpc_waiter && current_task && current_task != g_rpc_waiter &&
        (int32_t)current_task->id != ata_live_driver()) {
        cpu->eip -= 2U;
        schedule(cpu);
        return;
    }

    /* Maintenance réseau différée : aucune E/S DHCP n’est réalisée dans IRQ0. */
    (void)kernel_llm_dhcp_maintenance(timer_get_ticks());

    /* Tranche 5: with net-driver registered, network syscalls are reserved
     * to that worker PID. Degraded mode (no worker) is unchanged. */
    /* Tranche 5 slice 2: socket syscalls 99-108 of a non-worker task are
     * relayed to the worker over IPC instead of refused. A request already
     * in flight for this task is finished even if the worker just died. */
    if (current_task && net_relay_supported(cpu->eax) &&
        (net_relay_state_for((int32_t)current_task->id) != NET_RELAY_FREE ||
         !service_registry_net_syscall_allowed((int32_t)current_task->id, cpu->eax))) {
        if (syscall_net_relay(cpu)) return;
    }
    /* Tranche 5 pile: while the worker owns the NE2000 the whole stack
     * (ARP/IPv4/TCP/TLS) runs there, so LLM 91-98 of the other tasks are
     * relayed to it (bulk channel) instead of refused. Strict build: always
     * relayed to a live worker (loopback-only worker answers UNAVAILABLE). */
    if (current_task && net_relay_llm_supported(cpu->eax) &&
        (net_relay_state_for((int32_t)current_task->id) != NET_RELAY_FREE ||
         ((kernel_net_nic_port_mode() || !syscall_net_ring0_fallback_enabled()) &&
          !service_registry_net_syscall_allowed((int32_t)current_task->id, cpu->eax)))) {
        if (syscall_net_relay(cpu)) return;
    }
    if (!service_registry_net_syscall_allowed(current_task ? (int32_t)current_task->id : 0,
                                              cpu->eax)) {
        net_relay_note_denied();
        cpu->eax = (uint32_t)OS_NET_WORKER_REQUIRED;
        return;
    }
#ifdef MOHHDY_NET_NO_RING0_FALLBACK
    /* Strict network build (NET_RING0_FALLBACK=0): the kernel socket
     * registry, LLM session, peer TLS server and wire engine are never run
     * from a syscall. A gated call that was not relayed above to the live
     * Ring 3 networker is refused here, the worker included (it serves
     * sockets from its own Ring 3 stack). No worker = no network. */
    if (service_registry_net_syscall_gated(cpu->eax) ||
        (cpu->eax >= SYS_NET_WIRE_CONNECT && cpu->eax <= SYS_NET_WIRE_CLOSE)) {
        /* Wire 139-142 / connect 144 of a non-worker task: same refusal and
         * same counter (wire refused) as the legacy kernel. */
        if (((cpu->eax >= SYS_NET_WIRE_CONNECT && cpu->eax <= SYS_NET_WIRE_CLOSE) ||
             cpu->eax == SYS_SOCKET_CONNECT) && !net_wire_caller_is_worker()) {
            cpu->eax = (uint32_t)OS_NET_WORKER_REQUIRED;
            return;
        }
        net_relay_note_denied();
        syscall_net_strict_note_refused(cpu->eax);
        cpu->eax = (uint32_t)OS_NET_WORKER_REQUIRED;
        return;
    }
#endif
    /* Tranche 5 suite: if a kernel LLM (91-97) or peer (128-130) call was
     * not relayed above, it still drives the NE2000 from Ring 0. While the
     * worker owns the card that path is refused, worker included. Other
     * tasks reach the Ring 3 stack through the relay instead. */
    if (!nic_owner_kernel_may_touch() &&
        ((cpu->eax >= SYS_LLM_ACQUIRE_START && cpu->eax <= SYS_LLM_CLOSE) ||
         (cpu->eax >= SYS_PEER_LISTEN && cpu->eax <= SYS_PEER_TLS_POLL))) {
        nic_owner_note_kernel_gated();
        cpu->eax = (uint32_t)OS_NET_NIC_WORKER_OWNED;
        return;
    }

    // Le numéro de syscall est dans le registre EAX
    switch (cpu->eax) {
        case SYS_EXIT:
            service_notify_purge_pid(current_task->id);
            service_registry_backend_remove_pid(current_task->id);
            (void)service_registry_remove_watcher_pid(current_task->id);
            task_report_parent_exit(current_task, (int)cpu->ebx, OS_TASK_EVENT_EXITED);
            task_wake_waiter(current_task);
            task_reparent_children(current_task);
            current_task->state = TASK_TERMINATED;
            print_string_serial("[EXIT] task terminated, scheduling...\n");
            schedule(cpu);
            break;
        
        case SYS_PUTC:
            {
                extern void write_serial(char a);
                print_char((char)cpu->ebx, -1, -1, 0x0F); // VGA
                write_serial((char)cpu->ebx);             // Serial
            }
            break;
            
        case SYS_GETC:
            {
                // Réactiver les interruptions avant de lire le clavier
                asm volatile("sti");
                
                // Lecture clavier (ASCII)
                char c = keyboard_getc();
                cpu->eax = c;
                
                // Log uniquement si caractère non nul pour éviter le bruit
                if (c != 0) {
                    print_string_serial("SYS_GETC: caractère retourné: '");
                    write_serial(c);
                    print_string_serial("'\n");
                }
            }
            break;
            
        case SYS_PUTS:
            {
                char* str = (char*)cpu->ebx;
                if (str) {
                    for (int i = 0; i < 1024 && str[i] != '\0'; i++) {
                        char ch = str[i];
                        // Filtrer les non-imprimables (sauf \n, \r, \t)
                        if ((ch >= 32 && ch <= 126) || ch == '\n' || ch == '\r' || ch == '\t') {
                            print_char(ch, -1, -1, 0x0F);
                        }
                    }
                    // Garantir un flush visuel minimal
                    print_char('\n', -1, -1, 0x0F);
                }
            }
            break;
            
        case SYS_YIELD:
            /* Cooperative switch from the int 0x80 user frame (safe).
             * Nested int 0x30 / IRQ0 is not used: that frame has no SS/ESP. */
            cpu->eax = 0;
            schedule(cpu);
            break;
            
        // SYS_GETS - Lire une ligne depuis le clavier
        case SYS_GETS:
            /* Yield while the line is incomplete so a Ring 3 worker (the GGUF
             * disk copy, atadriver) keeps running. keyboard_getc would busy-wait
             * inside this syscall, and IRQ0 does not preempt a syscall. */
            sys_gets_cooperative((char*)cpu->ebx, cpu->ecx, cpu);
            break;
            
        case SYS_EXEC:
            print_string_serial("[EXEC] starting child\n");
            {
                int rc = sys_exec((const char*)cpu->ebx, (char**)cpu->ecx);
                cpu->eax = (uint32_t)rc;
                if (rc >= 0) {
                    print_string_serial("[EXEC] waiting for child\n");
                    current_task->state = TASK_WAITING;
                    schedule(cpu);
                }
            }
            break;
        case SYS_SPAWN:
            print_string_serial("[SPAWN] starting child\n");
            cpu->eax = sys_spawn((const char*)cpu->ebx, (char**)cpu->ecx);
            print_string_serial("[SPAWN] child created\n");
            if ((int)cpu->eax >= 0) {
                schedule(cpu);
            }
            break;
        case SYS_LISTDIR:
            /* AOS-2178: overlay part worker-mediated when vfs-virtual is live. */
            cpu->eax = (uint32_t)sys_listdir_historical((const char*)cpu->ebx, (os_dirent_t*)cpu->ecx, (int)cpu->edx);
            break;
        case SYS_READFILE:
            /* AOS-2177: overlay part worker-mediated when vfs-virtual is live. */
            cpu->eax = (uint32_t)sys_readfile_historical((const char*)cpu->ebx, (char*)cpu->ecx, cpu->edx);
            break;
        case SYS_GETPID:
            cpu->eax = (uint32_t)sys_getpid();
            break;
        case SYS_PS:
            cpu->eax = (uint32_t)sys_ps((os_proc_t*)cpu->ebx, (int)cpu->ecx);
            break;
        case SYS_KILL:
            cpu->eax = (uint32_t)sys_kill((int)cpu->ebx);
            break;
        case SYS_TICKS:
            cpu->eax = sys_ticks();
            break;
        case SYS_MEMINFO:
            cpu->eax = (uint32_t)sys_meminfo((os_meminfo_t*)cpu->ebx);
            break;
        case SYS_TASK_METRICS:
            cpu->eax = (uint32_t)sys_task_metrics((int)cpu->ebx, (os_task_metrics_t*)cpu->ecx);
            break;
        case SYS_TASK_SET_PRIORITY:
            cpu->eax = (uint32_t)sys_task_set_priority((int)cpu->ebx, cpu->ecx);
            break;
        case SYS_TASK_WAIT:
            cpu->eax = (uint32_t)sys_task_wait((int)cpu->ebx);
            if ((int)cpu->eax == 0) schedule(cpu);
            break;
        case SYS_TASK_SET_NAME:
            cpu->eax = (uint32_t)sys_task_set_name((int)cpu->ebx, (const char*)cpu->ecx);
            break;
        case SYS_TASK_CAPACITY:
            cpu->eax = (uint32_t)sys_task_capacity((os_task_capacity_t*)cpu->ebx);
            break;
        case SYS_TASK_CHILD_RESULT:
            cpu->eax = (uint32_t)sys_task_child_result((int)cpu->ebx,
                                                        (os_task_exit_result_t*)cpu->ecx);
            break;
        case SYS_TASK_CHILD_RESULT_LIST:
            cpu->eax = (uint32_t)sys_task_child_result_list((os_task_exit_history_t*)cpu->ebx);
            break;
        case SYS_TASK_CHILD_RESULT_ACK:
            cpu->eax = (uint32_t)sys_task_child_result_ack();
            break;
        case SYS_TASK_CHILD_RESULT_OBSERVE:
            cpu->eax = (uint32_t)sys_task_child_result_observe(cpu->ebx,
                (os_task_exit_history_observation_t*)cpu->ecx);
            break;
        case SYS_TASK_CHILD_RESULT_FIND:
            cpu->eax = (uint32_t)sys_task_child_result_find((int)cpu->ebx,
                (os_task_exit_result_t*)cpu->ecx);
            break;
        case SYS_TASK_CHILD_RESULT_FORGET:
            cpu->eax = (uint32_t)sys_task_child_result_forget((int)cpu->ebx);
            break;
        case SYS_TASK_SUSPEND:
            cpu->eax = (uint32_t)sys_task_suspend((int)cpu->ebx);
            break;
        case SYS_TASK_RESUME:
            cpu->eax = (uint32_t)sys_task_resume((int)cpu->ebx);
            break;
        case SYS_TASK_KILL_CHILDREN:
            cpu->eax = (uint32_t)sys_task_kill_children();
            break;
        case SYS_TASK_CHILDREN:
            cpu->eax = (uint32_t)sys_task_children((os_task_children_t*)cpu->ebx);
            break;
        case SYS_TASK_WAIT_ANY:
            cpu->eax = (uint32_t)sys_task_wait_any();
            if ((int)cpu->eax == 0) schedule(cpu);
            break;
        case SYS_TASK_CHILD_EXIT_COUNT:
            cpu->eax = (uint32_t)sys_task_child_exit_count((os_task_child_exit_count_t*)cpu->ebx);
            break;
        case SYS_TASK_DELEGATE_CHILD:
            cpu->eax = (uint32_t)sys_task_delegate_child((int)cpu->ebx, (int)cpu->ecx);
            break;
        case SYS_TASK_SUPERVISION_EVENTS:
            cpu->eax = (uint32_t)sys_task_supervision_events((os_task_supervision_events_t*)cpu->ebx);
            break;
        case SYS_TASK_SUPERVISION_EVENTS_ACK:
            cpu->eax = (uint32_t)sys_task_supervision_events_ack();
            break;
        case SYS_TASK_SUPERVISION_EVENTS_OBSERVE:
            cpu->eax = (uint32_t)sys_task_supervision_events_observe(cpu->ebx,
                (os_task_supervision_events_observation_t*)cpu->ecx);
            break;
        case SYS_TASK_SUPERVISION_EVENT_FIND:
            cpu->eax = (uint32_t)sys_task_supervision_event_find(cpu->ebx,
                (os_task_supervision_event_t*)cpu->ecx);
            break;
        case SYS_TASK_SUPERVISION_EVENT_FORGET:
            cpu->eax = (uint32_t)sys_task_supervision_event_forget(cpu->ebx);
            break;
        case SYS_TASK_SUPERVISION_SUMMARY:
            cpu->eax = (uint32_t)sys_task_supervision_summary(
                (os_task_supervision_summary_t*)cpu->ebx);
            break;
        case SYS_TASK_SUPERVISION_NOTIFY:
            cpu->eax = (uint32_t)sys_task_supervision_notify(cpu->ebx);
            break;
        case SYS_TASK_SUPERVISION_NOTIFY_FILTER:
            cpu->eax = (uint32_t)sys_task_supervision_notify_filter(cpu->ebx);
            break;
        case SYS_TASK_SUPERVISION_NOTIFY_STATUS:
            cpu->eax = (uint32_t)sys_task_supervision_notify_status(
                (os_task_supervision_notify_status_t*)cpu->ebx);
            break;
        case SYS_TASK_SUPERVISION_WATCH:
            cpu->eax = (uint32_t)sys_task_supervision_watch((int)cpu->ebx, cpu->ecx);
            break;
        case SYS_TASK_SUPERVISION_WATCH_STATUS:
            cpu->eax = (uint32_t)sys_task_supervision_watch_status(
                (os_task_supervision_watch_status_t*)cpu->ebx);
            break;
        case SYS_TASK_SUPERVISION_DELIVERY_STATS:
            cpu->eax = (uint32_t)sys_task_supervision_delivery_stats(
                (os_task_supervision_delivery_stats_t*)cpu->ebx);
            break;
        case SYS_TASK_SUPERVISION_DELIVERY_STATS_ACK:
            cpu->eax = (uint32_t)sys_task_supervision_delivery_stats_ack();
            break;
        case SYS_TASK_SUPERVISION_EVENT_REPLAY:
            cpu->eax = (uint32_t)sys_task_supervision_event_replay(cpu->ebx);
            break;
        case SYS_TASK_SUPERVISION_PRIORITY:
            cpu->eax = (uint32_t)sys_task_supervision_priority((int)cpu->ebx);
            break;
        case SYS_TASK_SUPERVISION_PRIORITY_STATUS:
            cpu->eax = (uint32_t)sys_task_supervision_priority_status(
                (os_task_supervision_priority_status_t*)cpu->ebx);
            break;
        case SYS_TASK_SUPERVISION_NOTIFY_BUDGET:
            cpu->eax = (uint32_t)sys_task_supervision_notify_budget(cpu->ebx);
            break;
        case SYS_TASK_SUPERVISION_NOTIFY_BUDGET_STATUS:
            cpu->eax = (uint32_t)sys_task_supervision_notify_budget_status(
                (os_task_supervision_notify_budget_status_t*)cpu->ebx);
            break;
        case SYS_FAT16_READ:
            cpu->eax = (uint32_t)sys_fat16_read((const char*)cpu->ebx,
                                                 (char*)cpu->ecx, cpu->edx);
            break;
        case SYS_FAT16_LIST:
            cpu->eax = (uint32_t)sys_fat16_list((os_fat16_dirent_t*)cpu->ebx, cpu->ecx);
            break;
        case SYS_FAT16_LIST_PAGE:
            cpu->eax = (uint32_t)sys_fat16_list_page((os_fat16_dirent_t*)cpu->ebx, cpu->ecx, cpu->edx);
            break;
        case SYS_FAT32_READ:
            cpu->eax = (uint32_t)sys_fat32_read((const char*)cpu->ebx, (char*)cpu->ecx, cpu->edx);
            break;
        case SYS_FAT32_LIST:
            cpu->eax = (uint32_t)sys_fat32_list((os_fat16_dirent_t*)cpu->ebx, cpu->ecx);
            break;
        case SYS_FAT32_LIST_PAGE:
            cpu->eax = (uint32_t)sys_fat32_list_page((os_fat16_dirent_t*)cpu->ebx, cpu->ecx, cpu->edx);
            break;
        case SYS_FAT16_LIST_PATH:
            cpu->eax = (uint32_t)sys_fat16_list_path((const char*)cpu->ebx,
                                                       (os_fat16_dirent_t*)cpu->ecx, cpu->edx,
                                                       cpu->esi);
            break;
        case SYS_FAT32_LIST_PATH:
            cpu->eax = (uint32_t)sys_fat32_list_path((const char*)cpu->ebx,
                                                       (os_fat16_dirent_t*)cpu->ecx, cpu->edx,
                                                       cpu->esi);
            break;

        case SYS_SERVICE_BACKEND_RELEASE:
            cpu->eax = (uint32_t)sys_service_backend_release((const char*)cpu->ebx);
            break;
        case SYS_NET_RELAY_REPLY:
            cpu->eax = (uint32_t)sys_net_relay_reply((const os_net_relay_reply_t*)cpu->ebx);
            break;
        case SYS_NET_RELAY_STATUS:
            cpu->eax = (uint32_t)sys_net_relay_status((os_net_relay_status_t*)cpu->ebx);
            break;
        case SYS_NET_WIRE_CONNECT:
            cpu->eax = (uint32_t)sys_net_wire_connect((const os_net_wire_connect_t*)cpu->ebx);
            break;
        case SYS_NET_NIC:
            cpu->eax = (uint32_t)sys_net_nic(cpu);
            break;
        case SYS_NET_RELAY_BULK:
            cpu->eax = (uint32_t)sys_net_relay_bulk(cpu);
            break;
        case SYS_NET_WIRE_SEND:
        case SYS_NET_WIRE_RECV:
            cpu->eax = (uint32_t)sys_net_wire_io(cpu->eax, (const os_net_wire_io_t*)cpu->ebx);
            break;
        case SYS_NET_WIRE_CLOSE:
            cpu->eax = (uint32_t)sys_net_wire_close((int)cpu->ebx);
            break;
        case SYS_NET_WIRE_STATUS:
            cpu->eax = (uint32_t)sys_net_wire_status((os_net_wire_status_t*)cpu->ebx);
            break;
        case SYS_SOCKET_CONNECT:
            cpu->eax = (uint32_t)sys_socket_connect((const os_socket_connect_request_t*)cpu->ebx);
            break;
        case SYS_SOCKET_OPEN:
            cpu->eax = (uint32_t)sys_socket_open((uint16_t)cpu->ebx, (uint16_t)cpu->ecx, cpu->edx);
            break;
        case SYS_SOCKET_LISTEN:
            cpu->eax = (uint32_t)sys_socket_listen((uint16_t)cpu->ebx, cpu->ecx);
            break;
        case SYS_SOCKET_ACCEPT_SYN:
            cpu->eax = (uint32_t)sys_socket_accept_syn((int)cpu->ebx,
                (const os_socket_passive_view_t*)cpu->ecx);
            break;
        case SYS_SOCKET_BUILD_SYN_ACK:
            cpu->eax = (uint32_t)sys_socket_build_syn_ack((int)cpu->ebx,
                (uint8_t*)cpu->ecx, (uint16_t)cpu->edx, (uint16_t*)cpu->esi);
            break;
        case SYS_SOCKET_ACCEPT_ACK:
            cpu->eax = (uint32_t)sys_socket_accept_ack((int)cpu->ebx,
                (const os_socket_passive_view_t*)cpu->ecx);
            break;
        case SYS_SOCKET_ACCEPT_SYN_ACK:
            cpu->eax = (uint32_t)sys_socket_accept_syn_ack((int)cpu->ebx,
                (const os_socket_syn_ack_t*)cpu->ecx);
            break;
        case SYS_SOCKET_SEND:
            cpu->eax = (uint32_t)sys_socket_send((const os_socket_send_request_t*)cpu->ebx);
            break;
        case SYS_SOCKET_FEED:
            cpu->eax = (uint32_t)sys_socket_feed((const os_socket_feed_request_t*)cpu->ebx);
            break;
        case SYS_SOCKET_RECEIVE:
            cpu->eax = (uint32_t)sys_socket_receive((const os_socket_receive_request_t*)cpu->ebx);
            break;
        case SYS_SOCKET_CLOSE:
            cpu->eax = (uint32_t)sys_socket_close((int)cpu->ebx);
            break;
        case SYS_NET_STATUS:
            cpu->eax = kernel_net_status();
            break;
        case SYS_LLM_SESSION_STATUS:
            cpu->eax = nic_owner_llm_status(kernel_llm_session_status());
            break;
        case SYS_LLM_ACQUIRE_START:
            cpu->eax = (uint32_t)kernel_llm_acquire_start(
                (const os_llm_acquire_start_request_t*)cpu->ebx);
            break;
        case SYS_LLM_POLL_TLS:
            cpu->eax = (uint32_t)kernel_llm_poll_tls();
            break;
        case SYS_LLM_REQUEST:
            cpu->eax = (uint32_t)kernel_llm_request((const os_llm_request_t*)cpu->ebx);
            break;
        case SYS_LLM_POLL_TEXT:
            cpu->eax = (uint32_t)kernel_llm_poll_text((os_llm_text_result_t*)cpu->ebx);
            break;
        case SYS_LLM_POLL_SSE:
            cpu->eax = (uint32_t)kernel_llm_poll_sse((os_llm_text_result_t*)cpu->ebx);
            break;
        case SYS_LLM_RESET_FOR_REQUEST:
            cpu->eax = (uint32_t)kernel_llm_reset_for_request();
            break;
        case SYS_LLM_CLOSE:
            cpu->eax = (uint32_t)kernel_llm_close();
            break;
        case SYS_LLM_OPENAI_CREDENTIAL:
            cpu->eax = (uint32_t)kernel_llm_configure_openai((const os_llm_openai_credential_request_t*)cpu->ebx);
            break;
        case SYS_MKDIR:
            /* AOS-2178: historical overlay mutations need the live worker. */
            cpu->eax = historical_overlay_mutation_allowed()
                ? (uint32_t)sys_mkdir((const char*)cpu->ebx)
                : (uint32_t)OS_VFS_BACKEND_WORKER_REQUIRED;
            break;
        case SYS_UNLINK:
            cpu->eax = historical_overlay_mutation_allowed()
                ? (uint32_t)sys_unlink((const char*)cpu->ebx)
                : (uint32_t)OS_VFS_BACKEND_WORKER_REQUIRED;
            break;
        case SYS_WRITEFILE:
            /* AOS-2177: ATA-backed overlay write only via live storage worker. */
            cpu->eax = (uint32_t)sys_writefile_historical((const char*)cpu->ebx, (const char*)cpu->ecx, cpu->edx);
            break;
        case SYS_STAT:
            /* AOS-2178: overlay stat worker-mediated, initrd stat stays open. */
            cpu->eax = (uint32_t)sys_stat_historical((const char*)cpu->ebx, (os_dirent_t*)cpu->ecx);
            break;
        case SYS_RENAME:
            cpu->eax = historical_overlay_mutation_allowed()
                ? (uint32_t)sys_rename((const char*)cpu->ebx, (const char*)cpu->ecx)
                : (uint32_t)OS_VFS_BACKEND_WORKER_REQUIRED;
            break;
        case SYS_COPY:
            cpu->eax = historical_overlay_mutation_allowed()
                ? (uint32_t)sys_copy((const char*)cpu->ebx, (const char*)cpu->ecx)
                : (uint32_t)OS_VFS_BACKEND_WORKER_REQUIRED;
            break;
        case SYS_APPEND:
            cpu->eax = historical_overlay_mutation_allowed()
                ? (uint32_t)sys_append((const char*)cpu->ebx, (const char*)cpu->ecx, cpu->edx)
                : (uint32_t)OS_VFS_BACKEND_WORKER_REQUIRED;
            break;
        case SYS_GPT2_GENERATE:
            cpu->eax = (uint32_t)sys_gpt2_generate((const char*)cpu->ebx, (char*)cpu->ecx, cpu->edx);
            break;
        case SYS_GPT2_GGUF_GENERATE:
            cpu->eax = (uint32_t)sys_gpt2_gguf_generate((const char*)cpu->ebx, (char*)cpu->ecx, cpu->edx);
            break;
        case SYS_GPT2_GGUF_CONTINUE:
            cpu->eax = (uint32_t)sys_gpt2_gguf_continue((char*)cpu->ecx, cpu->edx);
            break;
        case SYS_IPC_SEND:
            cpu->eax = (uint32_t)sys_ipc_send((int)cpu->ebx,
                                               (const os_ipc_payload_t*)cpu->ecx);
            /* Le shell dort ensuite dans SYS_GETS (Ring 0) et ne peut pas être
             * préempté par IRQ0. Un handoff coopératif livre donc sans délai un
             * message à une autre tâche utilisateur déjà prête. */
            if ((int)cpu->eax == 0 && task_has_other_ready_user()) schedule(cpu);
            break;
        case SYS_IPC_RECV:
            cpu->eax = (uint32_t)sys_ipc_receive((os_ipc_message_t*)cpu->ebx);
            break;
        case SYS_IPC_RECV_WAIT:
            cpu->eax = (uint32_t)sys_ipc_receive_wait((os_ipc_message_t*)cpu->ebx, cpu->ecx);
            break;
        case SYS_AI_ENGINE:
            cpu->eax = (uint32_t)sys_ai_engine(cpu);
            break;
        case SYS_SERVICE_REGISTER:
            cpu->eax = (uint32_t)sys_service_register((const char*)cpu->ebx);
            break;
        case SYS_SERVICE_LOOKUP:
            cpu->eax = (uint32_t)sys_service_lookup((const char*)cpu->ebx);
            break;
        case SYS_SERVICE_UNREGISTER:
            cpu->eax = (uint32_t)sys_service_unregister((const char*)cpu->ebx);
            break;
        case SYS_SERVICE_GRANT:
            cpu->eax = (uint32_t)sys_service_grant((const char*)cpu->ebx, (int)cpu->ecx);
            if ((int)cpu->eax == 0 && task_has_other_ready_user()) schedule(cpu);
            break;
        case SYS_SERVICE_BACKEND_GRANT:
            cpu->eax = (uint32_t)sys_service_backend_grant((const char*)cpu->ebx, (int)cpu->ecx);
            if ((int)cpu->eax == 0 && task_has_other_ready_user()) schedule(cpu);
            break;
        case SYS_SERVICE_BACKEND_REVOKE:
            cpu->eax = (uint32_t)sys_service_backend_revoke((const char*)cpu->ebx, (int)cpu->ecx);
            if ((int)cpu->eax == 0 && task_has_other_ready_user()) schedule(cpu);
            break;
        case SYS_SERVICE_BACKEND_GRANT_SCOPED:
            cpu->eax = (uint32_t)sys_service_backend_grant_scoped((const char*)cpu->ebx, (int)cpu->ecx, cpu->edx);
            if ((int)cpu->eax == 0 && task_has_other_ready_user()) schedule(cpu);
            break;
        case SYS_SERVICE_BACKEND_GRANT_SCOPED_SOURCE:
            cpu->eax = (uint32_t)sys_service_backend_grant_scoped_source((const char*)cpu->ebx,
                                                                          (int)cpu->ecx, cpu->edx, cpu->esi);
            if ((int)cpu->eax == 0 && task_has_other_ready_user()) schedule(cpu);
            break;
        case SYS_SERVICE_BACKEND_SCOPE_STATUS:
            cpu->eax = (uint32_t)sys_service_backend_scope_status((const char*)cpu->ebx,
                                                                    (int)cpu->ecx,
                                                                    (os_service_backend_scope_t*)cpu->edx);
            break;
        case SYS_SERVICE_BACKEND_GRANT_SCOPED_SOURCE_PREFIX:
            cpu->eax = (uint32_t)sys_service_backend_grant_scoped_source_prefix((const char*)cpu->ebx,
                                                                                   (int)cpu->ecx, cpu->edx,
                                                                                   cpu->esi, (const char*)cpu->edi);
            if ((int)cpu->eax == 0 && task_has_other_ready_user()) schedule(cpu);
            break;
        case SYS_SERVICE_BACKEND_STATUS:
            cpu->eax = (uint32_t)sys_service_backend_status((const char*)cpu->ebx, (int)cpu->ecx, (uint32_t*)cpu->edx);
            break;
        case SYS_SERVICE_BACKEND_LIST:
            cpu->eax = (uint32_t)sys_service_backend_list((const char*)cpu->ebx, (os_service_backend_list_t*)cpu->ecx);
            break;
        case SYS_SERVICE_BACKEND_OBSERVE:
            cpu->eax = (uint32_t)sys_service_backend_observe((const char*)cpu->ebx, cpu->ecx,
                                                              (os_service_backend_snapshot_t*)cpu->edx);
            break;
        case SYS_SERVICE_NOTIFY:
            cpu->eax = (uint32_t)sys_service_notify((const char*)cpu->ebx);
            break;
        case SYS_SERVICE_STATUS:
            cpu->eax = (uint32_t)sys_service_status((const char*)cpu->ebx,
                                                    (os_service_status_t*)cpu->ecx);
            break;
        case SYS_VFS_BACKEND_READ:
            cpu->eax = (uint32_t)sys_vfs_backend_read((const char*)cpu->ebx,
                                                       (char*)cpu->ecx, cpu->edx);
            break;
        case SYS_VFS_BACKEND_WRITE:
            cpu->eax = (uint32_t)sys_vfs_backend_write((const char*)cpu->ebx,
                                                         (const char*)cpu->ecx, cpu->edx);
            break;
        case SYS_VFS_FAT16_CREATE:
            cpu->eax = (uint32_t)sys_vfs_fat16_create((const char*)cpu->ebx,
                                                       (const char*)cpu->ecx, cpu->edx);
            break;
        case SYS_VFS_FAT16_UNLINK:
            cpu->eax = (uint32_t)sys_vfs_fat16_unlink((const char*)cpu->ebx);
            break;
        case SYS_VFS_FAT16_RENAME:
            cpu->eax = (uint32_t)sys_vfs_fat16_rename((const char*)cpu->ebx,
                                                       (const char*)cpu->ecx);
            break;
        case SYS_VFS_FAT32_CREATE:
            cpu->eax = (uint32_t)sys_vfs_fat32_create((const char*)cpu->ebx,
                                                       (const char*)cpu->ecx, cpu->edx);
            break;
        case SYS_VFS_FAT32_UNLINK:
            cpu->eax = (uint32_t)sys_vfs_fat32_unlink((const char*)cpu->ebx);
            break;
        case SYS_VFS_FAT32_RENAME:
            cpu->eax = (uint32_t)sys_vfs_fat32_rename((const char*)cpu->ebx,
                                                       (const char*)cpu->ecx);
            break;
        case SYS_VFS_INITRD_READ:
            cpu->eax = (uint32_t)sys_vfs_initrd_read((const char*)cpu->ebx,
                                                      (char*)cpu->ecx, cpu->edx);
            break;
        case SYS_VFS_OVERLAY_READ:
            cpu->eax = (uint32_t)sys_vfs_overlay_read((const char*)cpu->ebx,
                                                       (char*)cpu->ecx, cpu->edx);
            break;
        case SYS_VFS_OVERLAY_UNLINK:
            cpu->eax = (uint32_t)sys_vfs_overlay_unlink((const char*)cpu->ebx);
            break;
        case SYS_VFS_OVERLAY_RENAME:
            cpu->eax = (uint32_t)sys_vfs_overlay_rename((const char*)cpu->ebx,
                                                         (const char*)cpu->ecx);
            break;
        case SYS_VFS_INITRD_STAT:
            cpu->eax = (uint32_t)sys_vfs_initrd_stat((const char*)cpu->ebx,
                                                      (os_dirent_t*)cpu->ecx);
            break;
        case SYS_VFS_OVERLAY_STAT:
            cpu->eax = (uint32_t)sys_vfs_overlay_stat((const char*)cpu->ebx,
                                                       (os_dirent_t*)cpu->ecx);
            break;
        case SYS_VFS_INITRD_LISTDIR:
            cpu->eax = (uint32_t)sys_vfs_initrd_listdir((const char*)cpu->ebx,
                                                         (os_dirent_t*)cpu->ecx,
                                                         (int)cpu->edx);
            break;
        case SYS_VFS_OVERLAY_LISTDIR:
            cpu->eax = (uint32_t)sys_vfs_overlay_listdir((const char*)cpu->ebx,
                                                           (os_dirent_t*)cpu->ecx,
                                                           (int)cpu->edx);
            break;
        case SYS_VFS_INITRD_LISTDIR_PAGE:
            cpu->eax = (uint32_t)sys_vfs_initrd_listdir_page((const char*)cpu->ebx,
                                                               (os_dirent_t*)cpu->ecx,
                                                               cpu->edx);
            break;
        case SYS_VFS_OVERLAY_LISTDIR_PAGE:
            cpu->eax = (uint32_t)sys_vfs_overlay_listdir_page((const char*)cpu->ebx,
                                                                (os_dirent_t*)cpu->ecx,
                                                                cpu->edx);
            break;
        case SYS_VFS_OVERLAY_MKDIR:
            cpu->eax = (uint32_t)sys_vfs_overlay_mkdir((const char*)cpu->ebx);
            break;
        case SYS_VFS_OVERLAY_RMDIR:
            cpu->eax = (uint32_t)sys_vfs_overlay_rmdir((const char*)cpu->ebx);
            break;
        case SYS_PEER_LISTEN:
            cpu->eax = (uint32_t)kernel_peer_listen((const os_peer_listen_request_t*)cpu->ebx);
            break;
        case SYS_PEER_ACCEPT:
            cpu->eax = (uint32_t)kernel_peer_accept((const os_peer_accept_request_t*)cpu->ebx);
            break;
        case SYS_PEER_TLS_POLL:
            cpu->eax = (uint32_t)kernel_peer_tls_poll((const os_peer_tls_poll_request_t*)cpu->ebx);
            break;
        case SYS_ATA_CLAIM:
            cpu->eax = (uint32_t)sys_ata_claim();
            break;
        case SYS_ATA_RELEASE:
            cpu->eax = (uint32_t)sys_ata_release();
            break;
        case SYS_ATA_JOB_FETCH:
            cpu->eax = (uint32_t)sys_ata_job_fetch((os_ata_job_t*)cpu->ebx, (uint8_t*)cpu->ecx);
            break;
        case SYS_ATA_JOB_DONE:
            cpu->eax = (uint32_t)sys_ata_job_done((const os_ata_job_t*)cpu->ebx, (int32_t)cpu->ecx,
                                                  (const uint8_t*)cpu->edx);
            /* Slice 3: resume the task blocked on this sector job (or on the
             * whole overlay flush) now; any accepted chunk counts as progress
             * for the stall timeout. */
            if (g_rpc_waiter && (int)cpu->eax >= 0) g_rpc_started = timer_get_ticks();
            if (g_rpc_waiter && g_rpc_waiter->state == TASK_BLOCKED_KERNEL &&
                ((g_rpc_kind == ATA_RPC_IO &&
                  (ata_job_io_state() == ATA_IO_DONE || ata_job_io_state() == ATA_IO_FAILED)) ||
                 (g_rpc_kind == ATA_RPC_FLUSH && (int)cpu->eax == OS_ATA_JOB_FLUSH_DONE &&
                  !ata_job_flush_queued()))) {
                g_rpc_waiter->state = TASK_READY;
                task_sched_only = g_rpc_waiter;
                schedule(cpu);
            }
            break;
        case SYS_ATA_STATUS:
            cpu->eax = (uint32_t)sys_ata_status((os_ata_status_t*)cpu->ebx);
            break;
        case SYS_ATA_FS:
            cpu->eax = (uint32_t)sys_ata_fs(cpu);
            break;
        case SYS_ATA_DEBUG:
            /* Test hook, root shell only (the task the kernel started). */
            if (!current_task || task_root_shell_pid() <= 0 ||
                current_task->id != task_root_shell_pid() || cpu->ebx != OS_ATA_DEBUG_CRASH_FAT_WRITE) {
                cpu->eax = (uint32_t)OS_TASK_CONTROL_DENIED;
            } else {
                ata_job_debug_arm_crash();
                print_string_serial("[ATA] debug: driver crash armed for next FAT write\n");
                cpu->eax = 0;
            }
            break;
        case SYS_SERVICE_BACKEND_TOKEN:
            cpu->eax = (uint32_t)sys_service_backend_token((const char*)cpu->ebx, (uint32_t*)cpu->ecx);
            break;
        case SYS_SERVICE_EVENT_PULL:
            cpu->eax = (uint32_t)sys_service_event_pull((os_service_event_pull_t*)cpu->ebx);
            break;
        case SYS_MOUNT_JOURNAL:
            cpu->eax = (uint32_t)sys_mount_journal(cpu->ebx, cpu->ecx, cpu->edx);
            break;
        case SYS_TASK_IDENTITY_KEY:
            cpu->eax = (uint32_t)sys_task_identity_key_read((uint32_t*)cpu->ebx);
            break;
        case SYS_IPC_SPILL_DROPS:
            cpu->eax = (uint32_t)sys_ipc_spill_drops((os_ipc_spill_drops_t*)cpu->ebx);
            break;
        case SYS_SERVICE_RIGHT_TOKEN:
            cpu->eax = (uint32_t)sys_service_right_token((const char*)cpu->ebx, (uint32_t*)cpu->ecx);
            break;
        case SYS_VGA_BLIT:
            {
                if (!cpu->ebx) {
                    gfx_fb_leave();
                    cpu->eax = 0;
                    break;
                }
                {
                    const os_fb_scene_t *scene = (const os_fb_scene_t *)cpu->ebx;
                    if (scene->magic == OS_FB_MAGIC) {
                        cpu->eax = (uint32_t)gfx_fb_present(scene);
                    } else {
                        const os_vga_frame_t *frame = (const os_vga_frame_t *)cpu->ebx;
                        vga_desktop_blit(frame->cells);
                        cpu->eax = 0;
                    }
                }
            }
            break;
        default:

            // Syscall inconnu
            break;
    }
}

int sys_ipc_send(int target_pid, const os_ipc_payload_t* payload) {
    task_t* target;
    if (!current_task || !payload || payload->size > OS_IPC_MAX_DATA) {
        return OS_IPC_BAD_MESSAGE;
    }
    target = get_task_by_id(target_pid);
    if (!target || target->type != TASK_TYPE_USER || target->state == TASK_TERMINATED) {
        return OS_IPC_BAD_TARGET;
    }
    if (service_registry_pid_is_owner(target_pid) &&
        target->ipc_endpoint.count >= IPC_SERVICE_ENDPOINT_CAPACITY) {
        return OS_IPC_SERVICE_FULL;
    }
    {
        int rc = ipc_endpoint_send(&target->ipc_endpoint, current_task->id, payload);
        if (rc == 0) task_ipc_message_queued(target);
        return rc;
    }
}

int sys_ipc_receive(os_ipc_message_t* out) {
    os_ipc_payload_t spilled;
    uint32_t i;
    int rc;
    if (!current_task || current_task->type != TASK_TYPE_USER || !out) {
        return OS_IPC_BAD_MESSAGE;
    }
    rc = ipc_endpoint_receive(&current_task->ipc_endpoint, out);
    if (rc != OS_IPC_EMPTY) return rc;
    if (service_registry_ipc_spill_pop(current_task->id, &spilled) != 0) return OS_IPC_EMPTY;
    out->sender_pid = 0;
    out->type = spilled.type;
    out->size = spilled.size;
    out->request_id = spilled.request_id;
    for (i = 0U; i < OS_IPC_MAX_DATA; i++) out->data[i] = spilled.data[i];
    return 0;
}

/* SYS_IPC_RECV_WAIT: blocking receive.
 *
 * Same mailbox and same rules as SYS_IPC_RECV (own endpoint, then the
 * spill), so no capability or right check changes: senders are still
 * checked in sys_ipc_send. With a message pending it returns at once. With
 * timeout 0 it is a poll (OS_IPC_EMPTY). Otherwise the task sleeps in
 * TASK_BLOCKED_IPC on its own kernel stack (kernel continuation, like the
 * ATA RPC) and is never selected by the scheduler, so an idle server costs
 * no CPU switch. A queued message (task_ipc_message_queued) or the IRQ0
 * deadline scan (task_ipc_wait_tick) makes it READY again; it then resumes
 * here and receives in its own address space. A killed waiter is simply
 * never resumed. */
static int ipc_recv_wait_may_block(const task_t* self) {
    if (!self || self->type != TASK_TYPE_USER || !self->syscall_frame) return 0;
    if (self->kctx_valid) return 0;
    /* The ATA RPC restricts the scheduler; its waiter/driver must not sleep. */
    if (g_rpc_waiter || task_sched_only) return 0;
    return 1;
}

int sys_ipc_receive_wait(os_ipc_message_t* out, uint32_t timeout) {
    task_t* self = current_task;
    int rc;
    if (!self || self->type != TASK_TYPE_USER || !out) return OS_IPC_BAD_MESSAGE;
    rc = sys_ipc_receive(out);
    if (ipc_wait_decide(rc, OS_IPC_EMPTY, timeout, ipc_recv_wait_may_block(self)) ==
        IPC_WAIT_RETURN) {
        return rc;
    }
    ipc_wait_begin(&self->ipc_wait, timer_get_ticks(), timeout, OS_IPC_WAIT_FOREVER);
    for (;;) {
        int final_rc = OS_IPC_EMPTY;
        self->state = TASK_BLOCKED_IPC;
        self->kctx_valid = 1U;
        if (kctx_save(self->kctx) == 0) {
            schedule(self->syscall_frame); /* never returns; resumed below */
        }
        self->kctx_valid = 0U;
        rc = sys_ipc_receive(out);
        if (ipc_wait_after_wake(&self->ipc_wait, rc, OS_IPC_EMPTY, OS_IPC_TIMEOUT,
                                timer_get_ticks(), &final_rc) == IPC_WAIT_RETURN) {
            return final_rc;
        }
    }
}

int sys_task_supervision_notify(uint32_t enabled) {
    if (!current_task || current_task->type != TASK_TYPE_USER) return OS_TASK_NOT_FOUND;
    return task_set_supervision_notify(current_task->id, enabled);
}

int sys_task_supervision_notify_filter(uint32_t mask) {
    if (!current_task || current_task->type != TASK_TYPE_USER) return OS_TASK_NOT_FOUND;
    return task_set_supervision_notify_filter(current_task->id, mask);
}

int sys_task_supervision_notify_status(os_task_supervision_notify_status_t* out) {
    if (!current_task || current_task->type != TASK_TYPE_USER || !out) return OS_TASK_NOT_FOUND;
    return task_fill_supervision_notify_status(current_task->id, out);
}

int sys_task_supervision_watch(int child_pid, uint32_t enabled) {
    if (!current_task || current_task->type != TASK_TYPE_USER) return OS_TASK_NOT_FOUND;
    return task_update_supervision_watch(current_task->id, child_pid, enabled);
}

int sys_task_supervision_watch_status(os_task_supervision_watch_status_t* out) {
    if (!current_task || current_task->type != TASK_TYPE_USER || !out) return OS_TASK_NOT_FOUND;
    return task_fill_supervision_watch_status(current_task->id, out);
}

int sys_task_supervision_delivery_stats(os_task_supervision_delivery_stats_t* out) {
    if (!current_task || current_task->type != TASK_TYPE_USER || !out) return OS_TASK_NOT_FOUND;
    return task_fill_supervision_delivery_stats(current_task->id, out);
}

int sys_task_supervision_delivery_stats_ack(void) {
    if (!current_task || current_task->type != TASK_TYPE_USER) return OS_TASK_NOT_FOUND;
    return task_ack_supervision_delivery_stats(current_task->id);
}

int sys_task_supervision_event_replay(uint32_t sequence) {
    if (!current_task || current_task->type != TASK_TYPE_USER) return OS_TASK_NOT_FOUND;
    return task_replay_supervision_event(current_task->id, sequence);
}

int sys_task_supervision_priority(int child_pid) {
    if (!current_task || current_task->type != TASK_TYPE_USER) return OS_TASK_NOT_FOUND;
    return task_set_supervision_priority(current_task->id, child_pid);
}

int sys_task_supervision_priority_status(os_task_supervision_priority_status_t* out) {
    if (!current_task || current_task->type != TASK_TYPE_USER || !out) return OS_TASK_NOT_FOUND;
    return task_fill_supervision_priority_status(current_task->id, out);
}

int sys_task_supervision_notify_budget(uint32_t limit) {
    if (!current_task || current_task->type != TASK_TYPE_USER) return OS_TASK_NOT_FOUND;
    return task_set_supervision_notify_budget(current_task->id, limit);
}

int sys_task_supervision_notify_budget_status(os_task_supervision_notify_budget_status_t* out) {
    if (!current_task || current_task->type != TASK_TYPE_USER || !out) return OS_TASK_NOT_FOUND;
    return task_fill_supervision_notify_budget_status(current_task->id, out);
}

static int vfs_backend_allowed_for_source(uint32_t right, uint32_t source);
static int vfs_backend_allowed_for_source_path(uint32_t right, uint32_t source,
                                               const char* path);

int sys_fat16_read(const char* name, char* buffer, uint32_t max) {
    if (!vfs_backend_allowed_for_source_path(SERVICE_BACKEND_RIGHT_READ,
                                             OS_SERVICE_BACKEND_SOURCE_FAT16, name))
        return OS_VFS_BACKEND_DENIED;
    if (!current_task || !service_registry_ata_overlay_io_via_worker(current_task->id))
        return OS_VFS_BACKEND_DENIED;
    if (!name || !buffer || max == 0U) return OS_FAT16_BAD_PATH;
    {
        int handled, rc = fsr_fat_read(OS_ATA_FSOP_FAT16_READ + 0U, name, buffer, max, &handled);
        if (handled) return rc;
    }
    return fat16_read_path(fat16_root(), name, buffer, max);
}

int sys_fat16_list(os_fat16_dirent_t* out, uint32_t capacity) {
    if (!vfs_backend_allowed_for_source(SERVICE_BACKEND_RIGHT_READ, OS_SERVICE_BACKEND_SOURCE_FAT16))
        return OS_VFS_BACKEND_DENIED;
    if (!current_task || !service_registry_ata_overlay_io_via_worker(current_task->id))
        return OS_VFS_BACKEND_DENIED;
    if (!out || capacity == 0U) return OS_FAT16_BAD_PATH;
    {
        int handled, rc = fsr_fat_list(OS_ATA_FSOP_FAT16_LIST + 0U, 0, out, capacity, 0U, &handled);
        if (handled) return rc;
    }
    return fat16_list_root(fat16_root(), out, capacity);
}

int sys_fat16_list_page(os_fat16_dirent_t* out, uint32_t capacity, uint32_t start) {
    if (!vfs_backend_allowed_for_source(SERVICE_BACKEND_RIGHT_READ, OS_SERVICE_BACKEND_SOURCE_FAT16))
        return OS_VFS_BACKEND_DENIED;
    if (!current_task || !service_registry_ata_overlay_io_via_worker(current_task->id))
        return OS_VFS_BACKEND_DENIED;
    if (!out || capacity == 0U) return OS_FAT16_BAD_PATH;
    {
        int handled, rc = fsr_fat_list(OS_ATA_FSOP_FAT16_LIST_PAGE + 0U, 0, out, capacity, start, &handled);
        if (handled) return rc;
    }
    return fat16_list_root_page(fat16_root(), start, out, capacity);
}

int sys_fat16_list_path(const char* path, os_fat16_dirent_t* out, uint32_t capacity,
                        uint32_t start) {
    if (!vfs_backend_allowed_for_source_path(SERVICE_BACKEND_RIGHT_READ,
                                             OS_SERVICE_BACKEND_SOURCE_FAT16, path))
        return OS_VFS_BACKEND_DENIED;
    if (!current_task || !service_registry_ata_overlay_io_via_worker(current_task->id))
        return OS_VFS_BACKEND_DENIED;
    if (!path || !out || capacity == 0U) return OS_FAT16_BAD_PATH;
    {
        int handled, rc = fsr_fat_list(OS_ATA_FSOP_FAT16_LIST_PATH + 0U, path, out, capacity, start, &handled);
        if (handled) return rc;
    }
    return fat16_list_path_page(fat16_root(), path, start, out, capacity);
}

int sys_fat32_read(const char* name, char* buffer, uint32_t max) {
    if (!vfs_backend_allowed_for_source_path(SERVICE_BACKEND_RIGHT_READ,
                                             OS_SERVICE_BACKEND_SOURCE_FAT32, name))
        return OS_VFS_BACKEND_DENIED;
    if (!current_task || !service_registry_ata_overlay_io_via_worker(current_task->id))
        return OS_VFS_BACKEND_DENIED;
    if (!name || !buffer || max == 0U) return OS_FAT16_BAD_PATH;
    {
        int handled, rc = fsr_fat_read(OS_ATA_FSOP_FAT16_READ + OS_ATA_FSOP_FAT32_BASE, name, buffer, max, &handled);
        if (handled) return rc;
    }
    return fat32_read_path(fat32_root(), name, (uint8_t*)buffer, max);
}

int sys_fat32_list(os_fat16_dirent_t* out, uint32_t capacity) {
    if (!vfs_backend_allowed_for_source(SERVICE_BACKEND_RIGHT_READ, OS_SERVICE_BACKEND_SOURCE_FAT32))
        return OS_VFS_BACKEND_DENIED;
    if (!current_task || !service_registry_ata_overlay_io_via_worker(current_task->id))
        return OS_VFS_BACKEND_DENIED;
    if (!out || capacity == 0U) return OS_FAT16_BAD_PATH;
    {
        int handled, rc = fsr_fat_list(OS_ATA_FSOP_FAT16_LIST + OS_ATA_FSOP_FAT32_BASE, 0, out, capacity, 0U, &handled);
        if (handled) return rc;
    }
    return fat32_list_root(fat32_root(), out, capacity);
}

int sys_fat32_list_page(os_fat16_dirent_t* out, uint32_t capacity, uint32_t start) {
    if (!vfs_backend_allowed_for_source(SERVICE_BACKEND_RIGHT_READ, OS_SERVICE_BACKEND_SOURCE_FAT32))
        return OS_VFS_BACKEND_DENIED;
    if (!current_task || !service_registry_ata_overlay_io_via_worker(current_task->id))
        return OS_VFS_BACKEND_DENIED;
    if (!out || capacity == 0U) return OS_FAT16_BAD_PATH;
    {
        int handled, rc = fsr_fat_list(OS_ATA_FSOP_FAT16_LIST_PAGE + OS_ATA_FSOP_FAT32_BASE, 0, out, capacity, start, &handled);
        if (handled) return rc;
    }
    return fat32_list_root_page(fat32_root(), start, out, capacity);
}

int sys_fat32_list_path(const char* path, os_fat16_dirent_t* out, uint32_t capacity,
                        uint32_t start) {
    if (!vfs_backend_allowed_for_source_path(SERVICE_BACKEND_RIGHT_READ,
                                             OS_SERVICE_BACKEND_SOURCE_FAT32, path))
        return OS_VFS_BACKEND_DENIED;
    if (!current_task || !service_registry_ata_overlay_io_via_worker(current_task->id))
        return OS_VFS_BACKEND_DENIED;
    if (!path || !out || capacity == 0U) return OS_FAT16_BAD_PATH;
    {
        int handled, rc = fsr_fat_list(OS_ATA_FSOP_FAT16_LIST_PATH + OS_ATA_FSOP_FAT32_BASE, path, out, capacity, start, &handled);
        if (handled) return rc;
    }
    return fat32_list_path_page(fat32_root(), path, start, out, capacity);
}
static int syscall_user_range(const void* pointer, uint32_t length, int write) {
    uint32_t start, end, address;
    page_t* page;
    if (!current_task || current_task->type != TASK_TYPE_USER || !current_task->vmm_dir || !pointer) return 0;
    start = (uint32_t)pointer;
    if (length == 0U) return 1;
    if (start > 0xffffffffU - (length - 1U)) return 0;
    end = start + length - 1U;
    if (start >= 0xc0000000U || end >= 0xc0000000U) return 0;
    for (address = start & ~(PAGE_SIZE - 1U); ; address += PAGE_SIZE) {
        page = vmm_get_page(address, 0, current_task->vmm_dir);
        if (!page || !page->present || !page->user || (write && !page->rw)) return 0;
        /* Stop after the page holding the last byte (the old '>' also
         * required the following page to be mapped, which rejected buffers
         * ending in the top user stack page). */
        if (address >= end - (end % PAGE_SIZE)) break;
        if (address > 0xffffffffU - PAGE_SIZE) return 0;
    }
    return 1;
}

int sys_socket_open(uint16_t local_port, uint16_t remote_port, uint32_t local_sequence) {
    if (!current_task || current_task->type != TASK_TYPE_USER) return OS_SOCKET_BAD_ARGUMENT;
    return net_socket_open(local_port, remote_port, local_sequence);
}

int sys_socket_listen(uint16_t local_port, uint32_t local_sequence) {
    if (!current_task || current_task->type != TASK_TYPE_USER) return OS_SOCKET_BAD_ARGUMENT;
    return net_socket_listen(local_port, local_sequence);
}

static int sys_socket_passive_view_copy(const os_socket_passive_view_t* source, net_tcp_view_t* target) {
    if (!syscall_user_range(source, sizeof(*source), 0) || !target) return OS_SOCKET_BAD_ARGUMENT;
    target->source_port = source->source_port; target->destination_port = source->destination_port;
    target->sequence = source->sequence; target->acknowledgment = source->acknowledgment;
    target->flags = source->flags; target->payload = 0; target->payload_length = 0U;
    return 0;
}

int sys_socket_accept_syn(int socket_id, const os_socket_passive_view_t* view) {
    net_tcp_view_t tcp_view;
    if (sys_socket_passive_view_copy(view, &tcp_view) != 0) return OS_SOCKET_BAD_ARGUMENT;
    return net_socket_accept_syn(socket_id, &tcp_view);
}

int sys_socket_build_syn_ack(int socket_id, uint8_t* segment, uint16_t capacity, uint16_t* out_length) {
    if (!syscall_user_range(segment, capacity, 1) || !syscall_user_range(out_length, sizeof(*out_length), 1)) return OS_SOCKET_BAD_ARGUMENT;
    return net_socket_build_syn_ack(socket_id, segment, capacity, out_length);
}

int sys_socket_accept_ack(int socket_id, const os_socket_passive_view_t* view) {
    net_tcp_view_t tcp_view;
    if (sys_socket_passive_view_copy(view, &tcp_view) != 0) return OS_SOCKET_BAD_ARGUMENT;
    return net_socket_accept_ack(socket_id, &tcp_view);
}

int sys_socket_accept_syn_ack(int socket_id, const os_socket_syn_ack_t* view) {
    net_tcp_view_t tcp_view;
    if (!syscall_user_range(view, sizeof(*view), 0)) return OS_SOCKET_BAD_ARGUMENT;
    tcp_view.source_port = view->source_port; tcp_view.destination_port = view->destination_port;
    tcp_view.sequence = view->sequence; tcp_view.acknowledgment = view->acknowledgment;
    tcp_view.flags = view->flags; tcp_view.payload = 0; tcp_view.payload_length = 0U;
    return net_socket_accept_syn_ack(socket_id, &tcp_view);
}

int sys_socket_send(const os_socket_send_request_t* request) {
    if (!syscall_user_range(request, sizeof(*request), 0) || !syscall_user_range(request->payload, request->length, 0) || !syscall_user_range(request->segment, request->capacity, 1) || !syscall_user_range(request->out_length, sizeof(*request->out_length), 1)) return OS_SOCKET_BAD_ARGUMENT;
    return net_socket_send(request->socket_id, request->payload, request->length, request->segment, request->capacity, request->out_length);
}

int sys_socket_feed(const os_socket_feed_request_t* request) {
    if (!syscall_user_range(request, sizeof(*request), 0) || !syscall_user_range(request->segment, request->length, 0)) return OS_SOCKET_BAD_ARGUMENT;
    return net_socket_feed(request->socket_id, request->segment, request->length);
}

int sys_socket_receive(const os_socket_receive_request_t* request) {
    if (!syscall_user_range(request, sizeof(*request), 0) || !syscall_user_range(request->buffer, request->capacity, 1) || !syscall_user_range(request->out_length, sizeof(*request->out_length), 1)) return OS_SOCKET_BAD_ARGUMENT;
    return net_socket_receive(request->socket_id, request->buffer, request->capacity, request->out_length);
}

int sys_socket_close(int socket_id) {
    if (!current_task || current_task->type != TASK_TYPE_USER) return OS_SOCKET_BAD_ARGUMENT;
    (void)net_wire_unbind(socket_id); /* slice 3: never leave a stale wire binding */
    return net_socket_close(socket_id);
}

int sys_service_register(const char* name) {
    int owner_pid;
    int rc;
    task_t* owner;
    if (!current_task || current_task->type != TASK_TYPE_USER) return OS_SERVICE_BAD_NAME;
    /* Tranche 4: ata-driver (Ring 3 ATA port capability) only for atadriver. */
    if (!service_registry_ata_driver_name_allowed(name, current_task->name))
        return (name && strcmp(name, "ai-engine") == 0) ? OS_AI_ENGINE_REQUIRED
                                                        : OS_ATA_DRIVER_REQUIRED;
    owner_pid = service_registry_lookup(name);
    if (owner_pid > 0) {
        owner = get_task_by_id(owner_pid);
        if (!owner || owner->type != TASK_TYPE_USER || owner->state == TASK_TERMINATED) {
            (void)service_registry_remove(name, owner_pid);
            service_notify_change(name, owner_pid, 0, OS_SERVICE_EVENT_PURGED);
            owner_pid = OS_SERVICE_NOT_FOUND;
        }
    }
    rc = service_registry_register(name, current_task->id);
    if (rc == 0 && owner_pid == OS_SERVICE_NOT_FOUND) {
        service_notify_change(name, 0, current_task->id, OS_SERVICE_EVENT_PUBLISHED);
    }
    /* Tranche 4 slice 2: a new driver first loads the overlay snapshot back
     * through its own Ring 3 PIO (kept only if RAM did not change meanwhile). */
    /* Slice 3: the atadriver spawned at boot skips it: the kernel loaded the
     * snapshot from the same disk just before any task existed and every
     * overlay write since then was persisted, so disk == RAM. */
    if (rc == 0 && strcmp(name, "ata-driver") == 0) {
        if (g_boot_driver_pid > 0 && (int32_t)current_task->id == g_boot_driver_pid &&
            !g_boot_driver_registered) {
            g_boot_driver_registered = 1;
            print_string_serial("[ATA] boot driver registered; kernel boot load kept\n");
        } else {
            ata_job_request_load();
        }
    }
    return rc;
}

int sys_service_lookup(const char* name) {
    int owner_pid = service_registry_lookup(name);
    task_t* owner;
    if (owner_pid < 0) return owner_pid;
    owner = get_task_by_id(owner_pid);
    if (!owner || owner->type != TASK_TYPE_USER || owner->state == TASK_TERMINATED) {
        (void)service_registry_remove(name, owner_pid);
        service_notify_change(name, owner_pid, 0, OS_SERVICE_EVENT_PURGED);
        return OS_SERVICE_NOT_FOUND;
    }
    return owner_pid;
}

int sys_service_unregister(const char* name) {
    int rc;
    if (!current_task || current_task->type != TASK_TYPE_USER) return OS_SERVICE_BAD_NAME;
    rc = service_registry_remove(name, current_task->id);
    if (rc == 0) service_notify_change(name, current_task->id, 0, OS_SERVICE_EVENT_UNREGISTERED);
    return rc;
}

int sys_service_grant(const char* name, int target_pid) {
    task_t* target;
    int rc;
    if (!current_task || current_task->type != TASK_TYPE_USER) return OS_SERVICE_BAD_NAME;
    target = get_task_by_id(target_pid);
    if (!target || target->type != TASK_TYPE_USER || target->state == TASK_TERMINATED) {
        return OS_SERVICE_BAD_GRANTEE;
    }
    /* Tranche 4: the ATA port capability never moves to another binary. */
    if (!service_registry_ata_driver_name_allowed(name, target->name))
        return OS_ATA_DRIVER_REQUIRED;
    rc = service_registry_grant(name, current_task->id, target_pid);
    if (rc == 0) service_notify_change(name, current_task->id, target_pid, OS_SERVICE_EVENT_GRANTED);
    return rc;
}

int sys_service_backend_grant(const char* name, int target_pid) {
    task_t* target;
    if (!current_task || current_task->type != TASK_TYPE_USER) return OS_SERVICE_BAD_NAME;
    target = get_task_by_id(target_pid);
    if (!target || target->type != TASK_TYPE_USER || target->state == TASK_TERMINATED) {
        return OS_SERVICE_BAD_GRANTEE;
    }
    return service_registry_backend_grant(name, current_task->id, target_pid);
}

int sys_service_backend_grant_scoped(const char* name, int target_pid, uint32_t rights) {
    task_t* target;
    if (!current_task || current_task->type != TASK_TYPE_USER) return OS_SERVICE_BAD_NAME;
    target = get_task_by_id(target_pid);
    if (!target || target->type != TASK_TYPE_USER || target->state == TASK_TERMINATED) return OS_SERVICE_BAD_GRANTEE;
    return service_registry_backend_grant_scoped(name, current_task->id, target_pid, rights);
}

int sys_service_backend_grant_scoped_source(const char* name, int target_pid, uint32_t rights,
                                            uint32_t sources) {
    task_t* target;
    if (!current_task || current_task->type != TASK_TYPE_USER) return OS_SERVICE_BAD_NAME;
    target = get_task_by_id(target_pid);
    if (!target || target->type != TASK_TYPE_USER || target->state == TASK_TERMINATED) return OS_SERVICE_BAD_GRANTEE;
    return service_registry_backend_grant_scoped_source(name, current_task->id, target_pid, rights, sources);
}

int sys_service_backend_grant_scoped_source_prefix(const char* name, int target_pid,
                                                   uint32_t rights, uint32_t sources,
                                                   const char* prefix) {
    task_t* target;
    if (!current_task || current_task->type != TASK_TYPE_USER) return OS_SERVICE_BAD_NAME;
    target = get_task_by_id(target_pid);
    if (!target || target->type != TASK_TYPE_USER || target->state == TASK_TERMINATED) return OS_SERVICE_BAD_GRANTEE;
    return service_registry_backend_grant_scoped_source_prefix(name, current_task->id, target_pid,
                                                               rights, sources, prefix);
}

int sys_service_backend_revoke(const char* name, int target_pid) {
    task_t* target;
    if (!current_task || current_task->type != TASK_TYPE_USER) return OS_SERVICE_BAD_NAME;
    target = get_task_by_id(target_pid);
    if (!target || target->type != TASK_TYPE_USER || target->state == TASK_TERMINATED) return OS_SERVICE_BAD_GRANTEE;
    return service_registry_backend_revoke(name, current_task->id, target_pid);
}

int sys_service_backend_release(const char* name) {
    if (!current_task || current_task->type != TASK_TYPE_USER) return OS_SERVICE_BAD_NAME;
    return service_registry_backend_release(name, current_task->id);
}

int sys_service_backend_status(const char* name, int target_pid, uint32_t* out_rights) {
    task_t* target;
    if (!current_task || current_task->type != TASK_TYPE_USER || !out_rights) return OS_SERVICE_BAD_NAME;
    target = get_task_by_id(target_pid);
    if (!target || target->type != TASK_TYPE_USER || target->state == TASK_TERMINATED) return OS_SERVICE_BAD_GRANTEE;
    return service_registry_backend_rights(name, current_task->id, target_pid, out_rights);
}

int sys_task_identity_key_read(uint32_t* out) {
    if (!current_task || current_task->type != TASK_TYPE_USER || !out) return OS_TASK_NOT_FOUND;
    if (!syscall_user_range(out, sizeof(*out), 1)) return OS_TASK_NOT_FOUND;
    if (current_task->identity_key == 0U) return OS_TASK_NOT_FOUND;
    *out = current_task->identity_key;
    return 0;
}

int sys_ipc_spill_drops(os_ipc_spill_drops_t* out) {
    if (!current_task || current_task->type != TASK_TYPE_USER || !out) return OS_IPC_BAD_MESSAGE;
    if (!syscall_user_range(out, sizeof(*out), 1)) return OS_IPC_BAD_MESSAGE;
    out->service = service_registry_ipc_spill_drops();
    out->supervision = service_registry_ipc_spill_supervision_drops();
    return 0;
}

int sys_service_right_token(const char* name, uint32_t* out_token) {
    if (!current_task || current_task->type != TASK_TYPE_USER || !out_token) return OS_SERVICE_BAD_NAME;
    if (!syscall_user_range(out_token, sizeof(*out_token), 1)) return OS_SERVICE_BAD_NAME;
    return service_registry_right_token_of(name, current_task->id, out_token);
}

int sys_service_backend_token(const char* name, uint32_t* out_token) {
    if (!current_task || current_task->type != TASK_TYPE_USER || !out_token) return OS_SERVICE_BAD_NAME;
    if (!syscall_user_range(out_token, sizeof(*out_token), 1)) return OS_SERVICE_BAD_NAME;
    return service_registry_backend_token_of(name, current_task->id, out_token);
}

int sys_service_event_pull(os_service_event_pull_t* out) {
    service_registry_notify_event_t event;
    uint32_t i;
    int rc;
    if (!current_task || current_task->type != TASK_TYPE_USER || !out) return OS_SERVICE_BAD_NAME;
    if (!syscall_user_range(out, sizeof(*out), 1)) return OS_SERVICE_BAD_NAME;
    rc = service_registry_notify_pull(current_task->id, &event);
    if (rc != 0) return rc;
    for (i = 0U; i < OS_SERVICE_NAME_MAX; i++) out->name[i] = event.name[i];
    out->old_pid = event.old_pid;
    out->new_pid = event.new_pid;
    out->reason = event.reason;
    out->sequence = event.sequence;
    return 0;
}

int sys_mount_journal(uint32_t op, uint32_t arg1, uint32_t arg2) {
    if (!current_task || current_task->type != TASK_TYPE_USER) return OS_SERVICE_BAD_NAME;
    if (op == 1U) {
        if (arg2 == 0U || (arg2 & ~OS_SERVICE_BACKEND_SOURCE_ALL) != 0U) return OS_SERVICE_BAD_NAME;
        return service_registry_persistent_mount_add("vfs", (const char*)arg1, arg2);
    }
    if (op == 2U) {
        if (arg2 == 0U || !syscall_user_range((void*)arg1, arg2, 1)) return OS_SERVICE_BAD_NAME;
        return service_registry_mount_journal_format("vfs", (char*)arg1, arg2);
    }
    return OS_SERVICE_BAD_NAME;
}

int sys_service_backend_scope_status(const char* name, int target_pid,
                                     os_service_backend_scope_t* out_scope) {
    task_t* target;
    if (!current_task || current_task->type != TASK_TYPE_USER || !out_scope) return OS_SERVICE_BAD_NAME;
    target = get_task_by_id(target_pid);
    if (!target || target->type != TASK_TYPE_USER || target->state == TASK_TERMINATED) return OS_SERVICE_BAD_GRANTEE;
    return service_registry_backend_scope(name, current_task->id, target_pid, out_scope);
}

int sys_service_backend_list(const char* name, os_service_backend_list_t* out_list) {
    if (!current_task || current_task->type != TASK_TYPE_USER || !out_list) return OS_SERVICE_BAD_NAME;
    return service_registry_backend_list(name, current_task->id, out_list);
}

int sys_service_backend_observe(const char* name, uint32_t expected_generation,
                                os_service_backend_snapshot_t* out_snapshot) {
    if (!current_task || current_task->type != TASK_TYPE_USER || !out_snapshot) return OS_SERVICE_BAD_NAME;
    return service_registry_backend_observe(name, current_task->id, expected_generation, out_snapshot);
}

int sys_service_notify(const char* name) {
    if (!current_task || current_task->type != TASK_TYPE_USER) return OS_SERVICE_BAD_NAME;
    return service_registry_subscribe(name, current_task->id);
}

int sys_service_status(const char* name, os_service_status_t* out) {
    int owner_pid;
    task_t* owner;
    if (!current_task || current_task->type != TASK_TYPE_USER || !out) {
        return OS_SERVICE_BAD_NAME;
    }
    owner_pid = sys_service_lookup(name);
    if (owner_pid < 0) return owner_pid;
    owner = get_task_by_id(owner_pid);
    if (!owner || owner->type != TASK_TYPE_USER || owner->state == TASK_TERMINATED) {
        return OS_SERVICE_NOT_FOUND;
    }
    out->owner_pid = owner_pid;
    out->queued_messages = owner->ipc_endpoint.count;
    out->client_capacity = IPC_SERVICE_ENDPOINT_CAPACITY;
    out->endpoint_capacity = IPC_ENDPOINT_CAPACITY;
    return 0;
}

static int vfs_backend_allowed_for_source_path(uint32_t right, uint32_t source,
                                                   const char* path) {
    /* AOS-2172/2173/2174: owner bypass for valid scopes closes when vfs-virtual live. */
    return current_task && current_task->type == TASK_TYPE_USER &&
        (service_registry_owner_bypasses_backend("vfs", current_task->id, source) ||
         service_registry_backend_allowed_for_source_path("vfs", current_task->id, right,
                                                          source, path));
}

static int vfs_backend_allowed_for_source(uint32_t right, uint32_t source) {
    return vfs_backend_allowed_for_source_path(right, source, (const char*)0);
}

/* La lecture générique peut consulter overlay puis initrd : elle exige donc
 * toutes les sources. L’écriture générique est une primitive overlay seule. */
static int vfs_backend_allowed(uint32_t right, const char* path) {
    return vfs_backend_allowed_for_source_path(right, OS_SERVICE_BACKEND_SOURCE_ALL, path);
}

int sys_vfs_backend_read(const char* path, char* buffer, uint32_t max) {
    if (!vfs_backend_allowed(SERVICE_BACKEND_RIGHT_READ, path)) {
        return OS_VFS_BACKEND_DENIED;
    }
    return sys_readfile(path, buffer, max);
}

int sys_vfs_backend_write(const char* path, const char* data, uint32_t size) {
    if (!vfs_backend_allowed_for_source_path(SERVICE_BACKEND_RIGHT_MUTATE,
                                             OS_SERVICE_BACKEND_SOURCE_OVERLAY, path)) {
        return OS_VFS_BACKEND_DENIED;
    }
    /* AOS-2177: mutate grant ok, ATA overlay mutation needs the live worker. */
    if (!current_task || !service_registry_ata_overlay_io_via_worker(current_task->id)) {
        return OS_VFS_BACKEND_WORKER_REQUIRED;
    }
    return sys_writefile(path, data, size);
}

/* Tranche 4 suite: the FAT naming rules (8.3 / LFN alias / sub-directory
 * paths) moved unchanged to kernel/fs/fsop_exec.c, shared with the driver. */

/* Création FAT16 explicite : gère à la fois le format 8.3 classique et les noms longs LFN. */
int sys_vfs_fat16_create(const char* name, const char* data, uint32_t size) {
    if (!vfs_backend_allowed_for_source_path(SERVICE_BACKEND_RIGHT_MUTATE,
                                             OS_SERVICE_BACKEND_SOURCE_FAT16, name)) {
        return OS_VFS_BACKEND_DENIED;
    }
    if (!current_task || !service_registry_ata_overlay_io_via_worker(current_task->id)) {
        return OS_VFS_BACKEND_DENIED;
    }
    if (!name || (size != 0U && !data)) return OS_FAT16_BAD_PATH;
    {
        int handled, rc = fsr_fat_create(OS_ATA_FSOP_FAT16_CREATE + 0U, name, data, size, &handled);
        if (handled) return rc;
    }
    return fatvfs_fat16_create(fat16_root(), name, data, size, !data && size == 0U);
}

/* Suppression FAT16 explicite : supporte 8.3 et LFN. */
int sys_vfs_fat16_unlink(const char* name) {
    if (!vfs_backend_allowed_for_source_path(SERVICE_BACKEND_RIGHT_MUTATE,
                                             OS_SERVICE_BACKEND_SOURCE_FAT16, name)) {
        return OS_VFS_BACKEND_DENIED;
    }
    if (!current_task || !service_registry_ata_overlay_io_via_worker(current_task->id)) {
        return OS_VFS_BACKEND_DENIED;
    }
    if (!name) return OS_FAT16_BAD_PATH;
    {
        int handled, rc = fsr_fat_path(OS_ATA_FSOP_FAT16_UNLINK + 0U, name, 0, &handled);
        if (handled) return rc;
    }
    return fatvfs_fat16_unlink(fat16_root(), name);
}

/* Renommage FAT16 explicite : supporte 8.3 et LFN. */
int sys_vfs_fat16_rename(const char* old_name, const char* new_name) {
    if (!vfs_backend_allowed_for_source_path(SERVICE_BACKEND_RIGHT_MUTATE,
                                             OS_SERVICE_BACKEND_SOURCE_FAT16, old_name) ||
        !vfs_backend_allowed_for_source_path(SERVICE_BACKEND_RIGHT_MUTATE,
                                             OS_SERVICE_BACKEND_SOURCE_FAT16, new_name)) {
        return OS_VFS_BACKEND_DENIED;
    }
    if (!current_task || !service_registry_ata_overlay_io_via_worker(current_task->id)) {
        return OS_VFS_BACKEND_DENIED;
    }
    if (!old_name || !new_name) return OS_FAT16_BAD_PATH;
    {
        int handled, rc = fsr_fat_path(OS_ATA_FSOP_FAT16_RENAME + 0U, old_name, new_name, &handled);
        if (handled) return rc;
    }
    return fatvfs_fat16_rename(fat16_root(), old_name, new_name);
}

int sys_vfs_fat32_create(const char* name, const char* data, uint32_t size) {
    if (!vfs_backend_allowed_for_source_path(SERVICE_BACKEND_RIGHT_MUTATE,
                                             OS_SERVICE_BACKEND_SOURCE_FAT32, name)) {
        return OS_VFS_BACKEND_DENIED;
    }
    if (!current_task || !service_registry_ata_overlay_io_via_worker(current_task->id)) {
        return OS_VFS_BACKEND_DENIED;
    }
    if (!name || (size != 0U && !data)) return OS_FAT16_BAD_PATH;
    {
        int handled, rc = fsr_fat_create(OS_ATA_FSOP_FAT16_CREATE + OS_ATA_FSOP_FAT32_BASE, name, data, size, &handled);
        if (handled) return rc;
    }
    return fatvfs_fat32_create(fat32_root(), name, data, size, !data && size == 0U);
}

int sys_vfs_fat32_unlink(const char* name) {
    if (!vfs_backend_allowed_for_source_path(SERVICE_BACKEND_RIGHT_MUTATE,
                                             OS_SERVICE_BACKEND_SOURCE_FAT32, name)) {
        return OS_VFS_BACKEND_DENIED;
    }
    if (!current_task || !service_registry_ata_overlay_io_via_worker(current_task->id)) {
        return OS_VFS_BACKEND_DENIED;
    }
    if (!name) return OS_FAT16_BAD_PATH;
    {
        int handled, rc = fsr_fat_path(OS_ATA_FSOP_FAT16_UNLINK + OS_ATA_FSOP_FAT32_BASE, name, 0, &handled);
        if (handled) return rc;
    }
    return fatvfs_fat32_unlink(fat32_root(), name);
}

int sys_vfs_fat32_rename(const char* old_name, const char* new_name) {
    if (!vfs_backend_allowed_for_source_path(SERVICE_BACKEND_RIGHT_MUTATE,
                                             OS_SERVICE_BACKEND_SOURCE_FAT32, old_name) ||
        !vfs_backend_allowed_for_source_path(SERVICE_BACKEND_RIGHT_MUTATE,
                                             OS_SERVICE_BACKEND_SOURCE_FAT32, new_name)) {
        return OS_VFS_BACKEND_DENIED;
    }
    if (!current_task || !service_registry_ata_overlay_io_via_worker(current_task->id)) {
        return OS_VFS_BACKEND_DENIED;
    }
    if (!old_name || !new_name) return OS_FAT16_BAD_PATH;
    {
        int handled, rc = fsr_fat_path(OS_ATA_FSOP_FAT16_RENAME + OS_ATA_FSOP_FAT32_BASE, old_name, new_name, &handled);
        if (handled) return rc;
    }
    return fatvfs_fat32_rename(fat32_root(), old_name, new_name);
}

int sys_vfs_initrd_read(const char* path, char* buffer, uint32_t max) {
    if (!vfs_backend_allowed_for_source_path(SERVICE_BACKEND_RIGHT_READ,
                                             OS_SERVICE_BACKEND_SOURCE_INITRD, path)) {
        return OS_VFS_BACKEND_DENIED;
    }
    if (!path || !buffer || max == 0U) return -1;
    return initrd_read_into(path, buffer, max);
}

int sys_vfs_overlay_read(const char* path, char* buffer, uint32_t max) {
    if (!vfs_backend_allowed_for_source_path(SERVICE_BACKEND_RIGHT_READ,
                                             OS_SERVICE_BACKEND_SOURCE_OVERLAY, path)) {
        return OS_VFS_BACKEND_DENIED;
    }
    /* AOS-2175: ATA-backed overlay read only via live storage worker. */
    if (!current_task || !service_registry_ata_overlay_io_via_worker(current_task->id)) {
        return OS_VFS_BACKEND_DENIED;
    }
    if (!path || !buffer || max == 0U) return -1;
    return ovs_read(path, buffer, max);
}

int sys_vfs_overlay_unlink(const char* path) {
    if (!vfs_backend_allowed_for_source_path(SERVICE_BACKEND_RIGHT_MUTATE,
                                             OS_SERVICE_BACKEND_SOURCE_OVERLAY, path)) {
        return OS_VFS_BACKEND_DENIED;
    }
    /* AOS-2177: mutate grant ok, ATA overlay mutation needs the live worker. */
    if (!current_task || !service_registry_ata_overlay_io_via_worker(current_task->id)) {
        return OS_VFS_BACKEND_WORKER_REQUIRED;
    }
    if (!path) return -1;
    return ovs_unlink(path);
}

int sys_vfs_overlay_rename(const char* oldpath, const char* newpath) {
    if (!vfs_backend_allowed_for_source_path(SERVICE_BACKEND_RIGHT_MUTATE,
                                             OS_SERVICE_BACKEND_SOURCE_OVERLAY, oldpath) ||
        !vfs_backend_allowed_for_source_path(SERVICE_BACKEND_RIGHT_MUTATE,
                                             OS_SERVICE_BACKEND_SOURCE_OVERLAY, newpath)) {
        return OS_VFS_BACKEND_DENIED;
    }
    /* AOS-2177: mutate grant ok, ATA overlay mutation needs the live worker. */
    if (!current_task || !service_registry_ata_overlay_io_via_worker(current_task->id)) {
        return OS_VFS_BACKEND_WORKER_REQUIRED;
    }
    if (!oldpath || !newpath) return -1;
    return ovs_rename(oldpath, newpath);
}

int sys_vfs_initrd_stat(const char* path, os_dirent_t* out) {
    if (!vfs_backend_allowed_for_source_path(SERVICE_BACKEND_RIGHT_READ,
                                             OS_SERVICE_BACKEND_SOURCE_INITRD, path)) {
        return OS_VFS_BACKEND_DENIED;
    }
    if (!path || !out) return -1;
    return initrd_stat(path, out);
}

int sys_vfs_overlay_stat(const char* path, os_dirent_t* out) {
    if (!vfs_backend_allowed_for_source_path(SERVICE_BACKEND_RIGHT_READ,
                                             OS_SERVICE_BACKEND_SOURCE_OVERLAY, path)) {
        return OS_VFS_BACKEND_DENIED;
    }
    /* AOS-2175: ATA-backed overlay stat only via live storage worker. */
    if (!current_task || !service_registry_ata_overlay_io_via_worker(current_task->id)) {
        return OS_VFS_BACKEND_DENIED;
    }
    if (!path || !out) return -1;
    return ovs_stat(path, out);
}

int sys_vfs_initrd_listdir(const char* path, os_dirent_t* out, int max_n) {
    if (!vfs_backend_allowed_for_source_path(SERVICE_BACKEND_RIGHT_READ,
                                             OS_SERVICE_BACKEND_SOURCE_INITRD, path)) {
        return OS_VFS_BACKEND_DENIED;
    }
    if (!path || !out || max_n <= 0 || !initrd_is_dir(path)) return -1;
    return initrd_listdir(path, out, max_n);
}

int sys_vfs_overlay_listdir(const char* path, os_dirent_t* out, int max_n) {
    if (!vfs_backend_allowed_for_source_path(SERVICE_BACKEND_RIGHT_READ,
                                             OS_SERVICE_BACKEND_SOURCE_OVERLAY, path)) {
        return OS_VFS_BACKEND_DENIED;
    }
    /* AOS-2178: ATA-backed overlay list only via live storage worker. */
    if (!current_task || !service_registry_ata_overlay_io_via_worker(current_task->id)) {
        return OS_VFS_BACKEND_WORKER_REQUIRED;
    }
    if (!path || !out || max_n <= 0 || !ovs_is_dir(path)) return -1;
    return ovs_listdir(path, out, 0, max_n);
}

int sys_vfs_initrd_listdir_page(const char* path, os_dirent_t* out, uint32_t start) {
    if (!vfs_backend_allowed_for_source_path(SERVICE_BACKEND_RIGHT_READ,
                                             OS_SERVICE_BACKEND_SOURCE_INITRD, path)) {
        return OS_VFS_BACKEND_DENIED;
    }
    if (!path || !out || !initrd_is_dir(path)) return -1;
    return initrd_listdir_page(path, out, start, 5);
}

int sys_vfs_overlay_listdir_page(const char* path, os_dirent_t* out, uint32_t start) {
    if (!vfs_backend_allowed_for_source_path(SERVICE_BACKEND_RIGHT_READ,
                                             OS_SERVICE_BACKEND_SOURCE_OVERLAY, path)) {
        return OS_VFS_BACKEND_DENIED;
    }
    /* AOS-2178: ATA-backed overlay list only via live storage worker. */
    if (!current_task || !service_registry_ata_overlay_io_via_worker(current_task->id)) {
        return OS_VFS_BACKEND_WORKER_REQUIRED;
    }
    if (!path || !out || !ovs_is_dir(path)) return -1;
    return ovs_listdir_page(path, out, start, 5);
}

int sys_vfs_overlay_mkdir(const char* path) {
    if (!vfs_backend_allowed_for_source_path(SERVICE_BACKEND_RIGHT_MUTATE,
                                             OS_SERVICE_BACKEND_SOURCE_OVERLAY, path)) {
        return OS_VFS_BACKEND_DENIED;
    }
    /* AOS-2177: mutate grant ok, ATA overlay mutation needs the live worker. */
    if (!current_task || !service_registry_ata_overlay_io_via_worker(current_task->id)) {
        return OS_VFS_BACKEND_WORKER_REQUIRED;
    }
    if (!path) return -1;
    return ovs_mkdir(path);
}

int sys_vfs_overlay_rmdir(const char* path) {
    if (!vfs_backend_allowed_for_source_path(SERVICE_BACKEND_RIGHT_MUTATE,
                                             OS_SERVICE_BACKEND_SOURCE_OVERLAY, path)) {
        return OS_VFS_BACKEND_DENIED;
    }
    /* AOS-2177: mutate grant ok, ATA overlay mutation needs the live worker. */
    if (!current_task || !service_registry_ata_overlay_io_via_worker(current_task->id)) {
        return OS_VFS_BACKEND_WORKER_REQUIRED;
    }
    if (!path || !ovs_is_dir(path)) return -1;
    return ovs_unlink(path);
}

/*
 * Generation locale GPT-2. Les tailles sont volontairement bornees : le
 * modele s'execute dans le noyau freestanding et ne doit jamais consommer un
 * buffer utilisateur non borne.
 */
/* GGUF 109/110 session. With a GGUF-ready aiworker the session lives in the
 * worker (it encodes the prompt, samples and decodes each piece) and this
 * is only the kernel mirror of its replies (status snapshot, adoption by a
 * restarted worker). The default build (GGUF_RING0_FALLBACK=0) refuses
 * 109/110 when no such worker is ready, or when the relayed job is aborted.
 * GGUF_RING0_FALLBACK=1 keeps the Ring 0 tokenizer and session
 * (kernel/llm/gpt2_gguf_session.c). Prompt normalisation stays Ring 0. */
static gpt2_gguf_session_t g_gguf_session;
static uint32_t g_gguf_session_next_id;

static int gguf_session_relay(uint32_t op, const char* prompt, char* out, uint32_t max,
                              int* result, int* fallback);
static int ai_gguf_kernel_next(void* context, const uint32_t* tokens, uint32_t token_count,
                               uint32_t generated_count, uint32_t* next_token,
                               uint32_t* rng_state);

static void gguf_session_snapshot(void) {
    ai_relay_record_gguf(OS_AI_PATH_NONE, 0, g_gguf_session.tokens, g_gguf_session.prompt_tokens,
                         g_gguf_session.token_count);
}

/* One Ring 0 step (path kernel, or fallback after an aborted relay). */
static int gguf_kernel_step(char* out, uint32_t max, int fallback) {
    uint32_t before = g_gguf_session.token_count;
    int rc = gpt2_gguf_session_step(&g_gguf_session, ai_gguf_kernel_next, &fallback, out, max);
    if (g_gguf_session.token_count != before) gguf_session_snapshot();
    return rc;
}

static int gguf_refuse_ring0(void) {
    ai_relay_note_gguf_refused(OS_AI_GGUF_NO_WORKER);
    return OS_AI_GGUF_NO_WORKER;
}

/* GGUF profile (SYS_GPT2_GGUF_GENERATE 109 / CONTINUE 110): the whole
 * session job goes to the GGUF-ready aiworker (path worker). Without one,
 * the strict build refuses; GGUF_RING0_FALLBACK=1 runs Ring 0. */
static int sys_gpt2_gguf_generate_impl(const char* prompt, char* out, uint32_t max) {
    char prompt_copy[GPT2_GENERATE_PROMPT_MAX];
    int result = 0, fallback = 0, rc;

    if (!prompt || !out || max < 2) return -1;
    g_gguf_session.active = 0U;
    (void)gpt2_generate_normalize(prompt, 255U, prompt_copy, sizeof(prompt_copy));
    g_gguf_session.id = ++g_gguf_session_next_id;
    if (gguf_session_relay(OS_AI_GGUF_SESSION_START, prompt_copy, out, max, &result, &fallback))
        return result;
    if (!syscall_gguf_ring0_fallback_enabled()) return gguf_refuse_ring0();
    rc = gpt2_gguf_session_start(&g_gguf_session, prompt_copy, gpt2_gguf_infer_ready(),
                                 GPT2_GGUF_INFER_MAX_CONTEXT);
    if (rc != 0) return rc;
    ai_relay_note_gguf_session_kernel();
    gguf_session_snapshot();
    return gguf_kernel_step(out, max, fallback);
}

static int sys_gpt2_gguf_continue_impl(char* out, uint32_t max) {
    int result = 0, fallback = 0;
    if (!out || max < 2U) return -1;
    if (!g_gguf_session.active) return -6;
    if (g_gguf_session.token_count == 0U || g_gguf_session.token_count >= GPT2_GGUF_SESSION_TOKENS) {
        g_gguf_session.active = 0U;
        out[0] = '\0';
        return 0;
    }
    if (gguf_session_relay(OS_AI_GGUF_SESSION_STEP, 0, out, max, &result, &fallback))
        return result;
    if (!syscall_gguf_ring0_fallback_enabled()) return gguf_refuse_ring0();
    return gguf_kernel_step(out, max, fallback);
}

/* ------------------------------------------------------------------------
 * Inventory item 4: Ring 3 GPT-2 FP32 worker ("ai-engine" / aiworker).
 * While the worker is registered, SYS_GPT2_GENERATE of any other user task
 * is relayed to it: the kernel keeps the normalised prompt in the relay
 * slot, rings the worker with one IPC doorbell (sender 0, no prompt bytes)
 * and blocks the caller in TASK_BLOCKED_KERNEL. The worker pulls the job
 * with OS_AI_ENGINE_FETCH, runs the same gpt2_generate_fp32() on the
 * checkpoint it mapped read-only (OS_AI_ENGINE_MAP) and answers with
 * OS_AI_ENGINE_REPLY, which wakes the caller. Worker lost or stalled: the
 * caller runs the Ring 0 path (fallback, counted). */
static task_t* g_ai_waiter;
static int32_t g_ai_mapped_pid;

/* Live ai-engine owner without the purge side effects of
 * sys_service_lookup (also called from the scheduler hook). */
static int32_t ai_live_worker(void) {
    int32_t pid = service_registry_lookup("ai-engine");
    task_t* t;
    if (pid <= 0) return 0;
    t = get_task_by_id(pid);
    if (!t || t->type != TASK_TYPE_USER || t->state == TASK_TERMINATED) return 0;
    return pid;
}

static void ai_relay_wake_caller(void) {
    task_t* waiter = g_ai_waiter;
    if (waiter && (int32_t)waiter->id == ai_relay_caller() &&
        waiter->state == TASK_BLOCKED_KERNEL)
        waiter->state = TASK_READY;
}

/* Called from IRQ0 (kernel/timer.c) every tick: a job in flight whose
 * worker died, was replaced or stalled past AI_RELAY_TIMEOUT_TICKS is
 * failed and its caller woken for the Ring 0 fallback; a caller that died
 * frees the slot. Returns 1 when it woke the caller. */
int syscall_ai_relay_watchdog(uint32_t now) {
    task_t* caller;
    if (ai_relay_state() != AI_RELAY_SENT) return 0;
    caller = get_task_by_id(ai_relay_caller());
    if (!caller || caller->state == TASK_TERMINATED) {
        ai_relay_drop_caller();
        g_ai_waiter = NULL;
        return 0;
    }
    if (!ai_relay_should_fail(ai_live_worker(), now)) return 0;
    {
        uint32_t kind = ai_relay_kind();
        ai_relay_fail();
        if (!syscall_gguf_ring0_fallback_enabled() &&
            (kind == OS_AI_JOB_GGUF_SESSION || kind == OS_AI_JOB_GGUF_STEP))
            print_string_serial("[AI] relay aborted: ai-engine lost or stalled; Ring 0 refused\n");
        else
            print_string_serial("[AI] relay aborted: ai-engine lost or stalled; Ring 0 fallback\n");
    }
    ai_relay_wake_caller();
    return 1;
}

/* One job in flight at a time: the busy caller retries its syscall later
 * (the 22/109/110 entry paths are idempotent up to this point). */
static void ai_relay_busy_retry(task_t* self) {
    self->syscall_frame->eip -= 2U;
    schedule(self->syscall_frame);
}

/* Rings the worker for the job just begun (words = job id, capacity, size),
 * blocks the caller until the slot is DONE or FAILED and takes it. Returns
 * AI_RELAY_DONE with *reply filled, else the caller runs the Ring 0 path. */
static uint32_t ai_relay_dispatch(task_t* self, task_t* worker, int32_t job,
                                  uint32_t capacity, uint32_t size,
                                  os_ai_engine_reply_t* reply) {
    os_ipc_payload_t payload;
    uint32_t words[3];
    int rc;
    memset(&payload, 0, sizeof(payload));
    payload.type = OS_IPC_AI_ENGINE_REQUEST;
    payload.request_id = (uint32_t)job;
    words[0] = (uint32_t)job;
    words[1] = capacity;
    words[2] = size;
    payload.size = (uint32_t)sizeof(words);
    memcpy(payload.data, words, sizeof(words));
    rc = ipc_endpoint_send(&worker->ipc_endpoint, 0, &payload);
    if (rc != 0) {
        uint32_t kind = ai_relay_kind();
        ai_relay_cancel();
        if (!syscall_gguf_ring0_fallback_enabled() &&
            (kind == OS_AI_JOB_GGUF_SESSION || kind == OS_AI_JOB_GGUF_STEP))
            print_string_serial("[AI] ai-engine mailbox refused the job; Ring 0 refused\n");
        else
            print_string_serial("[AI] ai-engine mailbox refused the job; Ring 0 fallback\n");
        return AI_RELAY_FAILED;
    }
    task_ipc_message_queued(worker);
    g_ai_waiter = self;
    for (;;) {
        if (ai_relay_state() != AI_RELAY_SENT || ai_relay_caller() != (int32_t)self->id) break;
        self->state = TASK_BLOCKED_KERNEL;
        self->kctx_valid = 1U;
        if (kctx_save(self->kctx) == 0) {
            schedule(self->syscall_frame); /* never returns; resumed below */
        }
        self->kctx_valid = 0U;
    }
    g_ai_waiter = NULL;
    memset(reply, 0, sizeof(*reply));
    return ai_relay_take((int32_t)self->id, reply);
}

/* Runs one relayed generation. Returns the worker result with *fallback 0,
 * or *fallback 1 when the caller must run the Ring 0 path. */
static int ai_relay_run(int32_t worker_pid, const char* prompt, char* out, uint32_t max,
                        int* fallback) {
    task_t* self = current_task;
    task_t* worker = get_task_by_id(worker_pid);
    os_ai_engine_reply_t reply;
    uint32_t length = 0U;
    uint32_t state;
    int32_t job;

    *fallback = 1;
    if (!self || !self->syscall_frame || !worker) return 0;
    job = ai_relay_begin((int32_t)self->id, worker_pid, prompt, max, timer_get_ticks());
    if (job <= 0) {
        ai_relay_busy_retry(self);
        return 0; /* not reached */
    }
    while (prompt[length] != '\0' && length < OS_AI_ENGINE_PROMPT_MAX) length++;
    state = ai_relay_dispatch(self, worker, job,
                              max > OS_AI_ENGINE_TEXT_MAX ? OS_AI_ENGINE_TEXT_MAX : max,
                              length, &reply);
    if (state != AI_RELAY_DONE) return 0;
    *fallback = 0;
    if (reply.result >= 0) {
        uint32_t n = reply.text_length;
        if (n + 1U > max) n = max - 1U;
        memcpy(out, reply.text, n);
        out[n] = '\0';
    }
    ai_relay_record_last(OS_AI_PATH_WORKER, reply.result, reply.tokens,
                         reply.prompt_tokens, reply.token_count);
    return reply.result;
}

/* SYS_GPT2_GENERATE (22), FP32 llm.c checkpoint. */
int sys_gpt2_generate(const char* prompt, char* out, uint32_t max) {
    char prompt_copy[GPT2_GENERATE_PROMPT_MAX];
    gpt2_generate_trace_t trace;
    int32_t worker;
    int fallback = 0;
    int rc;

    if (!prompt || !out || max < 2) return -1;
    if (current_task && current_task->type == TASK_TYPE_USER &&
        !syscall_user_range(out, max, 1))
        return -1;
    (void)gpt2_generate_normalize(prompt, 255U, prompt_copy, sizeof(prompt_copy));
    worker = ai_live_worker();
    if (worker > 0 && current_task && current_task->type == TASK_TYPE_USER) {
        /* The worker itself never borrows the Ring 0 engine. */
        if ((int32_t)current_task->id == worker) return OS_AI_ENGINE_REQUIRED;
        rc = ai_relay_run(worker, prompt_copy, out, max, &fallback);
        if (!fallback) return rc;
    }
    rc = gpt2_generate_fp32(prompt_copy, out, max, &trace);
    ai_relay_note_kernel_infer(ai_live_worker() > 0, fallback);
    ai_relay_record_last(fallback ? OS_AI_PATH_KERNEL_FALLBACK : OS_AI_PATH_KERNEL, rc,
                         trace.tokens, trace.prompt_tokens, trace.token_count);
    return rc;
}

/* Ring 0 sampling step for gpt2_gguf_session_step(): counted as path kernel
 * (or fallback when a relayed job was just aborted). */
static int ai_gguf_kernel_next(void* context, const uint32_t* tokens, uint32_t token_count,
                               uint32_t generated_count, uint32_t* next_token,
                               uint32_t* rng_state) {
    int fallback = context ? *(int*)context : 0;
    int rc = gpt2_gguf_generate_next_sampled(tokens, token_count, generated_count, next_token,
                                             rng_state);
    ai_relay_note_gguf_kernel(ai_relay_gguf_worker(ai_live_worker()) > 0, fallback);
    ai_relay_record_gguf(fallback ? OS_AI_PATH_KERNEL_FALLBACK : OS_AI_PATH_KERNEL, rc,
                         tokens, g_gguf_session.prompt_tokens, token_count);
    return rc;
}

/* 109/110 relayed to the GGUF-ready worker as one session job. Returns 1
 * with *result = the worker's answer (the reply is the new kernel mirror,
 * the piece is copied to out), or 0 when the caller runs Ring 0 (*fallback
 * 1 if the job was aborted: worker lost or stalled). */
static int gguf_session_relay(uint32_t op, const char* prompt, char* out, uint32_t max,
                              int* result, int* fallback) {
    int32_t worker = ai_live_worker();
    int32_t gguf_worker = ai_relay_gguf_worker(worker);
    task_t* self = current_task;
    task_t* target;
    os_ai_engine_reply_t reply;
    int32_t job;
    uint32_t i;
    *fallback = 0;
    if (gguf_worker <= 0 || !self || self->type != TASK_TYPE_USER ||
        (int32_t)self->id == gguf_worker || !self->syscall_frame)
        return 0;
    target = get_task_by_id(gguf_worker);
    if (!target) return 0;
    job = ai_relay_begin_gguf_session((int32_t)self->id, gguf_worker, op, g_gguf_session.id,
                                      prompt, max, g_gguf_session.tokens,
                                      g_gguf_session.token_count, g_gguf_session.prompt_tokens,
                                      g_gguf_session.rng, timer_get_ticks());
    if (job <= 0) {
        ai_relay_busy_retry(self);
        return 0; /* not reached */
    }
    if (ai_relay_dispatch(self, target, job, max, g_gguf_session.token_count, &reply) != AI_RELAY_DONE) {
        *fallback = 1;
        return 0;
    }
    for (i = 0U; i < GPT2_GGUF_SESSION_TOKENS; i++)
        g_gguf_session.tokens[i] = i < reply.token_count ? reply.tokens[i] : 0U;
    g_gguf_session.token_count = reply.token_count;
    g_gguf_session.prompt_tokens = reply.prompt_tokens;
    g_gguf_session.rng = reply.rng_state;
    g_gguf_session.active = reply.session_active;
    ai_relay_record_gguf(OS_AI_PATH_WORKER, reply.result, g_gguf_session.tokens,
                         g_gguf_session.prompt_tokens, g_gguf_session.token_count);
    memcpy(out, reply.text, reply.text_length);
    out[reply.text_length] = '\0';
    *result = reply.result;
    return 1;
}

/* OS_AI_ENGINE_GGUF_OPEN: a fresh worker-owned window (normal user pages,
 * freed at exit) sized to the kernel's FAT16 GGUF. */
static int32_t g_ai_gguf_pid;
static uint32_t g_ai_gguf_size;
static uint32_t g_ai_gguf_loaded;

static int ai_engine_gguf_open(int32_t pid, os_ai_engine_map_t* out) {
    uint32_t size = gpt2_gguf_infer_resident_size();
    uint32_t pages, i;
    if (size == 0U || size > OS_AI_ENGINE_GGUF_WINDOW_MAX) return OS_AI_ENGINE_NO_MODEL;
    if (g_ai_gguf_pid != pid) {
        g_ai_gguf_pid = pid;
        g_ai_gguf_size = 0U;
        g_ai_gguf_loaded = 0U;
        ai_relay_set_gguf_worker(0, 0U);
    }
    if (g_ai_gguf_size == 0U) {
        pages = (size + PAGE_SIZE - 1U) / PAGE_SIZE;
        for (i = 0U; i < pages; i++) {
            void* frame = pmm_alloc_page();
            if (!frame) return OS_AI_ENGINE_NO_MODEL; /* mapped pages go with the worker */
            memset(frame, 0, PAGE_SIZE); /* identity-mapped frame */
            if (vmm_map_page_in_directory(current_task->vmm_dir, frame,
                                          (void*)(OS_AI_ENGINE_GGUF_WINDOW + i * PAGE_SIZE),
                                          PAGE_PRESENT | PAGE_WRITE | PAGE_USER) != 0) {
                pmm_free_page(frame);
                return OS_AI_ENGINE_BAD_ARGUMENT;
            }
        }
        g_ai_gguf_size = size;
    }
    out->address = OS_AI_ENGINE_GGUF_WINDOW;
    out->size = size;
    return 0;
}

/* OS_AI_ENGINE_GGUF_READ: the bulk read restricted to the worker. The bytes
 * come from fat16_read_file_range_disk() on the boot FAT16 volume (sector
 * path, atadriver when that driver is live), even while the kernel snapshot
 * still exists. Destination is the worker window at the same offset. */
static int ai_engine_gguf_read(int32_t pid, uint32_t offset, uint32_t length) {
    uint32_t read = 0U;
    int status;
    if (pid != g_ai_gguf_pid || g_ai_gguf_size == 0U) return OS_AI_ENGINE_BAD_ARGUMENT;
    /* Sequential only: loaded == the prefix really copied, so READY can
     * trust it. */
    if (offset != g_ai_gguf_loaded || offset >= g_ai_gguf_size || length == 0U ||
        length > OS_AI_ENGINE_GGUF_CHUNK_MAX)
        return OS_AI_ENGINE_BAD_ARGUMENT;
    if (length > g_ai_gguf_size - offset) length = g_ai_gguf_size - offset;
    if (gpt2_gguf_infer_resident_size() != g_ai_gguf_size) return OS_AI_ENGINE_NO_MODEL;
    status = fat16_read_file_range_disk(fat16_root(), gpt2_gguf_infer_filename(), offset,
                                        (uint8_t*)(OS_AI_ENGINE_GGUF_WINDOW + offset), length, &read);
    if (status != 0) return OS_AI_ENGINE_NO_MODEL;
    g_ai_gguf_loaded += read;
    return (int)read;
}

/* OS_AI_ENGINE_MAP: the initrd blob (the same frames the Ring 0 fallback
 * reads) mapped read-only into the worker at a fixed window, PAGE_BORROWED
 * so the worker exit never frees initrd frames. The <4 KiB of initrd bytes
 * sharing the first/last page are visible too; the whole initrd is already
 * world-readable through SYS_READFILE. */
static int ai_engine_map(uint32_t which, os_ai_engine_map_t* out) {
    const char* path;
    uint32_t window, window_max, data, size, offset, pages, i;
    if (which == OS_AI_ENGINE_BLOB_CHECKPOINT) {
        path = "models/gpt2_124M.bin";
        window = OS_AI_ENGINE_CHECKPOINT_WINDOW;
        window_max = OS_AI_ENGINE_CHECKPOINT_WINDOW_MAX;
        if (!gpt2_model_current()->ready) return OS_AI_ENGINE_NO_MODEL;
    } else if (which == OS_AI_ENGINE_BLOB_TOKENIZER) {
        path = "models/gpt2_tokenizer.bin";
        window = OS_AI_ENGINE_TOKENIZER_WINDOW;
        window_max = OS_AI_ENGINE_TOKENIZER_WINDOW_MAX;
        if (!gpt2_tokenizer_ready()) return OS_AI_ENGINE_NO_MODEL;
    } else {
        return OS_AI_ENGINE_BAD_ARGUMENT;
    }
    data = (uint32_t)initrd_read_file(path);
    size = initrd_get_file_size(path);
    if (!data || size == 0U) return OS_AI_ENGINE_NO_MODEL;
    offset = data & (PAGE_SIZE - 1U);
    if (size > window_max - offset) return OS_AI_ENGINE_NO_MODEL;
    pages = (offset + size + PAGE_SIZE - 1U) / PAGE_SIZE;
    for (i = 0U; i < pages; i++) {
        if (vmm_map_borrowed_user_page(current_task->vmm_dir,
                                       (void*)((data & ~(PAGE_SIZE - 1U)) + i * PAGE_SIZE),
                                       (void*)(window + i * PAGE_SIZE)) != 0)
            return OS_AI_ENGINE_BAD_ARGUMENT;
    }
    out->address = window + offset;
    out->size = size;
    ai_relay_note_mapped(which, size);
    return 0;
}

static int32_t sys_ai_engine(cpu_state_t* cpu) {
    int32_t pid = current_task ? (int32_t)current_task->id : 0;
    int32_t worker = ai_live_worker();
    switch (cpu->ebx) {
        case OS_AI_ENGINE_STATUS: {
            os_ai_engine_status_t* out = (os_ai_engine_status_t*)cpu->ecx;
            if (!syscall_user_range(out, sizeof(*out), 1)) return OS_AI_ENGINE_BAD_ARGUMENT;
            ai_relay_fill_status(out, worker);
            if (worker <= 0 || worker != g_ai_mapped_pid) {
                out->checkpoint_mapped = 0U;
                out->tokenizer_mapped = 0U;
            }
            return 0;
        }
        case OS_AI_ENGINE_MAP: {
            os_ai_engine_map_t map;
            int rc;
            if (pid <= 0 || pid != worker) return OS_AI_ENGINE_REQUIRED;
            if (!syscall_user_range((void*)cpu->edx, sizeof(map), 1)) return OS_AI_ENGINE_BAD_ARGUMENT;
            if (g_ai_mapped_pid != pid) {
                ai_relay_worker_reset();
                g_ai_mapped_pid = pid;
            }
            rc = ai_engine_map(cpu->ecx, &map);
            if (rc == 0) memcpy((void*)cpu->edx, &map, sizeof(map));
            return rc;
        }
        case OS_AI_ENGINE_FETCH: {
            os_ai_engine_job_t job;
            int rc;
            if (pid <= 0 || pid != worker) return OS_AI_ENGINE_REQUIRED;
            if (!syscall_user_range((void*)cpu->edx, sizeof(job), 1)) return OS_AI_ENGINE_BAD_ARGUMENT;
            rc = ai_relay_fetch(pid, worker, cpu->ecx, &job);
            if (rc == 0) memcpy((void*)cpu->edx, &job, sizeof(job));
            return rc;
        }
        case OS_AI_ENGINE_REPLY: {
            static os_ai_engine_reply_t reply; /* kernel copy, ~800 bytes */
            int rc;
            if (pid <= 0 || worker <= 0 || pid != worker) {
                ai_relay_note_rogue();
                print_string_serial("[AI] rogue relay reply refused\n");
                return OS_AI_ENGINE_REQUIRED;
            }
            if (!syscall_user_range((const void*)cpu->ecx, sizeof(reply), 0)) return OS_AI_ENGINE_BAD_ARGUMENT;
            memcpy(&reply, (const void*)cpu->ecx, sizeof(reply));
            rc = ai_relay_complete(pid, worker, &reply);
            if (rc == OS_AI_ENGINE_STALE) print_string_serial("[AI] stale relay reply refused\n");
            if (rc == 0) ai_relay_wake_caller();
            return rc;
        }
        case OS_AI_ENGINE_LOG: {
            const char* text = (const char*)cpu->ecx;
            uint32_t i, n = cpu->edx;
            if (pid <= 0 || pid != worker) return OS_AI_ENGINE_REQUIRED;
            if (n > OS_AI_ENGINE_LOG_MAX || !syscall_user_range(text, n, 0)) return OS_AI_ENGINE_BAD_ARGUMENT;
            for (i = 0U; i < n; i++) {
                print_char(text[i], -1, -1, 0x0F);
                write_serial(text[i]);
            }
            return 0;
        }
        case OS_AI_ENGINE_GGUF_OPEN: {
            os_ai_engine_map_t map;
            int rc;
            if (pid <= 0 || pid != worker) return OS_AI_ENGINE_REQUIRED;
            if (!syscall_user_range((void*)cpu->ecx, sizeof(map), 1)) return OS_AI_ENGINE_BAD_ARGUMENT;
            rc = ai_engine_gguf_open(pid, &map);
            if (rc == 0) memcpy((void*)cpu->ecx, &map, sizeof(map));
            return rc;
        }
        case OS_AI_ENGINE_GGUF_READ:
            if (pid <= 0 || pid != worker) return OS_AI_ENGINE_REQUIRED;
            return ai_engine_gguf_read(pid, cpu->ecx, cpu->edx);
        case OS_AI_ENGINE_GGUF_READY:
            if (pid <= 0 || pid != worker) return OS_AI_ENGINE_REQUIRED;
            if (pid != g_ai_gguf_pid || g_ai_gguf_size == 0U || g_ai_gguf_loaded < g_ai_gguf_size)
                return OS_AI_ENGINE_BAD_ARGUMENT;
            {
                uint32_t pages;
                ai_relay_set_gguf_worker(pid, g_ai_gguf_loaded);
                pages = gpt2_gguf_infer_release_resident();
                if (pages != 0U) {
                    print_string_serial("[AI] GGUF resident released pages ");
                    serial_print_u32(pages);
                    print_string_serial("\n");
                }
                print_string_serial("[AI] ai-engine GGUF ready in Ring 3\n");
                return 0;
            }
        default:
            return OS_AI_ENGINE_BAD_ARGUMENT;
    }
}

int sys_gpt2_gguf_generate(const char* prompt, char* out, uint32_t max) {
    /* The worker never borrows the Ring 0 GGUF engine while it is live. */
    if (current_task && current_task->type == TASK_TYPE_USER &&
        (int32_t)current_task->id == ai_live_worker())
        return OS_AI_ENGINE_REQUIRED;
    return sys_gpt2_gguf_generate_impl(prompt, out, max);
}

int sys_gpt2_gguf_continue(char* out, uint32_t max) {
    if (current_task && current_task->type == TASK_TYPE_USER &&
        (int32_t)current_task->id == ai_live_worker())
        return OS_AI_ENGINE_REQUIRED;
    return sys_gpt2_gguf_continue_impl(out, max);
}

// Cette fonction est maintenant obsolète pour l'entrée clavier
void syscall_add_input_char(char c) {
    (void)c;
}

void syscall_init() {
    gpt2_gguf_session_reset(&g_gguf_session);
    g_gguf_session_next_id = 0U;
    ai_relay_init();
    g_ai_waiter = NULL;
    g_ai_mapped_pid = 0;
    // Enregistre notre handler pour l'interruption 0x80
    register_interrupt_handler(0x80, (interrupt_handler_t)syscall_handler);
}

/* SYS_EXEC : cree l'enfant, le parent passe TASK_WAITING. Le handler
 * appelle schedule() depuis le cadre user. SYS_EXIT reveille le waiter. */
int sys_exec(const char* path, char* argv[]) {
    int capacity_rc;
    task_t* new_task;
    if (!current_task) return OS_TASK_NOT_FOUND;
    capacity_rc = task_can_create_child(current_task->id);
    if (capacity_rc != 0) return capacity_rc;
    capacity_rc = task_can_create_global();
    if (capacity_rc != 0) return capacity_rc;
    new_task = create_task_from_initrd_file(path);

    if (!new_task) {
        return -1;
    }

    if (argv) {
        char** argv_list = (char**)argv;
        const char* src = 0;
        if (argv_list[1]) src = argv_list[1];
        else if (argv_list[0]) src = argv_list[0];
        if (src) {
            char kbuf[256];
            int n = 0;
            while (n < 255 && src[n] != '\0') { kbuf[n] = src[n]; n++; }
            kbuf[n] = '\0';
            extern vmm_directory_t* current_directory;
            vmm_directory_t* old_dir = current_directory;
            vmm_switch_page_directory(new_task->vmm_dir->physical_addr);
            current_directory = new_task->vmm_dir;
            char* dst = (char*)(0xB0000000 - 512);
            for (int i = 0; i <= n; i++) dst[i] = kbuf[i];
            vmm_switch_page_directory(old_dir->physical_addr);
            current_directory = old_dir;
            new_task->cpu_state.ebx = (uint32_t)(0xB0000000 - 512);
        }
    }
    new_task->parent_pid = current_task ? current_task->id : -1;
    new_task->waiter_pid = current_task ? current_task->id : 0;
    return 0;
}

/* Cree la tache et retourne son pid. Le handler appelle schedule() pour
 * laisser tourner l'enfant jusqu'au prochain SYS_YIELD (cadre user, pas IRQ0). */
int sys_spawn(const char* path, char* argv[]) {
    int capacity_rc;
    task_t* new_task;
    if (!current_task) return OS_TASK_NOT_FOUND;
    capacity_rc = task_can_create_child(current_task->id);
    if (capacity_rc != 0) return capacity_rc;
    capacity_rc = task_can_create_global();
    if (capacity_rc != 0) return capacity_rc;
    new_task = create_task_from_initrd_file(path);
    if (!new_task) {
        return -1;
    }
    // Passer au moins un argument texte (preferer argv[1] si present)
    if (argv) {
        char** argv_list = (char**)argv;
        const char* src = 0;
        if (argv_list[1]) src = argv_list[1];
        else if (argv_list[0]) src = argv_list[0];
        if (src) {
            // Copier jusqu'a 255 octets
            char kbuf[256];
            int n = 0;
            while (n < 255 && src[n] != '\0') { kbuf[n] = src[n]; n++; }
            kbuf[n] = '\0';
            // Ecrire dans la pile utilisateur de la nouvelle tache (en haut - 512)
            vmm_directory_t* old_dir = current_directory;
            vmm_switch_page_directory(new_task->vmm_dir->physical_addr);
            current_directory = new_task->vmm_dir;
            char* dst = (char*)(0xB0000000 - 512);
            for (int i = 0; i <= n; i++) dst[i] = kbuf[i];
            // Restaurer
            vmm_switch_page_directory(old_dir->physical_addr);
            current_directory = old_dir;
            // Placer le pointeur dans EBX
            new_task->cpu_state.ebx = (uint32_t)(0xB0000000 - 512);
        }
    }
    new_task->parent_pid = current_task ? current_task->id : -1;
    return new_task->id;
}


// Implémentation de SYS_GETS - Lire une ligne complète depuis le clavier
static void sys_gets_cooperative(char* buffer, uint32_t size, cpu_state_t* cpu) {
    volatile uint32_t i = 0;
    (void)cpu;
    if (!buffer || size == 0) return;

    print_string_serial("SYS_GETS: Debut de la lecture (version corrigee)...\n");
    asm volatile("sti");

    while (i < size - 1) {
        char c = 0;
        if (!keyboard_poll_char(&c)) {
            task_t* self = current_task;
            /* Save the kernel continuation. schedule(cpu) would store the
             * user int 0x80 frame and re-enter SYS_GETS from scratch. */
            if (self && self->syscall_frame && !self->kctx_valid) {
                self->kctx_valid = 1U;
                if (kctx_save(self->kctx) == 0) {
                    schedule(self->syscall_frame);
                }
                self->kctx_valid = 0U;
            }
            continue;
        }
        if (c == '\r' || c == '\n') {
            print_char('\n', -1, -1, 0x0F);
            buffer[i] = '\0';
            print_string_serial("SYS_GETS: ligne lue: ");
            print_string_serial(buffer);
            print_string_serial("\n");
            return;
        }
        if (c == '\b' && i > 0) {
            i--;
            print_char('\b', -1, -1, 0x0F);
            print_char(' ', -1, -1, 0x0F);
            print_char('\b', -1, -1, 0x0F);
        } else if ((unsigned char)c >= 32 && (unsigned char)c <= 126) {
            buffer[i++] = c;
            print_char(c, -1, -1, 0x0F);
            print_string_serial("SYS_GETS: caractère ajouté: '");
            write_serial(c);
            print_string_serial("'\n");
        }
    }

    buffer[i] = '\0';
    print_string_serial("SYS_GETS: buffer plein, ligne lue: ");
    print_string_serial(buffer);
    print_string_serial("\n");
}

void sys_gets(char* buffer, uint32_t size) {
    if (!buffer || size == 0) return;
    
    print_string_serial("SYS_GETS: Debut de la lecture (version corrigee)...\n");
    
    // Réactiver les interruptions
    asm volatile("sti");
    
    uint32_t i = 0;
    
    while (i < size - 1) {
        char c = keyboard_getc(); // Utilise directement keyboard_getc qui est plus robuste
        
        if (c == '\r' || c == '\n') {
            // Fin de ligne - afficher aussi sur écran
            print_char('\n', -1, -1, 0x0F);
            buffer[i] = '\0';
            print_string_serial("SYS_GETS: ligne lue: ");
            print_string_serial(buffer);
            print_string_serial("\n");
            return;
        }
        
        if (c == '\b' && i > 0) {
            // Backspace - effacer sur l'écran aussi
            i--;
            print_char('\b', -1, -1, 0x0F);  // Backspace
            print_char(' ', -1, -1, 0x0F);   // Espace
            print_char('\b', -1, -1, 0x0F);  // Backspace
        } else if (c >= 32 && c <= 126) {
            // Caractère imprimable - l'afficher sur l'écran
            buffer[i++] = c;
            print_char(c, -1, -1, 0x0F);
            print_string_serial("SYS_GETS: caractère ajouté: '");
            write_serial(c);
            print_string_serial("'\n");
        }
    }
    
    buffer[i] = '\0';
    print_string_serial("SYS_GETS: buffer plein, ligne lue: ");
    print_string_serial(buffer);
    print_string_serial("\n");
}

int sys_listdir(const char* path, os_dirent_t* out, int max_n) {
    int n;
    if (!path || !out || max_n <= 0) return -1;
    if (!ovs_is_dir(path) && !initrd_is_dir(path)) return -1;
    n = initrd_listdir(path, out, max_n);
    if (n < 0) n = 0;
    return ovs_listdir(path, out, n, max_n);
}

int sys_readfile(const char* path, char* buf, uint32_t max) {
    int n;
    if (!path || !buf || max == 0) return -1;
    n = ovs_read(path, buf, max);
    if (n >= 0) return n;
    if (n == OV_ERR_ISDIR) return n;
    return initrd_read_into(path, buf, max);
}

/* AOS-2177: historical ABI entry points. sys_readfile / sys_writefile stay
 * ungated helpers because granted backend paths (SYS_VFS_BACKEND_WRITE and
 * friends) already passed their own AOS-2175/2176 worker gate. */
int sys_readfile_historical(const char* path, char* buf, uint32_t max) {
    os_dirent_t probe;
    int overlay_hit;
    int32_t pid = current_task ? (int32_t)current_task->id : 0;
    if (!path || !buf || max == 0) return -1;
    overlay_hit = ovs_stat(path, &probe) == OV_OK;
    switch (service_registry_historical_read_decision(pid, overlay_hit)) {
    case SERVICE_HIST_READ_FULL:
        return sys_readfile(path, buf, max);
    case SERVICE_HIST_READ_INITRD_ONLY:
        return initrd_read_into(path, buf, max);
    case SERVICE_HIST_READ_WORKER_REQUIRED:
        return OS_VFS_BACKEND_WORKER_REQUIRED;
    default:
        return -1;
    }
}

/* AOS-2178: historical SYS_LISTDIR. Initrd (RAM) listing stays open for
 * non-worker callers while vfs-virtual is live; overlay entries are hidden
 * and an overlay-only directory needs the worker. */
int sys_listdir_historical(const char* path, os_dirent_t* out, int max_n) {
    int overlay_hit;
    int n;
    int32_t pid = current_task ? (int32_t)current_task->id : 0;
    if (!path || !out || max_n <= 0) return -1;
    overlay_hit = !initrd_is_dir(path) && ovs_is_dir(path);
    switch (service_registry_historical_read_decision(pid, overlay_hit)) {
    case SERVICE_HIST_READ_FULL:
        return sys_listdir(path, out, max_n);
    case SERVICE_HIST_READ_INITRD_ONLY:
        if (!initrd_is_dir(path)) return -1;
        n = initrd_listdir(path, out, max_n);
        return n < 0 ? 0 : n;
    case SERVICE_HIST_READ_WORKER_REQUIRED:
        return OS_VFS_BACKEND_WORKER_REQUIRED;
    default:
        return -1;
    }
}

/* AOS-2178: historical SYS_STAT, same split as SYS_READFILE. */
int sys_stat_historical(const char* path, os_dirent_t* out) {
    os_dirent_t probe;
    int overlay_hit;
    int32_t pid = current_task ? (int32_t)current_task->id : 0;
    if (!path || !out) return -1;
    overlay_hit = ovs_stat(path, &probe) == OV_OK;
    switch (service_registry_historical_read_decision(pid, overlay_hit)) {
    case SERVICE_HIST_READ_FULL:
        return sys_stat(path, out);
    case SERVICE_HIST_READ_INITRD_ONLY:
        return initrd_stat(path, out);
    case SERVICE_HIST_READ_WORKER_REQUIRED:
        return OS_VFS_BACKEND_WORKER_REQUIRED;
    default:
        return -1;
    }
}

int sys_writefile_historical(const char* path, const char* buf, uint32_t n) {
    if (!current_task || !service_registry_ata_overlay_io_via_worker(current_task->id))
        return OS_VFS_BACKEND_WORKER_REQUIRED;
    return sys_writefile(path, buf, n);
}

int sys_mkdir(const char* path) {
    if (!path) return -1;
    return ovs_mkdir(path);
}

int sys_unlink(const char* path) {
    if (!path) return -1;
    return ovs_unlink(path);
}

int sys_writefile(const char* path, const char* buf, uint32_t n) {
    if (!path || (n > 0 && !buf)) return -1;
    return ovs_write(path, buf, n);
}

int sys_stat(const char* path, os_dirent_t* out) {
    if (!path || !out) return -1;
    if (ovs_stat(path, out) == OV_OK) return 0;
    return initrd_stat(path, out);
}

int sys_rename(const char* oldpath, const char* newpath) {
    if (!oldpath || !newpath) return -1;
    return ovs_rename(oldpath, newpath);
}

int sys_copy(const char* src, const char* dst) {
    if (!src || !dst) return -1;
    return ovs_copy(src, dst);
}

int sys_append(const char* path, const char* buf, uint32_t n) {
    if (!path || (n > 0 && !buf)) return -1;
    return ovs_append(path, buf, n);
}

int sys_getpid(void) {
    if (!current_task) return -1;
    return current_task->id;
}

int sys_ps(os_proc_t* out, int max_n) {
    if (!out || max_n <= 0) return -1;
    return task_fill_ps(out, max_n);
}

int sys_kill(int pid) {
    int rc;
    if (!current_task) return OS_TASK_CONTROL_DENIED;
    service_notify_purge_pid(pid);
    service_registry_backend_remove_pid(pid);
    (void)service_registry_remove_watcher_pid(pid);
    rc = task_kill(current_task->id, pid);
    return rc;
}

uint32_t sys_ticks(void) {
    return timer_get_ticks();
}

int sys_meminfo(os_meminfo_t* info) {
    if (!info) return -1;
    info->total_pages = pmm_get_total_pages();
    info->used_pages = pmm_get_used_pages();
    info->free_pages = pmm_get_free_pages();
    return 0;
}

int sys_task_metrics(int pid, os_task_metrics_t* out) {
    if (!out || pid < 0) return OS_TASK_NOT_FOUND;
    return task_fill_metrics(pid, out);
}

int sys_task_set_priority(int pid, uint32_t priority) {
    if (!current_task || pid < 0) return OS_TASK_NOT_FOUND;
    return task_set_priority(current_task->id, pid, priority);
}

int sys_task_wait(int pid) {
    if (!current_task || pid < 0) return OS_TASK_NOT_FOUND;
    return task_wait_for_child(current_task->id, pid);
}

int sys_task_set_name(int pid, const char* name) {
    if (!current_task || pid < 0) return OS_TASK_NOT_FOUND;
    return task_set_name(current_task->id, pid, name);
}

int sys_task_capacity(os_task_capacity_t* out) {
    return task_fill_capacity(out);
}

int sys_task_child_result(int pid, os_task_exit_result_t* out) {
    if (!current_task || pid < 0) return OS_TASK_NOT_FOUND;
    return task_get_child_result(current_task->id, pid, out);
}

int sys_task_child_result_list(os_task_exit_history_t* out) {
    if (!current_task) return OS_TASK_NOT_FOUND;
    return task_fill_child_result_history(current_task->id, out);
}

int sys_task_child_result_ack(void) {
    if (!current_task) return OS_TASK_NOT_FOUND;
    return task_ack_child_result_history(current_task->id);
}

int sys_task_child_result_observe(uint32_t expected_generation,
                                  os_task_exit_history_observation_t* out) {
    if (!current_task) return OS_TASK_NOT_FOUND;
    return task_observe_child_result_history(current_task->id, expected_generation, out);
}

int sys_task_child_result_find(int pid, os_task_exit_result_t* out) {
    if (!current_task || pid < 0) return OS_TASK_NOT_FOUND;
    return task_find_child_result_history(current_task->id, pid, out);
}

int sys_task_child_result_forget(int pid) {
    if (!current_task || pid < 0) return OS_TASK_NOT_FOUND;
    return task_forget_child_result_history(current_task->id, pid);
}

int sys_task_suspend(int pid) {
    if (!current_task || pid < 0) return OS_TASK_NOT_FOUND;
    return task_suspend_child(current_task->id, pid);
}

int sys_task_resume(int pid) {
    if (!current_task || pid < 0) return OS_TASK_NOT_FOUND;
    return task_resume_child(current_task->id, pid);
}

int sys_task_kill_children(void) {
    if (!current_task) return OS_TASK_NOT_FOUND;
    return task_kill_direct_children(current_task->id);
}

int sys_task_children(os_task_children_t* out) {
    if (!current_task || !out) return OS_TASK_NOT_FOUND;
    return task_fill_direct_children(current_task->id, out);
}

int sys_task_wait_any(void) {
    if (!current_task) return OS_TASK_NOT_FOUND;
    return task_wait_for_any_child(current_task->id);
}

int sys_task_child_exit_count(os_task_child_exit_count_t* out) {
    if (!current_task || !out) return OS_TASK_NOT_FOUND;
    return task_get_child_exit_count(current_task->id, &out->count);
}

int sys_task_delegate_child(int child_pid, int supervisor_pid) {
    if (!current_task) return OS_TASK_NOT_FOUND;
    return task_delegate_child(current_task->id, child_pid, supervisor_pid);
}

int sys_task_supervision_events(os_task_supervision_events_t* out) {
    if (!current_task || !out) return OS_TASK_NOT_FOUND;
    return task_fill_supervision_events(current_task->id, out);
}

int sys_task_supervision_events_ack(void) {
    if (!current_task) return OS_TASK_NOT_FOUND;
    return task_ack_supervision_events(current_task->id);
}

int sys_task_supervision_events_observe(uint32_t expected_generation,
                                        os_task_supervision_events_observation_t* out) {
    if (!current_task || !out) return OS_TASK_NOT_FOUND;
    return task_observe_supervision_events(current_task->id, expected_generation, out);
}

int sys_task_supervision_event_find(uint32_t sequence, os_task_supervision_event_t* out) {
    if (!current_task || !out) return OS_TASK_NOT_FOUND;
    return task_find_supervision_event(current_task->id, sequence, out);
}

int sys_task_supervision_event_forget(uint32_t sequence) {
    if (!current_task) return OS_TASK_NOT_FOUND;
    return task_forget_supervision_event(current_task->id, sequence);
}

int sys_task_supervision_summary(os_task_supervision_summary_t* out) {
    if (!current_task || !out) return OS_TASK_NOT_FOUND;
    return task_fill_supervision_summary(current_task->id, out);
}
