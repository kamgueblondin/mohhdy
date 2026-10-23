/* userspace/webrt.h - Lot 5 Phase 3 web runtime building blocks (pure C, no
 * libc, host unit tested). They feed the OS-UI text renderer
 * (osui_runtime.c); none of this is a browser engine: no network fetch of
 * real pages here, no JavaScript, no layout.
 *   US-038 HTTP cache: Cache-Control/ETag freshness, LRU, revalidation.
 *   US-039 web security: CSP parsing and source checks, HTML sanitizer.
 *   US-040 responsive: media query evaluation, @media flattening.
 *   US-041 themes: CSS custom properties (var()) and colour scheme.
 *   US-042 Web Components: <template> custom elements with <slot>.
 *   US-044 accessibility: static audit of an HTML fragment. */
#ifndef MOHHDY_WEBRT_H
#define MOHHDY_WEBRT_H
#include <stdint.h>

/* ---- US-038 cache ---- */
#define WEBRT_CACHE_SLOTS 8
#define WEBRT_CACHE_MISS 0
#define WEBRT_CACHE_FRESH 1
#define WEBRT_CACHE_STALE 2    /* revalidate with If-None-Match: etag */
typedef struct {
    char url[96];
    char etag[32];
    uint32_t stored, max_age, last_use, bytes;
    uint8_t used, no_cache;
} webrt_cache_entry_t;
typedef struct {
    webrt_cache_entry_t e[WEBRT_CACHE_SLOTS];
    uint32_t hits, misses, stale, revalidated, evictions, stores, refused;
    uint32_t clock;   /* LRU use counter */
} webrt_cache_t;
void webrt_cache_init(webrt_cache_t* c);
/* Store a response. Returns the slot, or -1 when Cache-Control forbids it. */
int webrt_cache_store(webrt_cache_t* c, const char* url, const char* cache_control,
                      const char* etag, uint32_t bytes, uint32_t now);
int webrt_cache_lookup(webrt_cache_t* c, const char* url, uint32_t now, const char** etag);
/* Answer to a conditional request: 304 refreshes the entry, 200 replaces it
 * (with the new headers), anything else drops it. Returns 1 if kept. */
int webrt_cache_revalidated(webrt_cache_t* c, const char* url, int status, const char* cache_control,
                            const char* etag, uint32_t bytes, uint32_t now);

/* ---- US-039 security ---- */
#define WEBRT_CSP_MAX 8
typedef struct {
    char name[16];
    char sources[112];
} webrt_csp_directive_t;
typedef struct {
    webrt_csp_directive_t d[WEBRT_CSP_MAX];
    int count;
    int errors;
} webrt_csp_t;
typedef void (*webrt_audit_fn)(const char* action, const char* detail);
void webrt_set_audit_callback(webrt_audit_fn fn);
int webrt_csp_parse(webrt_csp_t* p, const char* header);
/* url "inline" asks about an inline script/style ('unsafe-inline'). */
int webrt_csp_allows(const webrt_csp_t* p, const char* directive, const char* page_origin, const char* url);
/* Lot 23 (US-039 / US-041): Dynamic CSP enforcement and resource display list validation */
int webrt_csp_enforce(const webrt_csp_t* p, const char* page_origin, const char* resource_type, const char* resource_url);
int webrt_csp_validate_display_list_resource(const webrt_csp_t* p, const char* page_origin, const char* resource_type, const char* resource_url);
/* Origin "scheme://host[:port]" of an absolute URL into out (0 if relative). */
int webrt_url_origin(const char* url, char* out, int max);
/* Drops <script>, <iframe>, <object>, <embed>, <style> elements, on*
 * attributes and javascript: URLs. Returns the output length, *removed the
 * number of things removed. */
int webrt_sanitize_html(const char* in, char* out, int max, int* removed);
/* Lot 7 (US-039): same filter but <style> is kept, its CSS filtered
 * (@import, url(), javascript:, expression(), behavior dropped), and unsafe
 * inline style attributes removed. Used by the guest web engine pipeline. */
int webrt_sanitize_page(const char* in, char* out, int max, int* removed);

/* ---- US-040 responsive / US-041 themes ---- */
typedef struct {
    int width, height;
    int dark;      /* prefers-color-scheme: dark */
} webrt_viewport_t;
int webrt_mq_match(const char* query, const webrt_viewport_t* v);
/* Keeps the rules of matching @media blocks, drops the others. Returns the
 * output length; *kept / *dropped count the blocks. */
int webrt_css_flatten_media(const char* css, const webrt_viewport_t* v, char* out, int max,
                            int* kept, int* dropped);
/* Substitutes var(--name[, fallback]) with the custom property values
 * (declared anywhere, last wins) and drops the --name declarations. */
int webrt_css_resolve_vars(const char* css, char* out, int max, int* defined, int* unresolved);

/* ---- US-042 Web Components ---- */
int webrt_components_expand(const char* html, char* out, int max, int* defined, int* expanded);

/* ---- US-044 accessibility ---- */
typedef struct {
    int images, img_no_alt;
    int inputs, input_no_label;
    int buttons, button_no_text;
    int links, link_no_text;
    int headings, heading_skips;
    int lang_missing;
    int issues;
} webrt_a11y_t;
int webrt_a11y_audit(const char* html, webrt_a11y_t* r);

/* ---- US-038 / US-045 Local Storage Isolation & Audit ---- */
int webrt_site_storage_validate_origin(const char* page_origin, const char* target_origin);
int webrt_enc_v1_audit_log(const char* action, const char* path, int len);

/* ---- US-045 Ring 3 TLS / X.509 Engine ---- */
#define WEBRT_TLS_STATE_IDLE          0
#define WEBRT_TLS_STATE_CLIENT_HELLO   1
#define WEBRT_TLS_STATE_SERVER_HELLO   2
#define WEBRT_TLS_STATE_CERT_VERIFY    3
#define WEBRT_TLS_STATE_KEY_EXCHANGE   4
#define WEBRT_TLS_STATE_FINISHED       5
#define WEBRT_TLS_STATE_ESTABLISHED    6

typedef struct {
    uint8_t state;
    uint8_t cert_chain_depth;
    uint8_t cert_valid;
    char hostname[64];
    uint8_t client_random[32];
    uint8_t server_random[32];
    uint8_t master_secret[48];
} webrt_tls_session_t;

void webrt_tls_init(webrt_tls_session_t* session, const char* hostname);
int webrt_tls_verify_cert_chain(webrt_tls_session_t* session, const uint8_t* cert_data, int cert_len, int depth);
int webrt_tls_handshake_step(webrt_tls_session_t* session, const uint8_t* in, int in_len, uint8_t* out, int max_out);

/* ---- US-041 DOM Event Manager & Callbacks ---- */
#define WEBRT_EVENT_MAX 16
typedef struct {
    char element_id[32];
    char event_type[20]; /* e.g. "click", "submit", "change" */
    char callback_script[128];
    uint8_t active;
} webrt_dom_event_t;

typedef struct {
    webrt_dom_event_t listeners[WEBRT_EVENT_MAX];
    int count;
} webrt_dom_event_mgr_t;

void webrt_dom_event_init(webrt_dom_event_mgr_t* mgr);
int webrt_dom_add_event_listener(webrt_dom_event_mgr_t* mgr, const char* element_id, const char* event_type, const char* callback_script);
int webrt_dom_dispatch_event(webrt_dom_event_mgr_t* mgr, const char* element_id, const char* event_type, char* out_eval, int max_eval);

/* ---- US-043 Async Event Loop & DOM Timers ---- */
#define WEBRT_TIMER_MAX 16

typedef struct {
    uint32_t id;
    uint32_t delay_ms;
    uint32_t next_fire_ms;
    uint8_t is_interval;
    uint8_t active;
    char callback_script[128];
} webrt_timer_t;

int webrt_timer_schedule(uint32_t delay_ms, int is_interval, const char *callback_script);
int webrt_event_loop_tick(uint32_t current_time_ms, char *out_eval, int max_eval);

/* ---- US-043 X.509 KeyUsage/EKU & OCSP ---- */
#define WEBRT_KU_DIGITAL_SIGNATURE (1 << 0)
#define WEBRT_KU_NON_REPUDIATION   (1 << 1)
#define WEBRT_KU_KEY_ENCIPHERMENT  (1 << 2)
#define WEBRT_KU_DATA_ENCIPHERMENT (1 << 3)

#define WEBRT_EKU_SERVER_AUTH      (1 << 0)
#define WEBRT_EKU_CLIENT_AUTH      (1 << 1)
#define WEBRT_EKU_CODE_SIGNING     (1 << 2)

int webrt_x509_verify_key_usage(const uint8_t *cert_data, int cert_len, uint16_t required_usage, uint16_t required_eku);
int webrt_ocsp_check_status(const char *cert_serial, char *out_status, int cap);

/* ---- US-047 DOM MutationObserver & CSS Flexbox Light ---- */
#define WEBRT_OBSERVER_MAX 8
typedef struct {
    char target_id[32];
    char callback_script[128];
    uint8_t observe_child_list;
    uint8_t observe_attributes;
    uint8_t active;
} webrt_dom_observer_t;

typedef struct {
    webrt_dom_observer_t observers[WEBRT_OBSERVER_MAX];
    int count;
} webrt_dom_observer_mgr_t;

void webrt_dom_observer_init(webrt_dom_observer_mgr_t *mgr);
int webrt_dom_observe_mutation(webrt_dom_observer_mgr_t *mgr, const char *target_id, const char *callback_script, uint8_t child_list, uint8_t attributes);
int webrt_dom_notify_mutation(webrt_dom_observer_mgr_t *mgr, const char *target_id, const char *mutation_type, char *out_eval, int max_eval);

typedef struct {
    int x, y, width, height;
} webrt_rect_t;

#define WEBRT_FLEX_ITEM_MAX 8
typedef struct {
    int min_width, min_height;
    int computed_x, computed_y;
    int computed_w, computed_h;
} webrt_flex_item_t;

typedef struct {
    int direction;      /* 0: row, 1: column */
    int justify_content;/* 0: flex-start, 1: center, 2: flex-end, 3: space-between */
    int align_items;    /* 0: flex-start, 1: center, 2: flex-end */
    int width, height;
    webrt_flex_item_t items[WEBRT_FLEX_ITEM_MAX];
    int item_count;
} webrt_flex_container_t;

int webrt_flex_compute_layout(webrt_flex_container_t *container);

/* ---- Lot 29: Shadow DOM, Event Propagation Path & Microtasks/Macrotasks (US-032 / US-043) ---- */
#define WEBRT_SHADOW_MODE_OPEN   0
#define WEBRT_SHADOW_MODE_CLOSED 1
#define WEBRT_SHADOW_MAX_ROOTS   16
#define WEBRT_SHADOW_MAX_NODES   16

typedef struct {
    char id[32];
    char tag[32];
    char inner_html[128];
} webrt_shadow_node_t;

typedef struct {
    char host_id[32];
    uint8_t mode; /* 0: OPEN, 1: CLOSED */
    uint8_t active;
    webrt_shadow_node_t nodes[WEBRT_SHADOW_MAX_NODES];
    int node_count;
} webrt_shadow_root_t;

#define WEBRT_EVENT_PHASE_NONE      0
#define WEBRT_EVENT_PHASE_CAPTURING 1
#define WEBRT_EVENT_PHASE_AT_TARGET  2
#define WEBRT_EVENT_PHASE_BUBBLING   3

#define WEBRT_TASKS_MAX 16

typedef struct {
    void (*fn)(void *arg);
    void *arg;
    char desc[64];
    uint8_t active;
} webrt_task_t;

void webrt_shadow_init(void);
int webrt_shadow_root_attach(const char *host_element_id, uint8_t mode, webrt_shadow_root_t **out_root);
int webrt_shadow_root_add_node(const char *host_element_id, const char *node_id, const char *tag, const char *inner_html);
int webrt_shadow_root_get(const char *host_element_id, uint8_t requester_mode, webrt_shadow_root_t **out_root);

int webrt_dom_dispatch_event_path(webrt_dom_event_mgr_t *mgr, const char *target_id, const char *event_type, uint8_t bubbles, char *event_path_out, int path_cap);

int webrt_queue_microtask(void (*fn)(void *), void *arg, const char *desc);
int webrt_queue_macrotask(void (*fn)(void *), void *arg, const char *desc);
/* ---- Lot 28: Web Runtime Vector Engine & CSS Box Model / 2D Rasterizer (US-031 / US-040 / US-042) ---- */
typedef struct {
    int margin_top, margin_right, margin_bottom, margin_left;
    int border_top, border_right, border_bottom, border_left;
    int padding_top, padding_right, padding_bottom, padding_left;
    int content_width, content_height;
    int total_width, total_height;
} webrt_box_model_t;

int webrt_compute_box_model(int width, int height,
                            int m_top, int m_right, int m_bottom, int m_left,
                            int b_top, int b_right, int b_bottom, int b_left,
                            int p_top, int p_right, int p_bottom, int p_left,
                            webrt_box_model_t *out);

int webrt_text_wrap(const char *in_text, int max_width_px, int char_width_px,
                    char *out_lines, int max_lines, int line_cap);

#define WEBRT_TEXT_ALIGN_LEFT    0
#define WEBRT_TEXT_ALIGN_CENTER  1
#define WEBRT_TEXT_ALIGN_RIGHT   2
#define WEBRT_TEXT_ALIGN_JUSTIFY 3

int webrt_text_align_offset(int align_mode, int content_width_px, int text_width_px);

#define WEBRT_FORM_INPUT    1
#define WEBRT_FORM_BUTTON   2
#define WEBRT_FORM_TEXTAREA 3

typedef struct {
    int id;
    int type;
    int focused;
    char name[32];
    char value[128];
    char onchange_callback[64];
    char onclick_callback[64];
} webrt_form_component_t;

void webrt_form_component_init(webrt_form_component_t *comp, int id, int type, const char *name);
int webrt_form_component_set_focus(webrt_form_component_t *comp, int focused);
int webrt_form_component_input_text(webrt_form_component_t *comp, const char *text, char *out_eval, int max_eval);
int webrt_form_component_click(webrt_form_component_t *comp, char *out_eval, int max_eval);

#define WEBRT_VEC_LINE       1
#define WEBRT_VEC_ROUND_RECT 2
#define WEBRT_VEC_GRADIENT   3

typedef struct {
    int type;
    int x1, y1, x2, y2;
    int radius;
    uint32_t color_start;
    uint32_t color_end;
} webrt_vec_cmd_t;

#define WEBRT_VEC_CMD_MAX 16
typedef struct {
    webrt_vec_cmd_t cmds[WEBRT_VEC_CMD_MAX];
    int count;
} webrt_vec_display_list_t;

void webrt_vec_init(webrt_vec_display_list_t *dl);
int webrt_vec_add_line(webrt_vec_display_list_t *dl, int x1, int y1, int x2, int y2, uint32_t color);
int webrt_vec_add_round_rect(webrt_vec_display_list_t *dl, int x, int y, int w, int h, int radius, uint32_t color);
int webrt_vec_add_gradient(webrt_vec_display_list_t *dl, int x, int y, int w, int h, uint32_t color_start, uint32_t color_end);
int webrt_render_vector_dom(const webrt_box_model_t *box, uint32_t bg_color, uint32_t border_color, webrt_vec_display_list_t *out_dl);

/* Lot 30: Web Runtime Vector Canvas 2D & WebAssembly VM (US-048 / US-050) */
#define WEBRT_CANVAS2D_MAX_COMMANDS 64
#define WEBRT_WASM_STACK_MAX 128
#define WEBRT_WASM_GLOBALS_MAX 16

#define WEBRT_CANVAS_PATH_LINE    1
#define WEBRT_CANVAS_PATH_BEZIER  2
#define WEBRT_CANVAS_PATH_RECT    3
#define WEBRT_CANVAS_PATH_POLY    4

typedef struct {
    int32_t a, b, c, d, e, f; /* 16.16 fixed-point matrix: [a c e; b d f] */
} webrt_matrix2d_t;

typedef struct {
    int type;
    int32_t x1, y1, x2, y2, x3, y3, x4, y4;
    uint32_t fill_color;
    uint32_t stroke_color;
    int point_count;
} webrt_canvas_cmd_t;

typedef struct {
    int width;
    int height;
    webrt_matrix2d_t matrix;
    webrt_canvas_cmd_t commands[WEBRT_CANVAS2D_MAX_COMMANDS];
    int command_count;
} webrt_canvas2d_t;

typedef struct {
    int32_t stack[WEBRT_WASM_STACK_MAX];
    int sp;
    int32_t globals[WEBRT_WASM_GLOBALS_MAX];
    int global_count;
    uint32_t pc;
    uint8_t halted;
} webrt_wasm_vm_t;

typedef struct {
    uint32_t frame_id;
    uint32_t callback_id;
    uint8_t pending;
} webrt_anim_frame_t;

void webrt_canvas2d_init(webrt_canvas2d_t *canvas, int width, int height);
void webrt_canvas2d_transform(webrt_canvas2d_t *canvas, int32_t a, int32_t b, int32_t c, int32_t d, int32_t e, int32_t f);
int webrt_canvas2d_draw_path(webrt_canvas2d_t *canvas, int type, int32_t x1, int32_t y1, int32_t x2, int32_t y2, uint32_t fill_color);

void webrt_wasm_vm_init(webrt_wasm_vm_t *vm);
int webrt_wasm_stack_push(webrt_wasm_vm_t *vm, int32_t val);
int webrt_wasm_stack_pop(webrt_wasm_vm_t *vm, int32_t *out_val);
int webrt_wasm_exec_module(webrt_wasm_vm_t *vm, const uint8_t *bytecode, uint32_t code_len);

int webrt_request_animation_frame(uint32_t callback_id);
int webrt_render_frame_tick(uint32_t current_tick, uint32_t *out_executed_callback);

/* Lot 31: Web Runtime WebGL 3D Vector Pipeline & AssemblyScript JIT VM (US-049 / US-051) */
#define WEBRT_WEBGL3D_MAX_VERTICES 128
#define WEBRT_WEBGL3D_MAX_FACES    64
#define WEBRT_AS_HOST_FN_MAX       16
#define WEBRT_ARRAYBUFFER_MAX      1024

typedef struct {
    int32_t x, y, z; /* 16.16 fixed point */
} webrt_vec3_t;

typedef struct {
    int32_t m[4][4]; /* 16.16 fixed point matrix */
} webrt_mat4_t;

typedef struct {
    uint16_t v1, v2, v3;
    uint32_t color;
} webrt_face_t;

typedef struct {
    webrt_vec3_t vertices[WEBRT_WEBGL3D_MAX_VERTICES];
    int vertex_count;
    webrt_face_t faces[WEBRT_WEBGL3D_MAX_FACES];
    int face_count;
    webrt_mat4_t modelview;
    webrt_mat4_t projection;
    webrt_vec3_t light_dir;
} webrt_webgl3d_ctx_t;

typedef int32_t (*webrt_as_host_fn_t)(int32_t arg1, int32_t arg2);

typedef struct {
    uint8_t id;
    webrt_as_host_fn_t fn;
} webrt_as_host_reg_t;

typedef struct {
    uint8_t data[WEBRT_ARRAYBUFFER_MAX];
    uint32_t byte_length;
    uint32_t shared_ref_count;
    int32_t lock_state;
} webrt_arraybuffer_t;

void webrt_webgl3d_init(webrt_webgl3d_ctx_t *ctx);
void webrt_webgl3d_set_matrix(webrt_webgl3d_ctx_t *ctx, int is_projection, const webrt_mat4_t *mat);
int webrt_webgl3d_render_mesh(webrt_webgl3d_ctx_t *ctx, uint32_t *out_framebuffer, int width, int height);

int webrt_assemblyscript_register_host_fn(uint8_t fn_id, webrt_as_host_fn_t fn);
int webrt_assemblyscript_exec(const uint8_t *bytecode, uint32_t code_len, int32_t *out_result);

int webrt_arraybuffer_share(webrt_arraybuffer_t *ab, uint32_t length);
int webrt_atomics_wait_notify(webrt_arraybuffer_t *ab, uint32_t byte_offset, int32_t expected, int32_t new_val);

/* Lot 32: Web Runtime GPU Fixed-Point Rasterizer & WebAssembly SIMD Pipeline (US-052 / US-054) */
#define WEBRT_GPU_MAX_POLY_PTS 16
#define WEBRT_WASM_SIMD_STACK_MAX 16

typedef struct {
    int32_t x; /* 16.16 fixed point */
    int32_t y; /* 16.16 fixed point */
    int32_t z; /* 16.16 fixed point */
} webrt_gpu_point3d_t;

typedef struct {
    webrt_gpu_point3d_t pts[WEBRT_GPU_MAX_POLY_PTS];
    int count;
    uint32_t color;
} webrt_gpu_polygon_t;

typedef union {
    int32_t i32[4];
    uint32_t u32[4];
    uint8_t u8[16];
} webrt_wasm_v128_t;

typedef struct {
    webrt_wasm_v128_t stack[WEBRT_WASM_SIMD_STACK_MAX];
    int top;
} webrt_wasm_simd_ctx_t;

int webrt_gpu_rasterize_polygon(const webrt_gpu_polygon_t *poly, uint32_t *framebuffer, int width, int height);
int webrt_gpu_clip_frustum(const webrt_gpu_polygon_t *in_poly, webrt_gpu_polygon_t *out_poly);

int webrt_wasm_simd_exec(const uint8_t *bytecode, uint32_t code_len, webrt_wasm_v128_t *out_val);

int webrt_shared_memory_lock(webrt_arraybuffer_t *ab, uint32_t timeout_ticks);
int webrt_shared_memory_unlock(webrt_arraybuffer_t *ab);

/* Lot 33: Web Runtime WebGPU Fixed-Point Compute Shader & WebAssembly Threads Atomic Futex Engine (US-053 / US-055) */
#define WEBRT_WEBGPU_WORKGROUP_MAX 64
#define WEBRT_WASM_THREADS_MAX 8

#define WEBRT_WASM_ATOMIC_ADD 1
#define WEBRT_WASM_ATOMIC_SUB 2
#define WEBRT_WASM_ATOMIC_AND 3
#define WEBRT_WASM_ATOMIC_OR  4
#define WEBRT_WASM_ATOMIC_XOR 5
#define WEBRT_WASM_ATOMIC_XCHG 6

typedef struct {
    uint32_t workgroup_size_x;
    uint32_t workgroup_size_y;
    uint32_t dispatch_count;
    uint8_t active;
} webrt_webgpu_pipeline_t;

typedef struct {
    uint32_t num_threads;
    uint32_t active_threads;
    uint8_t shared_memory_enabled;
} webrt_wasm_threads_ctx_t;

void webrt_webgpu_compute_init(webrt_webgpu_pipeline_t *pipeline, uint32_t workgroup_size_x, uint32_t workgroup_size_y);
int webrt_webgpu_dispatch_compute(webrt_webgpu_pipeline_t *pipeline, const int32_t *input_data, int32_t *output_data, uint32_t count);

void webrt_wasm_threads_init(webrt_wasm_threads_ctx_t *ctx, uint32_t num_threads);
int webrt_wasm_atomic_op(webrt_arraybuffer_t *ab, uint32_t byte_offset, uint32_t op_type, int32_t operand, int32_t *out_old);
int webrt_futex_wait(webrt_arraybuffer_t *ab, uint32_t byte_offset, int32_t expected_val, uint32_t timeout_ms);
int webrt_futex_wake(webrt_arraybuffer_t *ab, uint32_t byte_offset, uint32_t wake_count);

/* Lot 35: Web Runtime Service Workers PWA Offscreen Cache Engine & WebAssembly SIMD Vector Acceleration (US-056 / US-058) */
#define WEBRT_SW_MAX_WORKERS       8
#define WEBRT_OFFSCREEN_CACHE_MAX 16

typedef struct {
    char scope[64];
    char script_url[96];
    uint8_t active;
} webrt_service_worker_t;

typedef struct {
    char key[64];
    char response_body[256];
    uint32_t body_len;
    uint8_t active;
} webrt_offscreen_cache_entry_t;

typedef struct {
    webrt_service_worker_t workers[WEBRT_SW_MAX_WORKERS];
    int worker_count;
    webrt_offscreen_cache_entry_t cache_entries[WEBRT_OFFSCREEN_CACHE_MAX];
    int cache_count;
} webrt_sw_cache_mgr_t;

void webrt_sw_cache_mgr_init(webrt_sw_cache_mgr_t *mgr);
int webrt_service_worker_register(webrt_sw_cache_mgr_t *mgr, const char *scope, const char *script_url);
int webrt_offscreen_cache_put(webrt_sw_cache_mgr_t *mgr, const char *key, const char *response_body, uint32_t body_len);
int webrt_offscreen_cache_match(webrt_sw_cache_mgr_t *mgr, const char *key, char *out_body, uint32_t max_len, uint32_t *out_len);

int webrt_wasm_simd_vec_add_f32(const float *a, const float *b, float *out, uint32_t count);
int webrt_wasm_simd_vec_dot_i32(const int32_t *a, const int32_t *b, uint32_t count, int32_t *out_dot);

/* Lot 37: Web Runtime Service Workers Background Sync Engine & WebAssembly SIMD Matrix 2D/3D Tensor Acceleration (US-057 / US-059) */
#define WEBRT_BG_SYNC_QUEUE_MAX 16

typedef struct {
    uint32_t task_id;
    char tag[32];
    char payload[128];
    uint32_t payload_len;
    uint8_t retries;
    uint8_t status; /* 0=pending, 1=synced, 2=failed */
} webrt_bg_sync_task_t;

typedef struct {
    webrt_bg_sync_task_t tasks[WEBRT_BG_SYNC_QUEUE_MAX];
    int count;
    uint32_t synced_count;
    uint32_t failed_count;
} webrt_bg_sync_queue_t;

typedef struct {
    int32_t m[4][4]; /* 16.16 fixed point */
} webrt_mat4x4_t;

typedef struct {
    int32_t data[2][2][2]; /* 16.16 fixed point 2x2x2 tensor */
} webrt_tensor3d_t;

void webrt_bg_sync_init(webrt_bg_sync_queue_t *q);
int webrt_bg_sync_register(webrt_bg_sync_queue_t *q, const char *tag, const char *payload, uint32_t len, uint32_t *out_task_id);
int webrt_bg_sync_process(webrt_bg_sync_queue_t *q, int *out_processed);

int webrt_wasm_simd_mat4_mul(const webrt_mat4x4_t *a, const webrt_mat4x4_t *b, webrt_mat4x4_t *out);
int webrt_wasm_simd_tensor3d_contract(const webrt_tensor3d_t *t1, const webrt_tensor3d_t *t2, int32_t *out_scalar);

/* Lot 38: Web Runtime Web Workers Shared Array Buffer Atomics Synchronization Engine & WebAssembly SIMD 128-bit Tensor Convolutions (US-058 / US-060) */
#define WEBRT_SAB_FUTEX_WAITERS_MAX 8

typedef struct {
    int32_t value; /* atomic counter/state */
    uint32_t lock_val;
    uint32_t waiters[WEBRT_SAB_FUTEX_WAITERS_MAX];
    int waiter_count;
    uint32_t lock_acquisitions;
    uint32_t contention_events;
} webrt_sab_futex_t;

typedef struct {
    int32_t m[8][8]; /* 16.16 fixed point 8x8 matrix */
} webrt_mat8x8_t;

typedef struct {
    int32_t input[16][16];  /* 16x16 input image/tensor */
    int32_t kernel[3][3];   /* 3x3 convolution kernel */
    int32_t output[14][14]; /* 14x14 output tensor */
    uint32_t conv_ops_count;
} webrt_tensor_conv_t;

void webrt_sab_futex_init(webrt_sab_futex_t *futex, int32_t initial_val);
int webrt_sab_futex_wait(webrt_sab_futex_t *futex, int32_t expected_val, uint32_t worker_id);
int webrt_sab_futex_wake(webrt_sab_futex_t *futex, int count, int *out_woken);

int webrt_wasm_simd_mat8_mul(const webrt_mat8x8_t *a, const webrt_mat8x8_t *b, webrt_mat8x8_t *out);
int webrt_wasm_simd_tensor_conv2d(webrt_tensor_conv_t *conv);

/* Lot 39: Web Runtime WebAssembly SIMD 256-bit Vector Acceleration Engine & WebGPU Compute Pipeline State Machine (US-059 / US-061) */
#define WEBRT_WEBGPU_BINDINGS_MAX 8
#define WEBRT_WEBGPU_BIND_GROUPS_MAX 4

typedef struct {
    int32_t val[8]; /* 256-bit SIMD vector containing 8 x int32_t or 16.16 fixed point elements */
} webrt_v256_t;

typedef struct {
    uint32_t binding_id;
    const void *buffer;
    uint32_t size;
} webrt_webgpu_bind_entry_t;

typedef struct {
    uint32_t group_id;
    webrt_webgpu_bind_entry_t entries[WEBRT_WEBGPU_BINDINGS_MAX];
    int entry_count;
} webrt_webgpu_bind_group_t;

typedef struct {
    uint32_t pipeline_id;
    uint32_t state; /* 0=uninitialized, 1=encoding, 2=dispatched, 3=completed */
    webrt_webgpu_bind_group_t bind_groups[WEBRT_WEBGPU_BIND_GROUPS_MAX];
    int bind_group_count;
    uint32_t dispatch_x;
    uint32_t dispatch_y;
    uint32_t dispatch_z;
    uint32_t dispatches_completed;
} webrt_webgpu_compute_pass_t;

void webrt_simd256_load(webrt_v256_t *v, const int32_t data[8]);
int webrt_wasm_simd256_dot(const webrt_v256_t *a, const webrt_v256_t *b, int64_t *out_dot);
int webrt_wasm_simd256_fma(const webrt_v256_t *a, const webrt_v256_t *b, const webrt_v256_t *c, webrt_v256_t *out);
int webrt_wasm_simd256_relu(const webrt_v256_t *in, webrt_v256_t *out);
int webrt_wasm_simd256_gelu(const webrt_v256_t *in, webrt_v256_t *out);

void webrt_webgpu_compute_pass_init(webrt_webgpu_compute_pass_t *pass, uint32_t pipeline_id);
int webrt_webgpu_bind_group_set(webrt_webgpu_compute_pass_t *pass, uint32_t group_id, const webrt_webgpu_bind_group_t *bg);
int webrt_webgpu_dispatch_workgroups(webrt_webgpu_compute_pass_t *pass, uint32_t x, uint32_t y, uint32_t z);
int webrt_webgpu_compute_pass_finish(webrt_webgpu_compute_pass_t *pass, int *out_completed_dispatches);

/* Lot 40: Web Runtime WebAssembly SIMD 512-bit AVX-512 Tensor Acceleration Engine & WebGPU Compute Pipeline Parallel Command Encoder (US-062 / US-087) */
#define WEBRT_WEBGPU_COMMAND_BUFFERS_MAX 4
#define WEBRT_WEBGPU_CMDS_PER_BUFFER 16

typedef struct {
    int32_t val[16]; /* 512-bit SIMD vector containing 16 x int32_t or 16.16 fixed point elements */
} webrt_v512_t;

typedef struct {
    uint32_t opcode; /* 1=dispatch */
    uint32_t pipeline_id;
    uint32_t dispatch_x;
    uint32_t dispatch_y;
    uint32_t dispatch_z;
} webrt_webgpu_cmd_t;

typedef struct {
    uint32_t buffer_id;
    webrt_webgpu_cmd_t cmds[WEBRT_WEBGPU_CMDS_PER_BUFFER];
    int cmd_count;
    uint8_t closed;
} webrt_webgpu_command_buffer_t;

typedef struct {
    uint32_t encoder_id;
    webrt_webgpu_command_buffer_t buffers[WEBRT_WEBGPU_COMMAND_BUFFERS_MAX];
    int buffer_count;
    uint32_t total_commands_encoded;
    uint8_t active;
} webrt_webgpu_parallel_encoder_t;

void webrt_simd512_load(webrt_v512_t *v, const int32_t data[16]);
int webrt_wasm_simd512_dot(const webrt_v512_t *a, const webrt_v512_t *b, int64_t *out_dot);
int webrt_wasm_simd512_fma(const webrt_v512_t *a, const webrt_v512_t *b, const webrt_v512_t *c, webrt_v512_t *out);
int webrt_wasm_simd512_silu(const webrt_v512_t *in, webrt_v512_t *out);
int webrt_wasm_simd512_softmax(const webrt_v512_t *in, webrt_v512_t *out);

void webrt_webgpu_parallel_encoder_init(webrt_webgpu_parallel_encoder_t *enc, uint32_t encoder_id);
int webrt_webgpu_parallel_create_command_buffer(webrt_webgpu_parallel_encoder_t *enc, uint32_t *out_buf_id);
int webrt_webgpu_command_buffer_encode_dispatch(webrt_webgpu_parallel_encoder_t *enc, uint32_t buf_id, uint32_t pipeline_id, uint32_t x, uint32_t y, uint32_t z);
int webrt_webgpu_parallel_encoder_finish(webrt_webgpu_parallel_encoder_t *enc, int *out_total_cmds);

#endif
