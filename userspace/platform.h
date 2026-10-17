/* userspace/platform.h - Phase 6 multi-platform groundwork (US-076..US-090).
 * Pure C, no allocation, host-testable. The guest shell front end lives in
 * userspace/shell_platform.c. Only the i386 port exists: the HAL describes
 * it and refuses to pretend another architecture is built. */
#ifndef MOHHDY_PLATFORM_H
#define MOHHDY_PLATFORM_H
#include <stdint.h>

/* ---- HAL (US-076 groundwork) ---- */
typedef struct {
    const char* arch;        /* "i386" */
    uint32_t word_bits;
    uint32_t page_size;
    int little_endian;
    char cpu_vendor[13];
    uint32_t cpu_family, cpu_model;
    uint32_t features_edx, features_ecx;
    uint32_t mem_total_kib, mem_free_kib;
} plat_hal_t;
int plat_hal_report(const plat_hal_t* h, char* out, int cap);
/* Ports known to the build: 1 for i386, 0 (with a reason) otherwise. */
int plat_port_supported(const char* arch, const char** reason);

/* ---- screen adaptation (US-080) ---- */
enum { PLAT_FORM_PHONE = 1, PLAT_FORM_TABLET, PLAT_FORM_DESKTOP };
typedef struct {
    int form;
    int cols, rows;          /* text grid after adaptation */
    int font_px;             /* cell height */
    int panels;              /* side-by-side OS-UI panels */
    int compact;             /* short labels, no side bar */
    int portrait;
} plat_layout_t;
int plat_layout(int width_px, int height_px, plat_layout_t* out);
int plat_layout_resize(plat_layout_t* layout, int new_w, int new_h);
const char* plat_form_name(int form);

/* ---- touch gestures from pointer samples (US-079, single pointer & multi-touch) ---- */
typedef struct { int x, y; uint32_t t_ms; int down; } plat_point_t;
enum { PLAT_G_NONE = 0, PLAT_G_TAP, PLAT_G_DOUBLE_TAP, PLAT_G_LONG_PRESS,
       PLAT_G_SWIPE_LEFT, PLAT_G_SWIPE_RIGHT, PLAT_G_SWIPE_UP, PLAT_G_SWIPE_DOWN, PLAT_G_DRAG,
       PLAT_G_PINCH_IN = 10, PLAT_G_PINCH_OUT = 11, PLAT_G_ROTATE = 12, PLAT_G_SWIPE_2FINGER = 13 };
int plat_gesture(const plat_point_t* p, int n);
int plat_gesture_multi(const plat_point_t* p1, const plat_point_t* p2, int n);
const char* plat_gesture_name(int g);

/* ---- mobile sensors (US-078) ---- */
typedef struct {
    float accel_x, accel_y, accel_z;
    float gyro_x, gyro_y, gyro_z;
    int light_lux;
    int proximity_cm;
    int battery_pct;
    int charging;
} plat_sensors_t;
int plat_sensor_read(plat_sensors_t* s);
int plat_sensor_report(const plat_sensors_t* s, char* out, int cap);

/* ---- energy (US-081) ---- */
enum { PLAT_PWR_PERFORMANCE = 1, PLAT_PWR_BALANCED, PLAT_PWR_SAVER };
typedef struct {
    int profile;
    uint32_t idle_yields;     /* yields between console polls */
    uint32_t poll_budget;     /* background datagrams per pump */
    uint32_t screen_dim_s;    /* seconds before dimming the screen */
    uint32_t background_ms;   /* min period of background services */
    uint32_t refresh_hz;      /* dynamic display refresh rate in Hz */
} plat_power_policy_t;
int plat_power_policy(int profile, plat_power_policy_t* out);
int plat_power_parse(const char* name);
const char* plat_power_name(int profile);
/* auto: pick a profile from the busy percentage of the last sample. */
int plat_power_auto(uint32_t busy_percent, int on_battery);
int plat_power_status_report(uint32_t busy_percent, int battery_pct, char* out, int cap);

/* ---- device registry (US-085) ---- */
#define PLAT_DEVICES 12
#define PLAT_DEV_RULES 8
typedef struct { char name[16]; char cls[12]; int present; char detail[48]; } plat_device_t;
typedef struct { plat_device_t d[PLAT_DEVICES]; int n; } plat_devices_t;
typedef struct {
    char name_pattern[16];
    char cls_pattern[12];
    char action_cmd[32];
    int enabled;
} plat_dev_rule_t;
typedef struct {
    plat_dev_rule_t rules[PLAT_DEV_RULES];
    int n_rules;
    uint32_t hotplug_events;
} plat_dev_rules_t;

int plat_dev_add(plat_devices_t* r, const char* name, const char* cls, int present, const char* detail);
int plat_dev_report(const plat_devices_t* r, const char* name, char* out, int cap);
int plat_dev_rule_add(plat_dev_rules_t* r, const char* name_pat, const char* cls_pat, const char* action);
int plat_dev_hotplug_event(plat_devices_t* dev_reg, plat_dev_rules_t* rule_reg, const char* dev_name, const char* cls, int present, const char* detail, char* out_action, int cap);

/* ---- multi-node administration view (US-089) ---- */
#define PLAT_ADMIN_MAX_NODES 8
typedef struct {
    uint32_t node_id;
    char name[16];
    char arch[16];
    uint32_t mem_free_kib;
    uint32_t busy_percent;
    int active;
} plat_admin_node_t;
typedef struct {
    plat_admin_node_t nodes[PLAT_ADMIN_MAX_NODES];
    int count;
} plat_admin_nodes_t;

int plat_admin_node_add(plat_admin_nodes_t* an, uint32_t id, const char* name, const char* arch, uint32_t mem_free, uint32_t busy);
int plat_admin_nodes_report(const plat_admin_nodes_t* an, char* out, int cap);

/* ---- application compatibility (US-087) ---- */
typedef struct { int ok; const char* reason; uint32_t entry; uint32_t phnum; uint32_t machine; } plat_compat_t;
int plat_elf_check(const uint8_t* b, int len, plat_compat_t* out);

/* ---- notifications (US-082, local queue with TTL) ---- */
#define PLAT_NOTES 8
typedef struct { uint32_t id; int prio; int acked; uint32_t count; uint32_t ttl_s; uint32_t created_s; char text[48]; } plat_note_t;
typedef struct { plat_note_t q[PLAT_NOTES]; uint32_t next_id; uint32_t dropped; uint32_t expired; } plat_notes_t;
uint32_t plat_note_push(plat_notes_t* n, int prio, const char* text);
uint32_t plat_note_push_ttl(plat_notes_t* n, int prio, uint32_t ttl_s, uint32_t now_s, const char* text);
int plat_note_tick(plat_notes_t* n, uint32_t now_s);
int plat_note_ack(plat_notes_t* n, uint32_t id);
int plat_note_report(const plat_notes_t* n, char* out, int cap);

/* ---- continuity cross-platform (US-084) ---- */
typedef struct {
    char session_id[16];
    char device_from[16];
    char clipboard[128];
    char active_task[32];
    uint32_t state_seq;
} plat_continuity_t;
int plat_continuity_export(const plat_continuity_t* c, char* out, int cap);
int plat_continuity_import(const char* in, plat_continuity_t* c);

/* ---- legacy emulation (US-086): see dosemu.h (real 8086 .COM interpreter) ---- */

/* ---- migration archive (US-088) and deployment manifest (US-090) ---- */
uint32_t plat_fnv(const uint8_t* b, int len);
#define PLAT_MIG_FILES 16
typedef struct { char name[64]; const char* data; int len; } plat_mig_entry_t;
/* "MMIG1\n" + per file "F <name> <len> <fnv>\n<bytes>\n" + "END <count> <fnv-all>\n" */
int plat_mig_pack(const plat_mig_entry_t* e, int n, char* out, int cap);
/* Verifies and splits an archive; entry data point into `in`. Returns the
 * file count or a negative error (-1 format, -2 checksum, -3 too many). */
int plat_mig_unpack(const char* in, int len, plat_mig_entry_t* e, int max);
/* "DEPLOY1\n" + lines "file <src> <dst> <fnv>". Returns count or -1. */
typedef struct { char src[64]; char dst[64]; uint32_t fnv; } plat_deploy_item_t;
int plat_deploy_parse(const char* in, int len, plat_deploy_item_t* items, int max);
#endif
