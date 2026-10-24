/* gfx_fb.c - Bochs/QEMU VBE linear framebuffer. Surface produit = fenetre QEMU.
 * La resolution suit la fenetre hote (COM2 WxH) ; sinon 1024x768 + zoom-to-fit.
 */
#include "clipboard.h"
#include "gfx_fb.h"
#include "gfx_term.h"

#ifndef KERNEL_TEST
#include "gfx_desktop.h"
#include "vga_console.h"
#include "keyboard.h"
#include "mem/vmm.h"
#include "task/task.h"
#include "mem/heap.h"
#include "mem/string.h"
#include "pci.h"
#include "pci_service.h"
#include "kernel.h"
#endif

#define VBE_DISPI_IOPORT_INDEX 0x01CE
#define VBE_DISPI_IOPORT_DATA  0x01CF
#define VBE_DISPI_INDEX_ID     0x0
#define VBE_DISPI_INDEX_XRES   0x1
#define VBE_DISPI_INDEX_YRES   0x2
#define VBE_DISPI_INDEX_BPP    0x3
#define VBE_DISPI_INDEX_ENABLE 0x4
#define VBE_DISPI_INDEX_BANK   0x5
#define VBE_DISPI_INDEX_VIRT_WIDTH 0x6
#define VBE_DISPI_INDEX_VIRT_HEIGHT 0x7
#define VBE_DISPI_INDEX_X_OFFSET 0x8
#define VBE_DISPI_INDEX_Y_OFFSET 0x9
#define VBE_DISPI_DISABLED 0x00
#define VBE_DISPI_ENABLED 0x01
#define VBE_DISPI_LFB_ENABLED 0x40
#define VBE_DISPI_ID4 0xB0C4
#define VBE_DISPI_ID5 0xB0C5

#define COM2_BASE 0x2F8
#define GFX_LFB_MAP_BYTES (16u * 1024u * 1024u)
#define GFX_BACK_MAX_BYTES ((uint32_t)GFX_FB_MAX_WIDTH * (uint32_t)GFX_FB_MAX_HEIGHT * 4u)

#ifndef KERNEL_TEST
static inline uint32_t lock_interrupts(void) {
    uint32_t flags;
    asm volatile ("pushfl; pop %0; cli" : "=r"(flags));
    return flags;
}

static inline void unlock_interrupts(uint32_t flags) {
    asm volatile ("push %0; popfl" : : "r"(flags));
}

static int g_on;
static int g_w = GFX_FB_WIDTH;
static int g_h = GFX_FB_HEIGHT;
static int g_pitch = GFX_FB_WIDTH;
static volatile uint32_t *g_lfb;
static uint32_t *g_back;
static uint32_t g_back_bytes;
static uint32_t g_lfb_phys;
static uint32_t g_mapped;
static int g_logged;
static int g_fit_n;
static char g_fit_buf[28];
static int g_com2_ready;

static uint32_t g_cursor_saved_bg[18 * 12];
static int g_saved_cursor_x = -1;
static int g_saved_cursor_y = -1;
static uint8_t g_saved_cursor_btn = 0;
static int g_saved_cursor_valid = 0;
static void vga_font_save(void);
static int g_term;
static int g_term_full;
static gfx_term_layout_t g_term_l;
static uint16_t g_term_shown[GFX_TERM_MAX_COLS * GFX_TERM_MAX_ROWS];
static uint16_t g_term_cells[GFX_TERM_MAX_COLS * GFX_TERM_MAX_ROWS];
static int g_term_geom_dirty;      /* window grid != console grid: resize at a safe point */
void print_sync_cursor(void);
static gfx_wm_win_t g_term_wm;     /* close/min/max/drag state (gfx_wm.c) */
static int g_term_minimized;       /* left by its minimize button */
/* Last desktop scene: the terminal window is composed over it. */
static os_fb_scene_t local; /* Lot 6: too large for the kernel stack */
static int g_scene_valid;
#endif

int gfx_fb_parse_fit_line(const char *s, int *w, int *h) {
    unsigned vw = 0, vh = 0;
    int seen_w = 0;
    if (!s || !w || !h) return 0;
    while (*s == ' ' || *s == '\t') s++;
    if (*s < '0' || *s > '9') return 0;
    while (*s >= '0' && *s <= '9') {
        vw = vw * 10u + (unsigned)(*s - '0');
        s++;
        seen_w = 1;
    }
    while (*s == ' ' || *s == '\t' || *s == 'x' || *s == 'X' || *s == ',') s++;
    if (*s < '0' || *s > '9') return 0;
    while (*s >= '0' && *s <= '9') {
        vh = vh * 10u + (unsigned)(*s - '0');
        s++;
    }
    if (!seen_w || vh == 0 || vw == 0) return 0;
    if (vw > GFX_FB_MAX_WIDTH) vw = GFX_FB_MAX_WIDTH;
    if (vh > GFX_FB_MAX_HEIGHT) vh = GFX_FB_MAX_HEIGHT;
    if (vw < GFX_FB_MIN_WIDTH) vw = GFX_FB_MIN_WIDTH;
    if (vh < GFX_FB_MIN_HEIGHT) vh = GFX_FB_MIN_HEIGHT;
    vw &= ~1u;
    vh &= ~1u;
    *w = (int)vw;
    *h = (int)vh;
    return 1;
}

#ifndef KERNEL_TEST
extern void write_serial(char c);

static void serial_uint(unsigned v) {
    char buf[12];
    int n = 0;
    if (v == 0) {
        write_serial('0');
        return;
    }
    while (v && n < 11) {
        buf[n++] = (char)('0' + (v % 10u));
        v /= 10u;
    }
    while (n--) write_serial(buf[n]);
}

static void log_fb_size(const char *tag, int width, int height) {
    print_string_serial(tag);
    serial_uint((unsigned)width);
    write_serial('x');
    serial_uint((unsigned)height);
    print_string_serial(" chrome=qemu_fb display_surface=vbe_lfb\n");
}

static void outw(unsigned short port, unsigned short val) {
    asm volatile("outw %0, %1" : : "a"(val), "Nd"(port));
}

static unsigned short inw(unsigned short port) {
    unsigned short val;
    asm volatile("inw %1, %0" : "=a"(val) : "Nd"(port));
    return val;
}

static void dispi_write(unsigned short index, unsigned short value) {
    outw(VBE_DISPI_IOPORT_INDEX, index);
    outw(VBE_DISPI_IOPORT_DATA, value);
}

static unsigned short dispi_read(unsigned short index) {
    outw(VBE_DISPI_IOPORT_INDEX, index);
    return inw(VBE_DISPI_IOPORT_DATA);
}

/* The LFB is mapped (supervisor RW) in the kernel directory, then its page
 * tables are shared with every live task directory. Before, it was mapped
 * only in the kernel directory and in the directory current at init (the
 * shell): IRQ0 redraws the cursor (timer.c) whatever CR3 is loaded, and a
 * task created before the desktop started (atadriver, vfsserver, aiworker
 * spawned at boot) faulted on the LFB write (qemu-osui-gui, EIP in
 * gfx_fb_update_cursor). Directories created later copy the kernel tables. */
static int map_range(uint32_t phys, uint32_t bytes) {
    uint32_t off, first, last;
    if (!kernel_directory) return -1;
    bytes = (bytes + 4095u) & ~4095u;
    if (bytes == 0u) return -1;
    for (off = 0; off < bytes; off += 4096u) {
        if (vmm_map_page_in_directory(kernel_directory,
                                      (void *)(phys + off),
                                      (void *)(phys + off),
                                      PAGE_PRESENT | PAGE_WRITE) != 0) {
            return -2;
        }
    }
    first = phys >> 22;
    last = (phys + bytes - 1u) >> 22;
    {
        /* Diagnostic: tables pushed to directories that lacked the LFB. */
        uint32_t shared = task_share_kernel_tables(first, last - first + 1u);
        char line[] = "[GFX] LFB page tables pushed to task directories: 000\n";
        uint32_t pos = sizeof(line) - 5u;
        if (shared > 999u) shared = 999u;
        line[pos] = (char)('0' + shared / 100u);
        line[pos + 1u] = (char)('0' + (shared / 10u) % 10u);
        line[pos + 2u] = (char)('0' + shared % 10u);
        print_string_serial(line);
    }
    return 0;
}

static uint32_t pci_vga_bar(void) {
    uint8_t slot, function;
    /* Lot 4 (US-001): after the Ring 3 pcidriver published the inventory,
     * the BAR comes from it; Ring 0 no longer scans config space. */
    if (pci_service_handed_off()) {
        os_pci_record_t vga;
        if (pci_service_find_class(0x03U, 0U, 1U, &vga) != 0 || (vga.bar0 & 1U)) return 0;
        return vga.bar0 & 0xfffffff0u;
    }
    for (slot = 0; slot < 32; slot++) {
        for (function = 0; function < 8; function++) {
            uint32_t id = pci_config_read32(0, slot, function, 0);
            uint32_t class_info, orig;
            if ((id & 0xffffu) == 0xffffu) continue;
            class_info = pci_config_read32(0, slot, function, 8);
            if ((class_info >> 24) != 0x03u) continue;
            orig = pci_config_read32(0, slot, function, 0x10);
            if (orig & 1u) continue;
            orig &= 0xfffffff0u;
            if (orig) return orig;
        }
    }
    return 0;
}

static void com2_init(void) {
    if (g_com2_ready) return;
    outb(COM2_BASE + 1, 0x00);
    outb(COM2_BASE + 3, 0x80);
    outb(COM2_BASE + 0, 0x03);
    outb(COM2_BASE + 1, 0x00);
    outb(COM2_BASE + 3, 0x03);
    outb(COM2_BASE + 2, 0xC7);
    outb(COM2_BASE + 4, 0x0B);
    g_com2_ready = 1;
}

static int com2_poll_size(int *w, int *h) {
    int got = 0;
    int nread = 0;
    unsigned char lsr;
    com2_init();
    lsr = inb(COM2_BASE + 5);
    if (lsr == 0xff) return 0;
    while ((lsr & 0x01) && nread < 64) {
        char c = (char)inb(COM2_BASE);
        nread++;
        if (c == '\r') {
            lsr = inb(COM2_BASE + 5);
            continue;
        }
        if (c == '\n') {
            g_fit_buf[g_fit_n] = 0;
            g_fit_n = 0;
            if (gfx_fb_parse_fit_line(g_fit_buf, w, h)) got = 1;
            lsr = inb(COM2_BASE + 5);
            continue;
        }
        if (g_fit_n < (int)sizeof(g_fit_buf) - 1) g_fit_buf[g_fit_n++] = c;
        else g_fit_n = 0;
        lsr = inb(COM2_BASE + 5);
    }
    return got;
}

static int vbe_set_mode(int width, int height) {
    unsigned short id;
    uint32_t bar;
    uint32_t bytes;

    if (!g_back) {
        g_back = (uint32_t *)kmalloc(GFX_BACK_MAX_BYTES);
        if (g_back) g_back_bytes = GFX_BACK_MAX_BYTES;
        else {
            g_back = (uint32_t *)kmalloc((uint32_t)GFX_FB_WIDTH * (uint32_t)GFX_FB_HEIGHT * 4u);
            if (g_back) g_back_bytes = (uint32_t)GFX_FB_WIDTH * (uint32_t)GFX_FB_HEIGHT * 4u;
        }
    }
    bytes = (uint32_t)width * (uint32_t)height * 4u;
    if (g_back && bytes > g_back_bytes) {
        width = GFX_FB_WIDTH;
        height = GFX_FB_HEIGHT;
        bytes = (uint32_t)width * (uint32_t)height * 4u;
    }

    dispi_write(VBE_DISPI_INDEX_ID, VBE_DISPI_ID5);
    id = dispi_read(VBE_DISPI_INDEX_ID);
    if (id < VBE_DISPI_ID4 || id > VBE_DISPI_ID5 + 4) {
        dispi_write(VBE_DISPI_INDEX_ID, VBE_DISPI_ID4);
        id = dispi_read(VBE_DISPI_INDEX_ID);
        if (id < VBE_DISPI_ID4) return -1;
    }

    if (!g_on) vga_font_save();
    dispi_write(VBE_DISPI_INDEX_ENABLE, VBE_DISPI_DISABLED);
    dispi_write(VBE_DISPI_INDEX_XRES, (unsigned short)width);
    dispi_write(VBE_DISPI_INDEX_YRES, (unsigned short)height);
    dispi_write(VBE_DISPI_INDEX_BPP, 32);
    dispi_write(VBE_DISPI_INDEX_VIRT_WIDTH, (unsigned short)width);
    dispi_write(VBE_DISPI_INDEX_VIRT_HEIGHT, (unsigned short)height);
    dispi_write(VBE_DISPI_INDEX_X_OFFSET, 0);
    dispi_write(VBE_DISPI_INDEX_Y_OFFSET, 0);
    dispi_write(VBE_DISPI_INDEX_ENABLE, (unsigned short)(VBE_DISPI_ENABLED | VBE_DISPI_LFB_ENABLED));

    if (!g_lfb) {
        bar = pci_vga_bar();
        if (!bar) bar = 0xE0000000u;
        if (map_range(bar, GFX_LFB_MAP_BYTES) != 0) {
            if (bar != 0xE0000000u) {
                bar = 0xE0000000u;
                if (map_range(bar, GFX_LFB_MAP_BYTES) != 0) return -3;
            } else {
                return -3;
            }
        }
        g_lfb_phys = bar;
        g_mapped = GFX_LFB_MAP_BYTES;
        g_lfb = (volatile uint32_t *)bar;
    }
    {
        unsigned short cw = dispi_read(VBE_DISPI_INDEX_XRES);
        unsigned short ch = dispi_read(VBE_DISPI_INDEX_YRES);
        unsigned short cvw = dispi_read(VBE_DISPI_INDEX_VIRT_WIDTH);
        if (cw >= GFX_FB_MIN_WIDTH && cw <= GFX_FB_MAX_WIDTH) width = (int)cw;
        if (ch >= GFX_FB_MIN_HEIGHT && ch <= GFX_FB_MAX_HEIGHT) height = (int)ch;
        g_w = width;
        g_h = height;
        g_pitch = (cvw >= (unsigned short)width) ? (int)cvw : width;
    }
    return 0;
}

/* Standard VGA mode 3 (80x25 text, 9x16 cells) register set.
 * Enabling the Bochs DISPI LFB rewrites the CRTC, graphics controller and
 * attribute controller for 1024x768 graphics; clearing DISPI_ENABLE does
 * not undo that. Only resetting misc and the sequencer left the card in
 * graphics mode, so the text cells at 0xB8000 were scanned as pixels: the
 * full screen magenta/green stripes seen after ESC. The LFB also overwrites
 * plane 2, where the text font lives, so the font is saved before the
 * first switch and written back here. */
static const uint8_t k_mode3_crtc[25] = {
    0x5F, 0x4F, 0x50, 0x82, 0x55, 0x81, 0xBF, 0x1F, 0x00, 0x4F, 0x0D, 0x0E,
    0x00, 0x00, 0x00, 0x50, 0x9C, 0x0E, 0x8F, 0x28, 0x1F, 0x96, 0xB9, 0xA3, 0xFF
};
static const uint8_t k_mode3_gc[9] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x10, 0x0E, 0x00, 0xFF };
static const uint8_t k_mode3_ac[21] = {
    0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x14, 0x07, 0x38, 0x39, 0x3A, 0x3B,
    0x3C, 0x3D, 0x3E, 0x3F, 0x0C, 0x00, 0x0F, 0x08, 0x00
};
#define VGA_FONT_BYTES (256u * 32u)
static uint8_t g_font_plane2[VGA_FONT_BYTES];
static int g_font_saved;

static void vga_seq(uint8_t i, uint8_t v) { outb(0x3C4, i); outb(0x3C5, v); }
static void vga_gc(uint8_t i, uint8_t v) { outb(0x3CE, i); outb(0x3CF, v); }

/* Plane 2 at 0xA0000, flat addressing. */
static void vga_plane2_open(uint8_t write_mask) {
    vga_seq(0x00, 0x01);
    vga_seq(0x02, write_mask);
    vga_seq(0x04, 0x06);
    vga_seq(0x00, 0x03);
    vga_gc(0x04, 0x02);
    vga_gc(0x05, 0x00);
    vga_gc(0x06, 0x04);
}

static void vga_plane2_close(void) {
    vga_seq(0x00, 0x01);
    vga_seq(0x02, 0x03);
    vga_seq(0x04, 0x02);
    vga_seq(0x00, 0x03);
    vga_gc(0x04, 0x00);
    vga_gc(0x05, 0x10);
    vga_gc(0x06, 0x0E);
}

static void vga_font_save(void) {
    volatile uint8_t *mem = (volatile uint8_t *)0xA0000u;
    uint32_t i;
    if (g_font_saved) return;
    vga_plane2_open(0x04);
    for (i = 0; i < VGA_FONT_BYTES; i++) g_font_plane2[i] = mem[i];
    vga_plane2_close();
    g_font_saved = 1;
}

static void vga_text_restore(void) {
    unsigned i;
    dispi_write(VBE_DISPI_INDEX_ENABLE, VBE_DISPI_DISABLED);
    outb(0x3C2, 0x67);
    vga_seq(0x00, 0x01);
    vga_seq(0x01, 0x00);
    vga_seq(0x02, 0x03);
    vga_seq(0x03, 0x00);
    vga_seq(0x04, 0x02);
    vga_seq(0x00, 0x03);
    outb(0x3D4, 0x11); outb(0x3D5, (uint8_t)(inb(0x3D5) & 0x7F));
    for (i = 0; i < sizeof(k_mode3_crtc); i++) { outb(0x3D4, (uint8_t)i); outb(0x3D5, k_mode3_crtc[i]); }
    for (i = 0; i < sizeof(k_mode3_gc); i++) vga_gc((uint8_t)i, k_mode3_gc[i]);
    (void)inb(0x3DA);
    for (i = 0; i < sizeof(k_mode3_ac); i++) { outb(0x3C0, (uint8_t)i); outb(0x3C0, k_mode3_ac[i]); }
    outb(0x3C0, 0x20);
    if (g_font_saved) {
        volatile uint8_t *mem = (volatile uint8_t *)0xA0000u;
        vga_plane2_open(0x04);
        for (i = 0; i < VGA_FONT_BYTES; i++) mem[i] = g_font_plane2[i];
        vga_plane2_close();
    }
}

void gfx_fb_update_cursor(void) {
    int mx = 0, my = 0;
    uint8_t btn = 0;
    int pitch;
    int cx, cy;
    uint32_t irq_flags;

    if (!g_on || !g_lfb) return;

    irq_flags = lock_interrupts();

    gfx_desktop_get_mouse(&mx, &my, &btn);
    if (mx < 0) mx = g_w / 2;
    if (my < 0) my = g_h / 2;
    if (mx >= g_w) mx = g_w - 1;
    if (my >= g_h) my = g_h - 1;

    if (g_saved_cursor_valid && mx == g_saved_cursor_x && my == g_saved_cursor_y && btn == g_saved_cursor_btn) {
        unlock_interrupts(irq_flags);
        return;
    }

    pitch = (g_pitch > 0) ? g_pitch : g_w;

    /* 1. Restore old cursor background */
    if (g_saved_cursor_valid) {
        for (cy = 0; cy < 18; cy++) {
            for (cx = 0; cx < 12; cx++) {
                int px = g_saved_cursor_x + cx;
                int py = g_saved_cursor_y + cy;
                if (px >= 0 && px < g_w && py >= 0 && py < g_h) {
                    uint32_t c = g_cursor_saved_bg[cy * 12 + cx];
                    if (g_back) g_back[py * g_w + px] = c;
                    g_lfb[py * pitch + px] = c;
                }
            }
        }
    }

    /* 2. Save new cursor background */
    for (cy = 0; cy < 18; cy++) {
        for (cx = 0; cx < 12; cx++) {
            int px = mx + cx;
            int py = my + cy;
            if (px >= 0 && px < g_w && py >= 0 && py < g_h) {
                g_cursor_saved_bg[cy * 12 + cx] = g_back ? g_back[py * g_w + px] : g_lfb[py * pitch + px];
            } else {
                g_cursor_saved_bg[cy * 12 + cx] = 0;
            }
        }
    }

    /* 3. Draw cursor at new position */
    if (g_back) {
        gfx_desktop_draw_cursor(g_back, g_w, g_h, g_w, mx, my, btn);
    }
    gfx_desktop_draw_cursor((uint32_t *)g_lfb, g_w, g_h, pitch, mx, my, btn);

    g_saved_cursor_x = mx;
    g_saved_cursor_y = my;
    g_saved_cursor_btn = btn;
    g_saved_cursor_valid = 1;

    unlock_interrupts(irq_flags);
}

static int present_local(void);
static volatile int g_presenting, g_present_again;

/* A mouse IRQ may ask for a redraw while a present runs: draw again at
 * the end instead of drawing into g_back twice at the same time. */
static int present_guarded(void) {
    int rc;
    if (g_presenting) { g_present_again = 1; return 0; }
    g_presenting = 1;
    do {
        g_present_again = 0;
        rc = present_local();
    } while (g_present_again && rc == 0);
    g_presenting = 0;
    return rc;
}

int gfx_fb_present(const os_fb_scene_t *scene) {
    if (!scene || scene->magic != OS_FB_MAGIC) return -1;
    if (g_term) {
        /* back from the terminal window to the desktop loop: the console
         * gets the text mode grid again (80x25, reflowed) */
        g_term = 0;
        vga_console_set_gfx_sink(0);
        vga_desktop_set(1);
        if (vga_console_resize(VGA_COLS, VGA_ROWS)) print_sync_cursor();
        g_saved_cursor_valid = 0;
    }
    memcpy(&local, scene, sizeof(local));
    g_scene_valid = 1;
    return present_guarded();
}

/* Window manager changes (minimize, drag ...) redraw the last scene
 * without waiting for the Ring 3 desktop loop. */
void gfx_fb_redraw_desktop(void) {
    if (!g_on || g_term || !g_scene_valid) return;
    (void)present_guarded();
}

static int present_local(void) {
    uint32_t *dst;
    uint32_t n, i;
    int nw, nh;
    int mx = 0, my = 0;
    uint8_t btn = 0;
    int cx, cy;
    if (com2_poll_size(&nw, &nh)) {
        if (!g_on || nw != g_w || nh != g_h) {
            if (vbe_set_mode(nw, nh) == 0) {
                g_on = 1;
                g_saved_cursor_valid = 0;
                vga_desktop_set(1);
                log_fb_size(g_logged ? "osui gui fb resize " : "osui gui fb ", nw, nh);
                g_logged = 1;
            }
        }
    }
    if (!g_on) {
        if (vbe_set_mode(GFX_FB_WIDTH, GFX_FB_HEIGHT) != 0) return -2;
        g_on = 1;
        g_saved_cursor_valid = 0;
        vga_desktop_set(1);
        if (!g_logged) {
            log_fb_size("osui gui fb ", g_w, g_h);
            g_logged = 1;
        }
    }
    dst = g_back ? g_back : (uint32_t *)g_lfb;
    gfx_desktop_draw_no_cursor(&local, dst, g_w, g_h);

    {
        uint32_t irq_flags = lock_interrupts();

        gfx_desktop_get_mouse(&mx, &my, &btn);
        if (mx < 0) mx = g_w / 2;
        if (my < 0) my = g_h / 2;
        if (mx >= g_w) mx = g_w - 1;
        if (my >= g_h) my = g_h - 1;

        for (cy = 0; cy < 18; cy++) {
            for (cx = 0; cx < 12; cx++) {
                int px = mx + cx;
                int py = my + cy;
                if (px >= 0 && px < g_w && py >= 0 && py < g_h) {
                    g_cursor_saved_bg[cy * 12 + cx] = dst[py * g_w + px];
                } else {
                    g_cursor_saved_bg[cy * 12 + cx] = 0;
                }
            }
        }
        g_saved_cursor_x = mx;
        g_saved_cursor_y = my;
        g_saved_cursor_btn = btn;
        g_saved_cursor_valid = 1;

        gfx_desktop_draw_cursor(dst, g_w, g_h, g_w, mx, my, btn);

        if (g_back && g_lfb) {
            int pitch = (g_pitch > 0) ? g_pitch : g_w;
            if (pitch == g_w) {
                n = (uint32_t)g_w * (uint32_t)g_h;
                for (i = 0; i < n; i++) g_lfb[i] = g_back[i];
            } else {
                int y, x;
                for (y = 0; y < g_h; y++) {
                    uint32_t src_row = (uint32_t)y * (uint32_t)g_w;
                    uint32_t dst_row = (uint32_t)y * (uint32_t)pitch;
                    for (x = 0; x < g_w; x++) {
                        g_lfb[dst_row + x] = g_back[src_row + x];
                    }
                }
            }
        }
        unlock_interrupts(irq_flags);
    }
    return 0;
}

/* Soft cursor: take it off both surfaces before they are redrawn under it
 * (the old code restored desktop pixels saved in g_back over the terminal,
 * which only lived in the LFB: hovering erased the terminal). */
static void cursor_hide(void) {
    int cx, cy, pitch = (g_pitch > 0) ? g_pitch : g_w;
    if (!g_saved_cursor_valid || !g_lfb) return;
    for (cy = 0; cy < 18; cy++)
        for (cx = 0; cx < 12; cx++) {
            int px = g_saved_cursor_x + cx, py = g_saved_cursor_y + cy;
            if (px >= 0 && px < g_w && py >= 0 && py < g_h) {
                uint32_t c = g_cursor_saved_bg[cy * 12 + cx];
                if (g_back) g_back[py * g_w + px] = c;
                g_lfb[py * pitch + px] = c;
            }
        }
    g_saved_cursor_valid = 0;
}

static void blit_rect(int x, int y, int w, int h) {
    int r, c, pitch = (g_pitch > 0) ? g_pitch : g_w;
    if (!g_back || !g_lfb) return;
    if (x < 0) { w += x; x = 0; }
    if (y < 0) { h += y; y = 0; }
    if (x + w > g_w) w = g_w - x;
    if (y + h > g_h) h = g_h - y;
    for (r = y; r < y + h; r++)
        for (c = x; c < x + w; c++) g_lfb[r * pitch + c] = g_back[r * g_w + c];
}

static uint32_t *term_surface(int *pitch) {
    *pitch = g_back ? g_w : ((g_pitch > 0) ? g_pitch : g_w);
    return g_back ? g_back : (uint32_t *)g_lfb;
}

/* Layout of the window; the grid drawn is the console grid until
 * gfx_fb_term_poll resized the console to the window (not from the mouse
 * path: the console may be in the middle of an update). */
static void term_layout_now(gfx_term_layout_t *l) {
    gfx_term_layout_at(g_w, g_h, g_term_wm.maximized, g_term_wm.dx, g_term_wm.dy, l);
    if (g_term && (l->cols != vga_console_cols() || l->rows != vga_console_rows())) {
        g_term_geom_dirty = 1;
        l->cols = vga_console_cols();
        l->rows = vga_console_rows();
    }
}

static void term_cells_now(void) {
    vga_console_frame_copy_n(g_term_cells, GFX_TERM_MAX_COLS * GFX_TERM_MAX_ROWS, 0, 0);
}

/* Window frame and every cell, composed in g_back then shown. With
 * desktop set the desktop is redrawn first (window moved or resized). */
static void term_compose(const uint16_t *cells, int desktop) {
    gfx_term_layout_t l;
    int pitch;
    uint32_t *dst = term_surface(&pitch);
    term_layout_now(&l);
    if (desktop && g_scene_valid && g_back) gfx_desktop_draw_no_cursor(&local, g_back, g_w, g_h);
    g_term_l = l;
    gfx_term_draw_frame(dst, g_w, g_h, pitch, &l, g_font_plane2,
                        "Terminal MOHHDY = console");
    (void)gfx_term_render(dst, g_w, g_h, pitch, &l, g_font_plane2, cells, g_term_shown, 1);
    if (desktop) blit_rect(0, 0, g_w, g_h);
    else blit_rect(l.x, l.y, l.w, l.h);
}

/* The console in a VBE window: vga_console.c composes the frame as in
 * text mode and hands it here instead of to the VGA text page. */
static void term_sink(const uint16_t *cells, int cols, int rows, int cx, int cy, int visible) {
    gfx_term_layout_t l;
    int pitch, lo, hi;
    uint32_t *dst;
    uint32_t flags;
    (void)cols; (void)rows; /* = vga_console_cols/rows, used by term_layout_now */
    (void)cx; (void)cy; (void)visible; /* the cursor cell is already inverted */
    if (!g_term || !g_on || !g_lfb) return;
    flags = lock_interrupts();
    cursor_hide();
    term_layout_now(&l);
    if (g_term_full || l.w != g_term_l.w || l.h != g_term_l.h || l.x != g_term_l.x || l.y != g_term_l.y
        || l.cols != g_term_l.cols || l.rows != g_term_l.rows) {
        term_compose(cells, !g_term_full);
        g_term_full = 0;
    } else {
        dst = term_surface(&pitch);
        (void)gfx_term_render_rows(dst, g_w, g_h, pitch, &l, g_font_plane2, cells, g_term_shown, 0, &lo, &hi);
        if (lo >= 0)
            blit_rect(l.gx, l.gy + lo * GFX_TERM_CELL_H * l.scale,
                      l.cols * GFX_TERM_CELL_W * l.scale, (hi - lo + 1) * GFX_TERM_CELL_H * l.scale);
    }
    unlock_interrupts(flags);
    gfx_fb_update_cursor();
}

/* Mouse on the terminal window (IRQ context): the shared window manager
 * (gfx_wm.c) decides; this only applies the result to the console. */
/* Mouse selection in the terminal: drag selects (inverted cells), the
 * release copies to the clipboard, the right button pastes it as typed
 * keys (newlines become spaces: a paste never runs a command). */
static uint16_t g_sel_cells[GFX_TERM_MAX_COLS * GFX_TERM_MAX_ROWS];
static int g_sel_drag, g_sel_on, g_sel_a, g_sel_b;
static char g_clip[OS_CLIP_MAX + 1];   /* paste buffer, filled from clipboard.c */
static int g_clip_n;

static void sel_show(void) {
    int n = g_term_l.cols * g_term_l.rows;
    uint32_t flags = lock_interrupts();
    term_cells_now();
    for (int i = 0; i < n && i < GFX_TERM_MAX_COLS * GFX_TERM_MAX_ROWS; i++) g_sel_cells[i] = g_term_cells[i];
    if (g_sel_on) gfx_term_sel_apply(g_sel_cells, n, g_sel_a, g_sel_b);
    cursor_hide();
    term_compose(g_sel_cells, 0);
    unlock_interrupts(flags);
}

static void term_select(int x, int y, int left, int was, int right_press) {
    int col, row, in = gfx_term_cell_at(&g_term_l, x, y, &col, &row);
    int idx = row * g_term_l.cols + col;
    if (right_press && clipboard_len() > 0) {
        int i;
        g_clip_n = clipboard_get(g_clip, (int)sizeof(g_clip));
        for (i = 0; i < g_clip_n; i++) kbd_put_char(g_clip[i] == '\n' ? ' ' : g_clip[i]);
        print_string_serial("gfx term paste ");
        serial_uint((unsigned)g_clip_n);
        write_serial('\n');
        return;
    }
    if (left && !was) {
        int had = g_sel_on;
        g_sel_drag = in;
        g_sel_on = 0;
        if (in) g_sel_a = g_sel_b = idx;
        if (had) sel_show();
        return;
    }
    if (left && g_sel_drag && in && idx != g_sel_b) {
        g_sel_b = idx;
        g_sel_on = g_sel_a != g_sel_b;
        sel_show();
        return;
    }
    if (!left && was && g_sel_drag) {
        g_sel_drag = 0;
        if (g_sel_on) {
            int cols = 0, rows = 0;
            vga_console_frame_copy_n(g_term_cells, GFX_TERM_MAX_COLS * GFX_TERM_MAX_ROWS, &cols, &rows);
            g_clip_n = gfx_term_sel_text(g_term_cells, cols, rows, g_sel_a, g_sel_b, g_clip, (int)sizeof(g_clip));
            (void)clipboard_set(g_clip, g_clip_n);
            print_string_serial("gfx term copy ");
            serial_uint((unsigned)g_clip_n);
            write_serial('\n');
        }
    }
}

void gfx_fb_term_mouse(int x, int y, uint8_t buttons, uint8_t prev) {
    int left = buttons & 1, was = prev & 1, act;
    gfx_term_layout_t c;
    gfx_wm_rect_t placed, base;
    if (!g_term) return;
    gfx_term_layout_at(g_w, g_h, 0, 0, 0, &c);
    base.x = c.x; base.y = c.y; base.w = c.w; base.h = c.h;
    placed.x = g_term_l.x; placed.y = g_term_l.y; placed.w = g_term_l.w; placed.h = g_term_l.h;
    act = gfx_wm_mouse(&g_term_wm, placed, base, GFX_TERM_TITLE_H * g_term_l.scale, g_term_l.scale,
                       x, y, left && !was, !left && was);
    if (act == GFX_WM_ACT_CLOSE || act == GFX_WM_ACT_MINIMIZE) {
        /* the console cannot be destroyed: both go back to the desktop.
         * Minimize keeps the half typed line and marks the dock icon. */
        g_term_minimized = act == GFX_WM_ACT_MINIMIZE;
        g_term_wm.closed = g_term_wm.minimized = 0;
        print_string_serial(act == GFX_WM_ACT_CLOSE ? "gfx term button close\n" : "gfx term button minimize\n");
        kbd_put_char((char)(act == GFX_WM_ACT_CLOSE ? OS_TERM_KEY_CLOSE : OS_TERM_KEY_MIN));
    } else if (act == GFX_WM_ACT_MAXIMIZE || act == GFX_WM_ACT_MOVED) {
        if (act == GFX_WM_ACT_MAXIMIZE)
            print_string_serial(g_term_wm.maximized ? "gfx term button maximize on\n" : "gfx term button maximize off\n");
        else
            print_string_serial("gfx term moved\n");
        term_cells_now();
        cursor_hide();
        term_compose(g_term_cells, 1);
    } else if (!g_term_wm.drag && (act == GFX_WM_ACT_BODY || act == GFX_WM_ACT_NONE || g_sel_drag ||
                                   ((buttons & 2) && !(prev & 2)))) {
        term_select(x, y, left, was, (buttons & 2) && !(prev & 2));
    }
}

int gfx_fb_term_minimized(void) { return g_term_minimized; }

/* Safe point (SYS_GETS idle loop): follow a new screen size sent on COM2
 * (QEMU window resize, scripts/qemu_gui_fit.py) and give the console the
 * grid of the window (maximize, resize). The shell output reflows to the
 * new width (vga_console_resize). */
void gfx_fb_term_poll(void) {
    gfx_term_layout_t l;
    int nw, nh;
    if (!g_term || !g_on || !g_lfb) return;
    if (com2_poll_size(&nw, &nh) && (nw != g_w || nh != g_h)) {
        uint32_t flags = lock_interrupts();
        cursor_hide();
        if (vbe_set_mode(nw, nh) == 0) log_fb_size("gfx term fb resize ", g_w, g_h);
        g_saved_cursor_valid = 0;
        g_term_geom_dirty = 1;
        unlock_interrupts(flags);
    }
    if (!g_term_geom_dirty) return;
    g_term_geom_dirty = 0;
    gfx_term_layout_at(g_w, g_h, g_term_wm.maximized, g_term_wm.dx, g_term_wm.dy, &l);
    if (l.cols != vga_console_cols() || l.rows != vga_console_rows()) {
        g_term_full = 1;
        (void)vga_console_resize(l.cols, l.rows);
        print_sync_cursor();
        print_string_serial("gfx term grid ");
        serial_uint((unsigned)vga_console_cols()); write_serial('x'); serial_uint((unsigned)vga_console_rows());
        print_string_serial(" reflow\n");
    }
    {
        uint32_t flags = lock_interrupts();
        term_cells_now();
        cursor_hide();
        term_compose(g_term_cells, 1);
        unlock_interrupts(flags);
    }
    gfx_fb_update_cursor();
}

int gfx_fb_term_enter(void) {
    gfx_term_layout_t l;
    if (!g_on || !g_lfb || !g_font_saved) return -1;
    g_term = 1;
    g_term_full = 1;
    g_term_wm.drag = 0;
    g_term_minimized = 0;
    term_layout_now(&l);
    print_string_serial("gfx term on fb ");
    serial_uint((unsigned)g_w); write_serial('x'); serial_uint((unsigned)g_h);
    print_string_serial(" grid ");
    serial_uint((unsigned)l.cols); write_serial('x'); serial_uint((unsigned)l.rows);
    print_string_serial(" scale ");
    serial_uint((unsigned)l.scale);
    print_string_serial(" window ");
    serial_uint((unsigned)l.x); write_serial(','); serial_uint((unsigned)l.y); write_serial(' ');
    serial_uint((unsigned)l.w); write_serial('x'); serial_uint((unsigned)l.h);
    print_string_serial(" backend=vbe same_console=vga_console\n");
    vga_console_set_gfx_sink(term_sink);
    return 0;
}

int gfx_fb_term_active(void) { return g_term; }

void gfx_fb_leave(void) {
    int was_term = g_term;
    if (g_term) {
        g_term = 0;
        vga_console_set_gfx_sink(0);
    }
    if (!g_on) {
        vga_desktop_set(0);
        return;
    }
    vga_text_restore();
    g_on = 0;
    g_lfb = 0;
    g_mapped = 0;
    g_logged = 0;
    g_fit_n = 0;
    g_saved_cursor_valid = 0;
    vga_console_vram_lost();
    if (was_term) {
        /* keep what the terminal showed, reflowed to 80x25 text mode */
        if (vga_console_resize(VGA_COLS, VGA_ROWS)) print_sync_cursor();
        vga_console_redraw();
    }
    else vga_desktop_set(0);
}

int gfx_fb_active(void) { return g_on; }
int gfx_fb_width(void) { return g_w; }
int gfx_fb_height(void) { return g_h; }

#else

int gfx_fb_present(const os_fb_scene_t *scene) {
    (void)scene;
    return 0;
}
void gfx_fb_leave(void) {}
int gfx_fb_term_enter(void) { return -1; }
int gfx_fb_term_active(void) { return 0; }
void gfx_fb_term_mouse(int x, int y, uint8_t buttons, uint8_t prev) { (void)x; (void)y; (void)buttons; (void)prev; }
int gfx_fb_term_minimized(void) { return 0; }
void gfx_fb_term_poll(void) {}
void gfx_fb_redraw_desktop(void) {}
int gfx_fb_active(void) { return 0; }
int gfx_fb_width(void) { return GFX_FB_WIDTH; }
int gfx_fb_height(void) { return GFX_FB_HEIGHT; }
void gfx_fb_update_cursor(void) {}

#endif
