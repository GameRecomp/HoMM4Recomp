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
#include "rc/log.h"
#include "rc/mem.h"
#include "rc/platform.h"

#include "heroes4.h"

void heroes4_patches(void);

#define SCROLL_TIMER 0x004a3e05u /* after `push 50`, the timer's interval */
#define SCROLL_STEP 0x004a4201u  /* after the step (25) is stored at view + 0x2e4 */

static void scroll_timer(rc_cpu *c) { rc_st32(c->r[RC_ESP], 16); }

static void scroll_step(rc_cpu *c) { rc_st32(c->r[RC_ESI] + 0x2e4, 8); }

static bool yes(const char *v) { return !strcmp(v, "yes") || !strcmp(v, "1") || !strcmp(v, "true"); }

void heroes4_patches(void)
{
    heroes4_widescreen();
    if (yes(rc_plat_setting("smooth_scroll", "no",
                            "yes: the adventure map scrolls in small steps, about 60 a second, at the\n"
                            "same speed. no (the default): as the original, 20 steps a second."))) {
        rc_on_point(SCROLL_TIMER, scroll_timer);
        rc_on_point(SCROLL_STEP, scroll_step);
        rc_log("smooth scrolling");
    }
}
