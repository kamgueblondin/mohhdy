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

#endif
