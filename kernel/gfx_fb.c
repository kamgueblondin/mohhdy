/* gfx_fb.c - Bochs/QEMU VBE linear framebuffer. Surface produit = fenetre QEMU. */
#include "gfx_fb.h"
#include "gfx_desktop.h"
#include "vga_console.h"
#include "mem/vmm.h"
#include "mem/heap.h"
#include "mem/string.h"
#include "pci.h"

#ifndef KERNEL_TEST
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

static int g_on;
static int g_w = GFX_FB_WIDTH;
static int g_h = GFX_FB_HEIGHT;
static volatile uint32_t *g_lfb;
static uint32_t *g_back;
static uint32_t g_lfb_phys;
static uint32_t g_lfb_bytes;
static int g_logged;

#ifndef KERNEL_TEST
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
    if (!kernel_directory) return -1;
    bytes = (bytes + 4095u) & ~4095u;
    for (off = 0; off < bytes; off += 4096u) {
        if (vmm_map_page_in_directory(kernel_directory,
                                      (void *)(phys + off),
                                      (void *)(phys + off),
                                      PAGE_PRESENT | PAGE_WRITE) != 0) {
            return -2;
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

static int vbe_enable(int width, int height) {
    unsigned short id;
    uint32_t bar;
    uint32_t bytes = (uint32_t)width * (uint32_t)height * 4u;

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

    bar = pci_vga_bar();
    if (!bar) bar = 0xE0000000u;
    if (map_range(bar, bytes) != 0) {
        if (bar != 0xE0000000u) {
            bar = 0xE0000000u;
            if (map_range(bar, bytes) != 0) return -3;
        } else {
            return -3;
        }
    }
    g_lfb_phys = bar;
    g_lfb_bytes = bytes;
    g_lfb = (volatile uint32_t *)bar;
    g_w = width;
    g_h = height;
    if (!g_back) g_back = (uint32_t *)kmalloc(bytes);
    return 0;
}

static void vga_text_restore(void) {
    dispi_write(VBE_DISPI_INDEX_ENABLE, VBE_DISPI_DISABLED);
    /* Mode texte VGA 80x25 : sequencer + misc. QEMU revient au plan 0xB8000. */
    outb(0x3C2, 0x67);
    outb(0x3C4, 0x00); outb(0x3C5, 0x03);
    outb(0x3C4, 0x01); outb(0x3C5, 0x00);
    outb(0x3C4, 0x02); outb(0x3C5, 0x03);
    outb(0x3C4, 0x03); outb(0x3C5, 0x00);
    outb(0x3C4, 0x04); outb(0x3C5, 0x02);
}

int gfx_fb_present(const os_fb_scene_t *scene) {
    uint32_t *dst;
    uint32_t n, i;
    os_fb_scene_t local;

    if (!scene || scene->magic != OS_FB_MAGIC) return -1;
    memcpy(&local, scene, sizeof(local));
    if (!g_on) {
        if (vbe_enable(GFX_FB_WIDTH, GFX_FB_HEIGHT) != 0) return -2;
        g_on = 1;
        vga_desktop_set(1);
        if (!g_logged) {
            print_string_serial("osui gui fb 1024x768 chrome=qemu_fb display_surface=vbe_lfb\n");
            g_logged = 1;
        }
    }
    dst = g_back ? g_back : (uint32_t *)g_lfb;
    gfx_desktop_draw(&local, dst, g_w, g_h);
    if (g_back && g_lfb) {
        n = (uint32_t)g_w * (uint32_t)g_h;
        for (i = 0; i < n; i++) g_lfb[i] = g_back[i];
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
    g_logged = 0;
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

#endif
