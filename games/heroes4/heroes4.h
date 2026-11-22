/* SPDX-License-Identifier: MIT */
/* The patches of Heroes IV (patches.c) and the parts they are made of. */
#ifndef HEROES4_H
#define HEROES4_H

#include <stdint.h>

/* The addresses the patches use in one build of Heroes4.exe (builds.c); the patch points among them
 * are listed for the lifter under the build's [binary.patch] in game.toml. */
typedef struct h4_build {
    const char *variant; /* the [[binary]]'s first variant */
    /* patches.c */
    uint32_t scroll_timer, scroll_step; /* smooth scrolling */
    uint32_t release_call;              /* a left button release is handed to its window */
    uint32_t nothing_ret4;              /* a function that only returns (ret 4) */
    uint32_t map_vtable;                /* the adventure map window's vtable */
    /* widescreen.c */
    uint32_t operator_new, buffer_set;
    uint32_t set_mode;             /* the display mode is set */
    uint32_t mode_labels[3];       /* the options page's "800 x 600", "1024 x 768", "1280 x 1024" */
    uint32_t layout_switch[12];    /* see widescreen.c's layout_switches, in that order */
    int32_t resource_bar_esp;      /* the stack slot of layout_switch[2]'s width */
    uint32_t map_view_offset;
    uint32_t guild_compare;        /* the mage guild compares the screen's width */
    uint32_t guild_compared;       /* after it, the 1024 or 1280 way */
    uint32_t guild_small;          /* the 800x600 way */
    uint32_t centre_800;           /* its rectangle */
    uint32_t screen_window[2];
    uint32_t town_screen[5];
    uint32_t town_picture;
    uint32_t main_menu_picture;
    uint32_t battlefield[2];
    uint32_t layout_read;
} h4_build;

/* The build being run (set by heroes4_patches before the parts' set-up); NULL for one the patches
 * do not know. */
extern const h4_build *h4;
const h4_build *heroes4_find_build(const char *variant);

/* Wide layouts, made from the game's own when it reads them (widescreen.c). */
void heroes4_widescreen(void);

#endif
