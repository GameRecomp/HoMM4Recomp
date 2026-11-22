/* SPDX-License-Identifier: MIT */
/*
 * The patches of Heroes IV, with the addresses for the lifter under [binary.patch] in game.toml.
 *
 * Smooth scrolling (opt-in, settings.ini `smooth_scroll`): the adventure map's view sets up its
 * scroll timer with 50 ms and a step of 25 pixels a tick, 20 jerky steps a second. With the
 * setting on, the timer runs every 16 ms with steps of 8 pixels: about the same speed in some 60
 * steps a second (the values the HeroesGL wrapper uses). Both are set in the view's constructor:
 * the interval is pushed as the timer's argument, the step stored into the view.
 *
 * Wide resolutions: widescreen.c.
 */
#include <stdbool.h>
#include <string.h>

#include "rc/cpu.h"
#include "rc/dispatch.h"
#include "rc/hle_modules.h"
#include "rc/vm.h"
#include "rc/log.h"
#include "rc/mem.h"
#include "rc/platform.h"

#include "heroes4.h"

void heroes4_patches(void);

#define SCROLL_TIMER 0x004a3e05u /* after `push 50`, the timer's interval */
#define SCROLL_STEP 0x004a4201u  /* after the step (25) is stored at view + 0x2e4 */

static void scroll_timer(rc_cpu *c) { rc_st32(c->r[RC_ESP], 16); }

static void scroll_step(rc_cpu *c) { rc_st32(c->r[RC_ESI] + 0x2e4, 8); }

/* A drag with the left button is not a click: the game sends the release, wherever it is, to the
 * window under the pointer as a click, and the adventure map then sets the hero's path to that
 * place. On the map, the release after the pointer moved more than a few pixels with the button
 * down goes to a handler that does nothing instead (EDX: the vtable of the call of [vtable + 0x28] at 0x8d1c95;
 * the game's state for the release, such as its capture, is kept). */
#define RELEASE_CALL 0x008d1c95u
#define NOTHING_RET4 0x0040f610u /* ret 4 */
#define MAP_VTABLE 0x00979bc0u   /* the adventure map's window (sliders and scroll bars need their release) */
static int down_x, down_y;
static bool held, dragged, release_dragged;

static bool watch_drag(const rc_event *e)
{
    if (e->type == RC_EV_MOUSE_BUTTON && e->button == 1) {
        if (e->down) {
            held = true;
            dragged = false;
            down_x = e->x;
            down_y = e->y;
        } else {
            release_dragged = held && dragged;
            held = false;
        }
    } else if (e->type == RC_EV_MOUSE_MOVE && held) {
        int dx = e->x - down_x, dy = e->y - down_y;
        dragged |= dx * dx + dy * dy > 8 * 8;
    }
    return false; /* everything goes on to the game */
}

static void release(rc_cpu *c)
{
    static uint32_t nothing; /* a vtable whose +0x28 does nothing */
    if (release_dragged && rc_ld32(c->r[RC_ESI]) == MAP_VTABLE) {
        if (!nothing) {
            nothing = rc_heap_alloc(0x40, true);
            rc_st32(nothing + 0x28, NOTHING_RET4);
        }
        c->r[RC_EDX] = nothing;
    }
    release_dragged = false;
}

static bool yes(const char *v) { return !strcmp(v, "yes") || !strcmp(v, "1") || !strcmp(v, "true"); }

void heroes4_patches(void)
{
    heroes4_widescreen();
    rc_set_input_filter(watch_drag);
    rc_wnd_own_pointer(true); /* the game draws its pointer; no system arrow over it */
    rc_on_point(RELEASE_CALL, release);
    if (yes(rc_plat_setting("smooth_scroll", "no",
                            "yes: the adventure map scrolls in small steps, about 60 a second, at the\n"
                            "same speed. no (the default): as the original, 20 steps a second."))) {
        rc_on_point(SCROLL_TIMER, scroll_timer);
        rc_on_point(SCROLL_STEP, scroll_step);
        rc_log("smooth scrolling");
    }
}
