/* SPDX-License-Identifier: MIT */
/*
 * Wide resolutions for Heroes IV, after the way of Verok's Resolution Mod, with the wide layouts made
 * from the game's own while it reads them instead of drawn for each size.
 *
 * The game knows three resolutions, 800x600, 1024x768 and 1280x1024 (its "classes" here), and has a
 * layout for each screen in each of them (layers.adventure.1024.h4d, town, combat, main menu...).
 * With `resolution` in settings.ini other than off, each class gets a wide size: the shape of the
 * size the settings ask for (the window's, by default), at the class's height. The game goes on
 * thinking in classes (its options page, the registry, its mode list) until it sets the display
 * mode (0x8d3750), where a class becomes its wide size. After that:
 *
 *   - the dozen places that choose a layout by the screen's width see the class's width;
 *   - the layouts are made as wide as the screen when the game reads them: the adventure screen's
 *     right part (frame, panel) moves to the right edge and the frame along the map grows (new
 *     columns where it meets the right frame, its pattern continued); the other
 *     screens (town, combat, main menu) move to the middle on a background as wide as the screen,
 *     its sides continued from its own picture (black where it has none);
 *   - what the game centres or places itself takes the real size: the 800x600 offset of the map
 *     view, the town's dialogs, the battlefield, the main menu's picture, the town's picture (at
 *     its layer's left edge); the town, which scales its picture by the screen's width, sees the
 *     class's.
 *
 * A layer in memory (0x68 bytes): its name (a string, text at +0x08, length at +0x0c), its type
 * (+0x14; 4 and more: with alpha), its rectangle (+0x18: left, top, right, bottom), and shared
 * buffers (+0x28 rows, +0x34 alpha, +0x40 alpha summary, +0x4c pixels; each {next, previous,
 * data}), its palette at +0x58 (3 bytes a colour, data at +0x5c). A row is {first column, end
 * column, offset of its first pixel}; pixels are palette indices, alpha 4 bits a pixel (low nibble
 * first), the summary 2 bits for each 32 pixels (0 transparent, 1 opaque, 2 both, 3 partly).
 */
#include <stdbool.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "rc/cpu.h"
#include "rc/dispatch.h"
#include "rc/hle.h"
#include "rc/hle_modules.h"
#include "rc/log.h"
#include "rc/mem.h"
#include "rc/platform.h"

#include "heroes4.h"

#define LAYER_SIZE 0x68u
#define OPERATOR_NEW 0x008d98a0u /* cdecl (size) */
#define BUFFER_SET 0x00596c50u   /* thiscall (data): takes the data, frees the old if unshared */

/* ---- the sizes ---------------------------------------------------------------------------- */

static const int32_t class_w[3] = {800, 1024, 1280}, class_h[3] = {600, 768, 1024};
static int32_t wide_w[3], wide_h[3];
static bool enabled;

/* The class of a display size, or -1. */
static int class_of(int32_t w, int32_t h)
{
    for (int i = 0; i < 3; i++) {
        if (w == wide_w[i] && h == wide_h[i]) {
            return i;
        }
    }
    return -1;
}

/* The current mode and its class, if it is a wide one. */
static bool wide_mode(int32_t *w, int32_t *h, int *k)
{
    uint32_t mw, mh;
    rc_ddraw_mode(&mw, &mh);
    int i = enabled ? class_of((int32_t)mw, (int32_t)mh) : -1;
    if (i < 0 || (wide_w[i] == class_w[i] && wide_h[i] == class_h[i])) {
        return false;
    }
    *w = (int32_t)mw;
    *h = (int32_t)mh;
    *k = i;
    return true;
}

/* ---- the code ----------------------------------------------------------------------------- */

/* The display mode is set (cdecl width, height, ...): a class becomes its wide size. */
static void set_mode(rc_cpu *c)
{
    static bool labelled;
    if (enabled && !labelled) {
        /* the options page's three choices (the program is loaded by now; at set-up it is not) */
        static const uint32_t labels[3] = {0x00a75930, 0x00a75918, 0x00a758f8};
        for (int i = 0; i < 3; i++) {
            char text[16];
            snprintf(text, sizeof text, "%d x %d", wide_w[i], wide_h[i]);
            if (strlen(text) < 12) {
                rc_put_str(labels[i], 12, text);
            }
        }
        labelled = true;
    }
    uint32_t sp = c->r[RC_ESP];
    int32_t w = (int32_t)rc_ld32(sp + 4), h = (int32_t)rc_ld32(sp + 8);
    for (int i = 0; enabled && i < 3; i++) {
        if (w == class_w[i] && h == class_h[i]) {
            rc_st32(sp + 4, (uint32_t)wide_w[i]);
            rc_st32(sp + 8, (uint32_t)wide_h[i]);
            rc_log("widescreen: %dx%d as %dx%d", w, h, wide_w[i], wide_h[i]);
            return;
        }
    }
}

/* Where a layout switch is about to compare the width, and where the width is. */
static const struct {
    uint32_t at;
    int reg;     /* register, or -1 */
    int esp_off; /* else the stack slot [esp + esp_off] */
} layout_switches[] = {
    {0x004a35a7, RC_EAX, 0},  /* adventure map view */
    {0x004a397e, RC_EAX, 0},  /* adventure map view */
    {0x004accf4, -1, 0xb8},   /* adventure map: resource bar (the argument, read again later) */
    {0x004b56f1, RC_EAX, 0},  /* adventure map (with 0x4b57cb in Verok's list) */
    {0x00584266, RC_EAX, 0},  /* class index 0, 1, 2 of a size (with its height) */
    {0x0062b8da, RC_EAX, 0},  /* combat: background and border */
    {0x0069fbca, RC_EAX, 0},  /* an animation */
    {0x0076da14, RC_EAX, 0},  /* main menu: its art */
    {0x0076e534, RC_EAX, 0},  /* main menu: its buttons */
    {0x00773e80, RC_EAX, 0},  /* credits */
    {0x008ac64a, RC_EAX, 0},  /* town */
    {0x008c8f8f, RC_EAX, 0},  /* a clock */
};

static void layout_switch(rc_cpu *c)
{
    int32_t w, h;
    int k;
    if (!wide_mode(&w, &h, &k)) {
        return;
    }
    for (size_t i = 0; i < sizeof layout_switches / sizeof layout_switches[0]; i++) {
        if (layout_switches[i].at == c->eip) {
            uint32_t a = c->r[RC_ESP] + (uint32_t)layout_switches[i].esp_off;
            uint32_t v = layout_switches[i].reg >= 0 ? c->r[layout_switches[i].reg] : rc_ld32(a);
            if ((int32_t)v != w) {
                return; /* a window's width, not the screen's */
            }
            if (layout_switches[i].reg >= 0) {
                c->r[layout_switches[i].reg] = (uint32_t)class_w[k];
            } else {
                rc_st32(a, (uint32_t)class_w[k]);
            }
            return;
        }
    }
}

/* After a resolution change on the options page the screen's window gets the class's size
 * ([ESP]: the rectangle, before 0x71eee0 at 0x773f59 from the main menu, 0x4b57c6 from the map):
 * the wide one. */
static void screen_window(rc_cpu *c)
{
    int32_t w, h;
    int k;
    uint32_t r = rc_ld32(c->r[RC_ESP]);
    if (wide_mode(&w, &h, &k) && rc_ld32(r) == 0 && rc_ld32(r + 4) == 0 &&
        (int32_t)rc_ld32(r + 8) == class_w[k] && (int32_t)rc_ld32(r + 12) == class_h[k]) {
        rc_st32(r + 8, (uint32_t)w);
        rc_st32(r + 12, (uint32_t)h);
    }
}

/* The adventure map view's offset for 800x600 things over the map (0x4a35ed: after its switch). */
static void map_view_offset(rc_cpu *c)
{
    int32_t w, h;
    int k;
    if (wide_mode(&w, &h, &k)) {
        rc_st32(c->r[RC_ESI] + 0xcc, (uint32_t)((w - 800) / 2));
        rc_st32(c->r[RC_ESI] + 0xd0, (uint32_t)((h - 600) / 2));
    }
}

/* The town asks for the screen's rectangle (after a call of 0x8d2e20; EAX: the rectangle) to
 * choose and scale its picture: the class's. */
static void town_screen(rc_cpu *c)
{
    int32_t w, h;
    int k;
    uint32_t r = c->r[RC_EAX];
    if (wide_mode(&w, &h, &k) && (int32_t)(rc_ld32(r + 8) - rc_ld32(r)) == w) {
        rc_st32(r + 8, rc_ld32(r) + (uint32_t)class_w[k]);
        rc_st32(r + 12, rc_ld32(r + 4) + (uint32_t)class_h[k]);
    }
}

/* The town's picture window is made at the left edge of the layout's town_image, not at 0
 * (0x8b6835, after `xor edx, edx`: EDX is the x argument; the town function keeps the layer's
 * left at [ESP + 0x2c] here). */
static void town_picture(rc_cpu *c)
{
    int32_t w, h;
    int k;
    if (wide_mode(&w, &h, &k)) {
        c->r[RC_EDX] = rc_ld32(c->r[RC_ESP] + 0x2c);
    }
}

/* A rectangle at [ESP + off] that spans the screen becomes an 800x600 one in its middle (a town
 * dialog, 0x75fe42). */
static void centre_800(rc_cpu *c)
{
    int32_t w, h;
    int k;
    if (!wide_mode(&w, &h, &k)) {
        return;
    }
    uint32_t r = c->r[RC_ESP] + 0x2c;
    int32_t l = (int32_t)rc_ld32(r), t = (int32_t)rc_ld32(r + 4);
    int32_t rw = (int32_t)rc_ld32(r + 8) - l, rh = (int32_t)rc_ld32(r + 12) - t;
    if (rw > 800) {
        rc_st32(r, (uint32_t)(l + (rw - 800) / 2));
        rc_st32(r + 8, (uint32_t)(l + (rw - 800) / 2 + 800));
    }
    if (rh > 600) {
        rc_st32(r + 4, (uint32_t)(t + (rh - 600) / 2));
        rc_st32(r + 12, (uint32_t)(t + (rh - 600) / 2 + 600));
    }
}

/* The main menu's picture: its rectangle ([ESP], before 0x71e980 at 0x599f62) in the middle. */
static void main_menu_picture(rc_cpu *c)
{
    int32_t w, h;
    int k;
    if (!wide_mode(&w, &h, &k)) {
        return;
    }
    uint32_t r = rc_ld32(c->r[RC_ESP]);
    int32_t pw = (int32_t)(rc_ld32(r + 8) - rc_ld32(r)), ph = (int32_t)(rc_ld32(r + 12) - rc_ld32(r + 4));
    if (pw > 0 && pw < w) {
        rc_st32(r, (uint32_t)((w - pw) / 2));
        rc_st32(r + 8, (uint32_t)((w - pw) / 2 + pw));
    }
    if (ph > 0 && ph < h) {
        rc_st32(r + 4, (uint32_t)((h - ph) / 2));
        rc_st32(r + 12, (uint32_t)((h - ph) / 2 + ph));
    }
}

/* The battlefield's offset in the combat screen ([ESP], [ESP+4] of the call of 0x5a6500). */
static void battlefield(rc_cpu *c)
{
    int32_t w, h;
    int k;
    if (wide_mode(&w, &h, &k)) {
        rc_st32(c->r[RC_ESP], (uint32_t)((w - class_w[k]) / 2));
        rc_st32(c->r[RC_ESP] + 4, (uint32_t)((h - class_h[k]) / 2));
    }
}

/* ---- the layouts -------------------------------------------------------------------------- */

typedef struct {
    uint32_t at; /* the layer in guest memory */
    char name[48];
    int typ;
    int32_t l, t, r, b;
} layer;

static uint32_t guest_new(rc_cpu *c, uint32_t size)
{
    uint32_t esp = c->r[RC_ESP];
    uint32_t p = rc_call_guest(c, OPERATOR_NEW, 1, &size);
    c->r[RC_ESP] = esp; /* cdecl: the caller removes the argument */
    return p;
}

static void buffer_set(rc_cpu *c, uint32_t buffer, uint32_t data)
{
    c->r[RC_ECX] = buffer;
    rc_call_guest(c, BUFFER_SET, 1, &data);
}

static void read_layer(uint32_t at, layer *L)
{
    L->at = at;
    uint32_t text = rc_ld32(at + 0x08), len = rc_ld32(at + 0x0c);
    if (!text) {
        len = 0;
    }
    if (len >= sizeof L->name) {
        len = sizeof L->name - 1;
    }
    memcpy(L->name, text ? rc_ptr(text) : "", len);
    L->name[len] = 0;
    L->typ = (int)rc_ld32(at + 0x14);
    L->l = (int32_t)rc_ld32(at + 0x18);
    L->t = (int32_t)rc_ld32(at + 0x1c);
    L->r = (int32_t)rc_ld32(at + 0x20);
    L->b = (int32_t)rc_ld32(at + 0x24);
}

static void set_rect(layer *L, int32_t l, int32_t t, int32_t r, int32_t b)
{
    rc_st32(L->at + 0x18, (uint32_t)l);
    rc_st32(L->at + 0x1c, (uint32_t)t);
    rc_st32(L->at + 0x20, (uint32_t)r);
    rc_st32(L->at + 0x24, (uint32_t)b);
}

static void move_layer(layer *L, int32_t dx) { set_rect(L, L->l + dx, L->t, L->r + dx, L->b); }

static int alpha_at(const uint8_t *a, uint32_t i) { return (a[i >> 1] >> (4 * (i & 1))) & 15; }

static void alpha_put(uint8_t *a, uint32_t i, int v)
{
    a[i >> 1] = (uint8_t)((a[i >> 1] & ~(15 << (4 * (i & 1)))) | (v << (4 * (i & 1))));
}

static void summarise(const uint8_t *a, uint32_t n, uint8_t *s)
{
    memset(s, 0, (n + 0x83) >> 7);
    for (uint32_t k = 0; k * 32 < n; k++) {
        bool opaque = true, clear = true, partial = false;
        for (uint32_t i = k * 32; i < n && i < k * 32 + 32; i++) {
            int v = alpha_at(a, i);
            opaque &= v == 15;
            clear &= v == 0;
            partial |= v > 0 && v < 15;
        }
        int code = partial ? 3 : opaque ? 1 : clear ? 0 : 2;
        s[k >> 2] |= (uint8_t)(code << (2 * (k & 3)));
    }
}

typedef struct {
    uint32_t x0, x1, off;
} row;

static row row_at(const uint8_t *rows, uint32_t y)
{
    row r;
    r.x0 = (uint32_t)rows[8 * y] | (uint32_t)rows[8 * y + 1] << 8;
    r.x1 = (uint32_t)rows[8 * y + 2] | (uint32_t)rows[8 * y + 3] << 8;
    memcpy(&r.off, rows + 8 * y + 4, 4);
    return r;
}

static void row_put(uint8_t *rows, uint32_t y, uint32_t x0, uint32_t x1, uint32_t off)
{
    rows[8 * y] = (uint8_t)x0;
    rows[8 * y + 1] = (uint8_t)(x0 >> 8);
    rows[8 * y + 2] = (uint8_t)x1;
    rows[8 * y + 3] = (uint8_t)(x1 >> 8);
    memcpy(rows + 8 * y + 4, &off, 4);
}

/* A layer's new pixels: rows, pixels, alpha (if any) built by the caller in host memory, copied into
 * guest buffers and given to the layer with its new rectangle. */
typedef struct {
    uint32_t h, n;
    uint8_t *rows, *pix, *alpha;
} pixels;

static bool pixels_new(pixels *p, uint32_t h, uint32_t n, bool alpha)
{
    p->h = h;
    p->n = n;
    p->rows = calloc(h ? h : 1, 8);
    p->pix = calloc(n ? n : 1, 1);
    p->alpha = alpha ? calloc(n / 2 + 1, 1) : NULL;
    return p->rows && p->pix && (!alpha || p->alpha);
}

static void pixels_free(pixels *p)
{
    free(p->rows);
    free(p->pix);
    free(p->alpha);
}

static uint32_t guest_copy(rc_cpu *c, const void *src, uint32_t n)
{
    uint32_t g = guest_new(c, n ? n : 1);
    memcpy(rc_ptr(g), src, n);
    return g;
}

static void pixels_give(rc_cpu *c, layer *L, const pixels *p)
{
    buffer_set(c, L->at + 0x28, guest_copy(c, p->rows, 8 * p->h));
    buffer_set(c, L->at + 0x4c, guest_copy(c, p->pix, p->n));
    if (p->alpha) {
        uint32_t ns = ((p->n + 0x83) >> 7) + 1;
        uint8_t *s = calloc(ns, 1);
        if (s) {
            summarise(p->alpha, p->n, s);
            buffer_set(c, L->at + 0x34, guest_copy(c, p->alpha, p->n / 2 + 1));
            buffer_set(c, L->at + 0x40, guest_copy(c, s, ns));
            free(s);
        }
    }
}

/* A reflection of `v` into [0, n): 0 1 2 .. n-1 n-1 .. 1 0 0 1 .. */
static int32_t mirror(int32_t v, int32_t n)
{
    if (n <= 1) {
        return 0;
    }
    int32_t m = v % (2 * n);
    if (m < 0) {
        m += 2 * n;
    }
    return m < n ? m : 2 * n - 1 - m;
}

enum { PATTERN, MIRROR_AFTER, MIRROR_BEFORE };

/* The period (1 to 96 columns) of the columns just left of `s`: the shift with which the 64 columns
 * before it best repeat themselves in the rows that span them. */
static uint32_t period_before(const uint8_t *rows, const uint8_t *pix, uint32_t h, uint32_t s)
{
    uint32_t best = 1, best_bad = UINT32_MAX;
    for (uint32_t p = 1; p <= 96 && p + 64 <= s; p++) {
        uint32_t bad = 0;
        for (uint32_t y = 0; y < h; y++) {
            row r = row_at(rows, y);
            if (r.x0 + p + 64 > s || r.x1 < s) {
                continue;
            }
            for (uint32_t x = s - 64; x < s; x++) {
                bad += pix[r.off + x - r.x0] != pix[r.off + x - p - r.x0];
            }
        }
        if (bad < best_bad) {
            best_bad = bad;
            best = p;
        }
    }
    return best;
}

/* `d` new columns at column `s` of the layer (0 to its width): the rows that reach `s` get them,
 * the pattern before it continued (PATTERN), or the picture after it (MIRROR_AFTER) or before it
 * (MIRROR_BEFORE) reflected, at most `zone_max` columns of it (0: half of the row's); the rows
 * past it move. The rectangle grows by `d` to the right. */
static void insert_columns(rc_cpu *c, layer *L, uint32_t s, uint32_t d, int mode, int32_t zone_max)
{
    uint32_t w = (uint32_t)(L->r - L->l), h = (uint32_t)(L->b - L->t);
    if (!h || !w || !d) {
        if (d) {
            set_rect(L, L->l, L->t, L->r + (int32_t)d, L->b);
        }
        return;
    }
    const uint8_t *rows = rc_ptr(rc_ld32(L->at + 0x30));
    const uint8_t *pix = rc_ptr(rc_ld32(L->at + 0x54));
    uint32_t ga = L->typ >= 4 ? rc_ld32(L->at + 0x3c) : 0;
    const uint8_t *alpha = ga ? rc_ptr(ga) : NULL;
    uint32_t p = mode == PATTERN ? period_before(rows, pix, h, s) : 1, n = 0;
    for (uint32_t y = 0; y < h; y++) {
        row r = row_at(rows, y);
        bool spans = r.x0 <= s && s <= r.x1 && r.x1 > r.x0;
        n += r.x1 - r.x0 + (spans ? d : 0);
    }
    pixels out;
    if (!pixels_new(&out, h, n, alpha != NULL)) {
        pixels_free(&out);
        return;
    }
    uint32_t o = 0;
    for (uint32_t y = 0; y < h; y++) {
        row r = row_at(rows, y);
        uint32_t len = r.x1 - r.x0;
        bool spans = r.x0 <= s && s <= r.x1 && len > 0;
        uint32_t x0 = r.x0 > s || (!spans && r.x0 >= s) ? r.x0 + d : r.x0;
        uint32_t nl = len + (spans ? d : 0);
        for (uint32_t i = 0; i < nl; i++) {
            uint32_t x = r.x0 + i, src; /* x: the new column, from the row's start */
            if (!spans || x < s) {
                src = i;
            } else if (x >= s + d) {
                src = i - d;
            } else {
                uint32_t j = x - s; /* 0 .. d-1 */
                int32_t col;
                if (mode == PATTERN) {
                    col = (int32_t)(s >= p ? s - p + j % p : s - 1);
                } else if (mode == MIRROR_AFTER) {
                    /* from the half of the picture next to s only: what lies further (a frame
                     * around something else) stays out */
                    int32_t zone = (int32_t)(r.x1 - s) / 2;
                    zone = zone_max > 0 && zone > zone_max ? zone_max : zone;
                    col = (int32_t)s + mirror((int32_t)(d - 1 - j), zone > 0 ? zone : 1);
                } else {
                    int32_t zone = (int32_t)(s - r.x0) / 2;
                    zone = zone_max > 0 && zone > zone_max ? zone_max : zone;
                    col = (int32_t)s - 1 - mirror((int32_t)j, zone > 0 ? zone : 1);
                }
                if (col < (int32_t)r.x0) {
                    col = (int32_t)r.x0;
                }
                if (col >= (int32_t)r.x1) {
                    col = (int32_t)r.x1 - 1;
                }
                src = (uint32_t)col - r.x0;
            }
            out.pix[o + i] = pix[r.off + src];
            if (alpha) {
                alpha_put(out.alpha, o + i, alpha_at(alpha, r.off + src));
            }
        }
        row_put(out.rows, y, x0, x0 + nl, o);
        o += nl;
    }
    pixels_give(c, L, &out);
    pixels_free(&out);
    set_rect(L, L->l, L->t, L->r + (int32_t)d, L->b);
}

/* The darkest colour of a layer's palette. */
static uint8_t darkest(const layer *L)
{
    uint32_t pal = rc_ld32(L->at + 0x5c), end = rc_ld32(L->at + 0x60);
    uint32_t n = pal && end > pal ? (end - pal) / 3 : 0;
    const uint8_t *p = pal ? rc_ptr(pal) : NULL;
    uint32_t best = 0, best_sum = UINT32_MAX;
    for (uint32_t i = 0; i < n; i++) {
        uint32_t s = (uint32_t)p[3 * i] + p[3 * i + 1] + p[3 * i + 2];
        if (s < best_sum) {
            best_sum = s;
            best = i;
        }
    }
    return (uint8_t)best;
}

/* Palette index 0 free for "transparent" (type 1 layers): a colour the pixels use there moves to an
 * index they do not use. False if every index is in use. */
static bool free_index_0(layer *L, uint8_t *pix, uint32_t n)
{
    uint32_t pal = rc_ld32(L->at + 0x5c), end = rc_ld32(L->at + 0x60);
    uint32_t ncol = pal && end > pal ? (end - pal) / 3 : 0;
    bool used[256] = {false};
    for (uint32_t i = 0; i < n; i++) {
        used[pix[i]] = true;
    }
    if (!used[0]) {
        return true;
    }
    for (uint32_t u = 1; u < ncol && u < 256; u++) {
        if (!used[u]) {
            uint8_t *p = rc_ptr(pal);
            memcpy(p + 3 * u, p, 3);
            for (uint32_t i = 0; i < n; i++) {
                if (!pix[i]) {
                    pix[i] = (uint8_t)u;
                }
            }
            return true;
        }
    }
    return false;
}

/* The calmest part of a layer's picture: the `pw` x `ph` rectangle of its full rows with the least
 * change of brightness from pixel to pixel (no frame, scroll or box in it), in `px`, `py`. */
static bool calm_patch(const layer *L, const uint8_t *rows, const uint8_t *pix, int32_t pw, int32_t ph, int32_t *px,
                       int32_t *py)
{
    int32_t ow = L->r - L->l, oh = L->b - L->t;
    uint32_t pal = rc_ld32(L->at + 0x5c);
    if (!pal || pw > ow || ph > oh) {
        return false;
    }
    const uint8_t *p = rc_ptr(pal);
    /* brightness, -1 where a row does not cover the column */
    int32_t *lum = malloc(sizeof *lum * (size_t)ow * (size_t)oh);
    double *sum = calloc((size_t)(ow + 1) * (size_t)(oh + 1), sizeof *sum);
    double *bright = calloc((size_t)(ow + 1) * (size_t)(oh + 1), sizeof *bright);
    if (!lum || !sum || !bright) {
        free(lum);
        free(sum);
        free(bright);
        return false;
    }
    for (int32_t y = 0; y < oh; y++) {
        row r = row_at(rows, (uint32_t)y);
        for (int32_t x = 0; x < ow; x++) {
            if ((uint32_t)x < r.x0 || (uint32_t)x >= r.x1) {
                lum[y * ow + x] = -1;
            } else {
                const uint8_t *c3 = p + 3 * pix[r.off + (uint32_t)x - r.x0];
                lum[y * ow + x] = c3[0] + c3[1] + c3[2];
            }
        }
    }
    /* the change at each pixel (a missing pixel counts much), summed up from the top left */
    for (int32_t y = 0; y < oh; y++) {
        for (int32_t x = 0; x < ow; x++) {
            int32_t v = lum[y * ow + x], g;
            if (v < 0) {
                g = 100000;
            } else {
                int32_t right = x + 1 < ow ? lum[y * ow + x + 1] : v, down = y + 1 < oh ? lum[(y + 1) * ow + x] : v;
                g = (right < 0 ? 0 : abs(v - right)) + (down < 0 ? 0 : abs(v - down));
                g = g > 90 ? 50 * g : g; /* a line (a frame, a scroll's rod) counts far more than grain */
            }
            sum[(y + 1) * (ow + 1) + x + 1] = g + sum[y * (ow + 1) + x + 1] + sum[(y + 1) * (ow + 1) + x] - sum[y * (ow + 1) + x];
            bright[(y + 1) * (ow + 1) + x + 1] = (v < 0 ? 0 : v) + bright[y * (ow + 1) + x + 1] +
                                                 bright[(y + 1) * (ow + 1) + x] - bright[y * (ow + 1) + x];
        }
    }
    /* as bright as the picture is on the whole: not a scroll's light inside */
    double mean = bright[oh * (ow + 1) + ow] / ((double)ow * oh), area = (double)pw * ph, best = -1;
    for (int32_t y = 0; y + ph <= oh; y += 4) {
        for (int32_t x = 0; x + pw <= ow; x += 4) {
#define BOX(t) (t[(y + ph) * (ow + 1) + x + pw] - t[y * (ow + 1) + x + pw] - t[(y + ph) * (ow + 1) + x] + t[y * (ow + 1) + x])
            double v = BOX(sum) + 0.25 * fabs(BOX(bright) / area - mean) * area;
#undef BOX
            if (best < 0 || v < best) {
                best = v;
                *px = x;
                *py = y;
            }
        }
    }
    free(lum);
    free(sum);
    free(bright);
    return best >= 0;
}

/* The background as large as the screen (0, 0, w, h): the old one moved by `dx`; the screen's
 * middle (the class's `cw` x `ch` there) left transparent where the old one did not reach, for the
 * picture the game draws there (the battlefield, the town); the sides the calmest part of its own
 * picture, reflected to and fro, with a dark line along the middle (or its darkest colour where it
 * has hardly any picture). */
static void widen_background(rc_cpu *c, layer *L, int32_t dx, int32_t w, int32_t h, int32_t cw, int32_t ch)
{
    int32_t ow = L->r - L->l, oh = L->b - L->t;
    const uint8_t *rows = oh > 0 ? rc_ptr(rc_ld32(L->at + 0x30)) : NULL;
    const uint8_t *pix = oh > 0 ? rc_ptr(rc_ld32(L->at + 0x54)) : NULL;
    uint32_t ga = L->typ >= 4 && oh > 0 ? rc_ld32(L->at + 0x3c) : 0;
    const uint8_t *alpha = ga ? rc_ptr(ga) : NULL;
    /* A picture to continue: the rows that cover the whole width. */
    int32_t full = 0;
    for (int32_t y = 0; rows && y < oh; y++) {
        row r = row_at(rows, (uint32_t)y);
        full += r.x0 == 0 && (int32_t)r.x1 == ow;
    }
    bool picture = ow > 8 && full * 2 > oh;
    uint8_t dark = darkest(L);
    int32_t pw = ow / 2 < 96 ? ow / 2 : 96, ph = oh / 2 < 128 ? oh / 2 : 128, px = 0, py = 0;
    picture = picture && pw >= 8 && ph >= 8 && calm_patch(L, rows, pix, pw, ph, &px, &py);
    pixels out;
    if (!pixels_new(&out, (uint32_t)h, (uint32_t)(w * h), alpha != NULL)) {
        pixels_free(&out);
        return;
    }
    for (int32_t y = 0; y < h; y++) {
        for (int32_t x = 0; x < w; x++) {
            uint32_t i = (uint32_t)(y * w + x);
            int32_t sx = x - dx - L->l, sy = y - L->t;
            bool middle = x >= dx && x < dx + cw && y < ch;
            bool inside = sx >= 0 && sx < ow && sy >= 0 && sy < oh;
            if (middle && !inside) {
                out.pix[i] = 0; /* transparent */
                if (out.alpha) {
                    alpha_put(out.alpha, i, 0);
                }
                continue;
            }
            bool edge = (x >= dx - 3 && x < dx) || (x >= dx + cw && x < dx + cw + 3);
            if (!picture || (edge && !inside)) {
                out.pix[i] = dark;
                if (out.alpha) {
                    alpha_put(out.alpha, i, 15);
                }
                continue;
            }
            if (!inside) {
                sx = px + mirror(x, pw);
                sy = py + mirror(y, ph);
            }
            row r = row_at(rows, (uint32_t)sy);
            uint32_t col = (uint32_t)sx < r.x0 ? r.x0 : (uint32_t)sx >= r.x1 ? r.x1 - 1 : (uint32_t)sx;
            uint32_t from = r.x1 > r.x0 ? r.off + col - r.x0 : r.off;
            out.pix[i] = pix[from];
            if (out.alpha) {
                alpha_put(out.alpha, i, r.x1 > r.x0 ? alpha_at(alpha, from) : 0);
            }
        }
        row_put(out.rows, (uint32_t)y, 0, (uint32_t)w, (uint32_t)(y * w));
    }
    if (!out.alpha) {
        /* the side's pixels must not be index 0, which is transparent now: */
        uint8_t *mid = calloc((size_t)w * (size_t)h, 1);
        if (mid) {
            for (int32_t y = 0; y < h; y++) {
                for (int32_t x = 0; x < w; x++) {
                    int32_t sx = x - dx - L->l, sy = y - L->t;
                    mid[y * w + x] = x >= dx && x < dx + cw && y < ch && !(sx >= 0 && sx < ow && sy >= 0 && sy < oh);
                }
            }
            /* move index 0 of the pixels that stay visible to a free index */
            uint8_t *vis = malloc((size_t)w * (size_t)h);
            if (vis) {
                uint32_t nv = 0;
                for (uint32_t i = 0; i < (uint32_t)(w * h); i++) {
                    if (!mid[i]) {
                        vis[nv++] = out.pix[i];
                    }
                }
                if (free_index_0(L, vis, nv)) {
                    nv = 0;
                    for (uint32_t i = 0; i < (uint32_t)(w * h); i++) {
                        if (!mid[i]) {
                            out.pix[i] = vis[nv++];
                        }
                    }
                }
                free(vis);
            }
            free(mid);
        }
        rc_st32(L->at + 0x14, 1); /* type 1: index 0 is transparent */
    }
    pixels_give(c, L, &out);
    pixels_free(&out);
    set_rect(L, 0, 0, w, h);
}

/* Whether a layer is a picture (most of its rows cover its width), not a marker for a place (a
 * few pixels a row: where the game draws a list, the battlefield, a film). */
static bool is_picture(const layer *L)
{
    uint32_t w = (uint32_t)(L->r - L->l), h = (uint32_t)(L->b - L->t), full = 0;
    if (!w || !h) {
        return false;
    }
    const uint8_t *rows = rc_ptr(rc_ld32(L->at + 0x30));
    for (uint32_t y = 0; y < h; y++) {
        row r = row_at(rows, y);
        full += r.x1 - r.x0 >= w - w / 8;
    }
    return full * 2 > h;
}

static layer *find(layer *ls, int n, const char *name)
{
    for (int i = 0; i < n; i++) {
        if (!strcmp(ls[i].name, name)) {
            return &ls[i];
        }
    }
    return NULL;
}

/* The point after the layers of a layout are read: the layout's layers are [esi+4, esi+8). */
static void layout_read(rc_cpu *c)
{
    int32_t mw, mh;
    int k;
    if (!wide_mode(&mw, &mh, &k)) {
        return;
    }
    uint32_t group = c->r[RC_ESI];
    uint32_t first = rc_ld32(group + 4), end = rc_ld32(group + 8);
    if (!first || end <= first || (end - first) % LAYER_SIZE) {
        return;
    }
    int n = (int)((end - first) / LAYER_SIZE);
    layer *ls = calloc((size_t)n, sizeof *ls);
    if (!ls) {
        return;
    }
    int32_t cw = 0, ch = 0;
    for (int i = 0; i < n; i++) {
        read_layer(first + (uint32_t)i * LAYER_SIZE, &ls[i]);
        cw = ls[i].r > cw ? ls[i].r : cw;
        ch = ls[i].b > ch ? ls[i].b : ch;
    }
    int32_t dx = mw - cw;
    layer *map = find(ls, n, "Adventure_Window");
    if (cw == 800 && ch == 600 && map && find(ls, n, "Mini_Map") && find(ls, n, "Close_Button")) {
        /* the world view (an 800x600 dialog in every class): as much wider as the screen, less the
         * class's margins, in the middle of its map */
        int32_t dw = mw - (class_w[k] - 800);
        dw = dw > 2200 ? 2200 : dw;
        int32_t d = dw - 800, split = (map->l + map->r) / 2;
        if (d > 0) {
            uint32_t keep[8];
            memcpy(keep, c->r, sizeof keep);
            for (int i = 0; i < n; i++) {
                if (ls[i].l >= split) {
                    move_layer(&ls[i], d);
                } else if (ls[i].r > split) {
                    insert_columns(c, &ls[i], (uint32_t)(split - ls[i].l), (uint32_t)d, PATTERN, 0);
                }
            }
            memcpy(c->r, keep, sizeof keep);
            rc_log("widescreen: world view 800x600 to %dx600", dw);
        }
        free(ls);
        return;
    }
    if (cw != class_w[k] || ch != class_h[k] || dx <= 0) {
        free(ls); /* not a screen of this class (a dialog, a button) */
        return;
    }
    /* The calls below clobber registers the game still needs after the point. */
    uint32_t keep[8];
    memcpy(keep, c->r, sizeof keep);
    layer *frame = find(ls, n, "Right_Border");
    if (frame && find(ls, n, "mini_map")) {
        /* the adventure screen: the right part to the right edge, the strips along the map wider */
        int32_t split = frame->l;
        for (int i = 0; i < n; i++) {
            if (ls[i].l >= split) {
                move_layer(&ls[i], dx);
            } else if (ls[i].r >= split) {
                /* to the frame or across it (the 1280 layout's top runs on over it): the new
                 * columns at the frame, the part over it moving with it */
                insert_columns(c, &ls[i], (uint32_t)(split - ls[i].l), (uint32_t)dx, PATTERN, 0);
            }
        }
        rc_log("widescreen: adventure screen %dx%d to %dx%d", cw, ch, mw, mh);
    } else {
        /* every other screen of the class (town, combat, main menu, the intro's and a scenario's
         * pictures): in the middle; its background as large as the screen, the pieces at the
         * screen's left or right edge reaching on to it */
        int32_t half = dx / 2;
        /* what lies in the middle (neither the background nor at an edge): the new columns of the
         * pieces at the edges go in halfway between it and the edge */
        int32_t inner_l = cw / 2, inner_r = cw / 2;
        for (int i = 0; i < n; i++) {
            if (strcmp(ls[i].name, "Background") && ls[i].l > 0 && ls[i].r < cw && ls[i].r > ls[i].l) {
                inner_l = ls[i].l < inner_l ? ls[i].l : inner_l;
                inner_r = ls[i].r > inner_r ? ls[i].r : inner_r;
            }
        }
        const char *what = find(ls, n, "town_image") ? "town" : find(ls, n, "battle_scene") ? "combat"
                         : find(ls, n, "New_Game_Released") ? "main menu" : "picture";
        for (int i = 0; i < n; i++) {
            layer *L = &ls[i];
            if (!strcmp(L->name, "Background")) {
                widen_background(c, L, half, mw, mh, cw, ch);
            } else if (!strcmp(L->name, "town_image")) {
                set_rect(L, half, L->t, half + cw, L->b); /* the town's picture is as wide as the class */
            } else if ((L->l <= 0 || L->r >= cw) && is_picture(L)) {
                /* the new columns go in between the edge and the middle's content (past a frame
                 * along the edge, before what it frames), the picture there reflected */
                bool left = L->l <= 0, right = L->r >= cw;
                int32_t at_l = inner_l / 2, at_r = (inner_r + cw) / 2; /* in the class's columns */
                if (!left) {
                    move_layer(L, half);
                    read_layer(L->at, L);
                }
                if (left) {
                    int32_t sl = at_l - L->l;
                    sl = sl < 1 ? 1 : sl >= L->r - L->l ? L->r - L->l - 1 : sl;
                    insert_columns(c, L, (uint32_t)sl, (uint32_t)half, MIRROR_AFTER, (inner_l - at_l) * 2 / 3);
                    read_layer(L->at, L);
                }
                if (right) {
                    /* the class's column at_r: past the moved piece's old left edge, or past
                     * the `half` columns the left edge put in */
                    int32_t sr = left ? at_r + half : at_r - (L->l - half);
                    sr = sr < 1 ? 1 : sr >= L->r - L->l ? L->r - L->l - 1 : sr;
                    insert_columns(c, L, (uint32_t)sr, (uint32_t)(mw - L->r), MIRROR_BEFORE, (at_r - inner_r) * 2 / 3);
                }
            } else {
                move_layer(L, half);
            }
        }
        rc_log("widescreen: %s screen %dx%d in the middle of %dx%d", what, cw, ch, mw, mh);
    }
    memcpy(c->r, keep, sizeof keep);
    free(ls);
}

/* ---- set-up ------------------------------------------------------------------------------- */

void heroes4_widescreen(void)
{
    int w, h;
    if (!rc_plat_game_size(&w, &h) || w * 3 <= h * 4) {
        return; /* "off", or no wider than 4:3 */
    }
    for (int i = 0; i < 3; i++) {
        int32_t ww = class_h[i] == h ? w : (int32_t)((int64_t)class_h[i] * w / h) & ~7;
        wide_w[i] = ww > class_w[i] ? ww : class_w[i];
        wide_h[i] = class_h[i];
    }
    enabled = true;
    rc_on_point(0x008d3750, set_mode);
    for (size_t i = 0; i < sizeof layout_switches / sizeof layout_switches[0]; i++) {
        rc_on_point(layout_switches[i].at, layout_switch);
    }
    rc_on_point(0x004a35ed, map_view_offset);
    rc_on_point(0x00773f59, screen_window);
    rc_on_point(0x004b57c6, screen_window);
    static const uint32_t town_rects[] = {0x008b40c0, 0x008b43c0, 0x008b61e9, 0x008b66fa, 0x008b6a40};
    for (size_t i = 0; i < sizeof town_rects / sizeof town_rects[0]; i++) {
        rc_on_point(town_rects[i], town_screen);
    }
    rc_on_point(0x008b6835, town_picture);
    rc_on_point(0x0075fe42, centre_800);
    rc_on_point(0x00599f62, main_menu_picture);
    rc_on_point(0x0062c608, battlefield);
    rc_on_point(0x0062ae28, battlefield);
    rc_on_point(0x0058e467, layout_read);
    rc_log("widescreen: %dx%d, %dx%d, %dx%d", wide_w[0], wide_h[0], wide_w[1], wide_h[1], wide_w[2], wide_h[2]);
}
