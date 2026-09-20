/* gfx_fb.c - Bochs/QEMU VBE linear framebuffer. Surface produit = fenetre QEMU.
 * La resolution suit la fenetre hote (COM2 WxH) ; sinon 1024x768 + zoom-to-fit.
 */
#include "gfx_fb.h"

#ifndef KERNEL_TEST
#include "gfx_desktop.h"
#include "vga_console.h"
#include "mem/vmm.h"
#include "mem/heap.h"
#include "mem/string.h"
#include "pci.h"
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

static int map_range(uint32_t phys, uint32_t bytes) {
    uint32_t off;
    vmm_directory_t *dirs[2];
    int d, nd = 0;
    if (!kernel_directory) return -1;
    dirs[nd++] = kernel_directory;
    if (current_directory && current_directory != kernel_directory)
        dirs[nd++] = current_directory;
    bytes = (bytes + 4095u) & ~4095u;
    for (d = 0; d < nd; d++) {
        for (off = 0; off < bytes; off += 4096u) {
            if (vmm_map_page_in_directory(dirs[d],
                                          (void *)(phys + off),
                                          (void *)(phys + off),
                                          PAGE_PRESENT | PAGE_WRITE) != 0) {
                return -2;
            }
        }
    }
    return 0;
}

static uint32_t pci_vga_bar(void) {
    uint8_t slot, function;
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

static void vga_text_restore(void) {
    dispi_write(VBE_DISPI_INDEX_ENABLE, VBE_DISPI_DISABLED);
    outb(0x3C2, 0x67);
    outb(0x3C4, 0x00); outb(0x3C5, 0x03);
    outb(0x3C4, 0x01); outb(0x3C5, 0x00);
    outb(0x3C4, 0x02); outb(0x3C5, 0x03);
    outb(0x3C4, 0x03); outb(0x3C5, 0x00);
    outb(0x3C4, 0x04); outb(0x3C5, 0x02);
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

int gfx_fb_present(const os_fb_scene_t *scene) {
    uint32_t *dst;
    uint32_t n, i;
    os_fb_scene_t local;
    int nw, nh;
    int mx = 0, my = 0;
    uint8_t btn = 0;
    int cx, cy;

    if (!scene || scene->magic != OS_FB_MAGIC) return -1;
    memcpy(&local, scene, sizeof(local));
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

void gfx_fb_leave(void) {
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
    vga_desktop_set(0);
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
int gfx_fb_active(void) { return 0; }
int gfx_fb_width(void) { return GFX_FB_WIDTH; }
int gfx_fb_height(void) { return GFX_FB_HEIGHT; }
void gfx_fb_update_cursor(void) {}

#endif
