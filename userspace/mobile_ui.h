/* Lot 8 (US-077): mobile shell of the guest (phone and tablet layouts).
 * Pure C, no syscalls: layout, touch hit testing, gestures (through the
 * US-079 classifier of platform.c), navigation state and a pixel model
 * that the shell hashes. No mobile hardware: input comes from the shell
 * command or the tests. */
#ifndef MOBILE_UI_H
#define MOBILE_UI_H

#include <stdint.h>
#include "platform.h"

#define MUI_MIN_TARGET 48        /* px, touch target floor */
#define MUI_APPS 8
#define MUI_MAX_TARGETS 16
#define MUI_NAV_BACK 100
#define MUI_NAV_HOME 101
#define MUI_NAV_RECENTS 102
#define MUI_STACK 8

enum { MUI_SCREEN_HOME = 0, MUI_SCREEN_APP, MUI_SCREEN_RECENTS };
enum { MUI_ACT_NONE = 0, MUI_ACT_OPEN, MUI_ACT_BACK, MUI_ACT_HOME, MUI_ACT_RECENTS,
       MUI_ACT_SHADE_OPEN, MUI_ACT_SHADE_CLOSE };

typedef struct { int id; int x, y, w, h; } mui_target_t;

typedef struct {
    int w, h, form, portrait;
    int status_h, nav_h;
    int cols, rows, cell_w, cell_h, icon;
    int ntargets;
    int min_target;              /* smallest target side, >= MUI_MIN_TARGET */
    mui_target_t t[MUI_MAX_TARGETS];
} mui_layout_t;

typedef struct {
    int screen;
    int app;                     /* open app id when screen == APP */
    int stack[MUI_STACK];        /* back stack of app ids */
    int depth;
    int shade;                   /* notification shade pulled down */
    uint32_t opens, backs;
} mui_state_t;

const char* mui_app_name(int id);
int mui_layout(int w, int h, mui_layout_t* out);
int mui_hit(const mui_layout_t* l, int x, int y);
void mui_init(mui_state_t* s);
int mui_tap(mui_state_t* s, const mui_layout_t* l, int x, int y);
int mui_gesture(mui_state_t* s, const mui_layout_t* l, const plat_point_t* p, int n);
const char* mui_action_name(int a);
uint32_t mui_pixel(const mui_layout_t* l, const mui_state_t* s, int x, int y);
uint32_t mui_frame_hash(const mui_layout_t* l, const mui_state_t* s);

#endif
