/* SPDX-License-Identifier: MIT */
/*
 * The addresses of the patches in each build of Heroes4.exe the manifest lifts.
 *
 * GOG 25023 (gog-de) and GOG 76223 (gog-de-76223 and, with the same code, the English, French,
 * Italian and Spanish builds) share their addresses: 76223 only adds a cave in padding, reached from
 * the start of the display mode's function. The Polish 76223 build is compiled on its own: the same
 * code at other addresses (found by the instructions around each place; only the mage guild's
 * comparison is arranged differently, see widescreen.c) and one stack slot elsewhere.
 */
#include <string.h>

#include "heroes4.h"

static const h4_build builds[] = {
    {
        .variant = "gog-de",
        .scroll_timer = 0x004a3e05,
        .scroll_step = 0x004a4201,
        .release_call = 0x008d1c95,
        .nothing_ret4 = 0x0040f610,
        .map_vtable = 0x00979bc0,
        .operator_new = 0x008d98a0,
        .buffer_set = 0x00596c50,
        .set_mode = 0x008d3750,
        .mode_labels = {0x00a75930, 0x00a75918, 0x00a758f8},
        .layout_switch = {0x004a35a7, 0x004a397e, 0x004accf4, 0x004b56f1, 0x00584266, 0x0062b8da, 0x0069fbca,
                          0x0076da14, 0x0076e534, 0x00773e80, 0x008ac64a, 0x008c8f8f},
        .resource_bar_esp = 0xb8,
        .map_view_offset = 0x004a35ed,
        .guild_compare = 0x0075fa98,
        .guild_compared = 0x0075faa4,
        .guild_small = 0x0075fe2a,
        .centre_800 = 0x0075fe42,
        .screen_window = {0x00773f59, 0x004b57c6},
        .town_screen = {0x008b40c0, 0x008b43c0, 0x008b61e9, 0x008b66fa, 0x008b6a40},
        .town_picture = 0x008b6835,
        .main_menu_picture = 0x00599f62,
        .battlefield = {0x0062c608, 0x0062ae28},
        .layout_read = 0x0058e467,
    },
    {
        .variant = "gog-pl-76223",
        .scroll_timer = 0x004a5635,
        .scroll_step = 0x004a5a31,
        .release_call = 0x008d737c,
        .nothing_ret4 = 0x0088aa30,
        .map_vtable = 0x00983bb0,
        .operator_new = 0x008defb0,
        .buffer_set = 0x00598160,
        .set_mode = 0x008d8e60,
        .mode_labels = {0x00a85920, 0x00a85908, 0x00a858e8},
        .layout_switch = {0x004a4dc9, 0x004a51ae, 0x004ae4c2, 0x004b6dc1, 0x005857c6, 0x0062dd96, 0x006a20ba,
                          0x007709a0, 0x007714a4, 0x00776ce0, 0x008b1f9a, 0x008ce5af},
        .resource_bar_esp = 0xc0,
        .map_view_offset = 0x004a4e0f,
        .guild_compare = 0x007629e1,
        .guild_compared = 0x007629ff, /* after its second comparison, which picks 1024 or 1280 */
        .guild_small = 0x00762d86,
        .centre_800 = 0x00762d96,
        .screen_window = {0x00776db9, 0x004b6e96},
        .town_screen = {0x008b97b0, 0x008b9ab0, 0x008bb899, 0x008bbda8, 0x008bc0f0},
        .town_picture = 0x008bbee3,
        .main_menu_picture = 0x0059b485,
        .battlefield = {0x0062eafb, 0x0062d298},
        .layout_read = 0x0058fac7,
    },
};

const h4_build *h4;

const h4_build *heroes4_find_build(const char *variant)
{
    if (!variant) {
        return NULL;
    }
    if (!strcmp(variant, "gog-de-76223")) {
        variant = "gog-de"; /* the same addresses */
    }
    for (size_t i = 0; i < sizeof builds / sizeof builds[0]; i++) {
        if (!strcmp(builds[i].variant, variant)) {
            return &builds[i];
        }
    }
    return NULL;
}
