# Release notes (draft)

Draft for the first release, v0.1. Nothing is released yet. The notes say what has been shown to work and what is
known not to, so that players know what to expect.

## What you need

- Your own copy of the game: the German GOG installer of *Heroes of Might and Magic IV – Complete*
  (`setup_heroes_of_might_and_magic_4_complete_3.0_(german)_(25023).exe` and the `-1.bin` file beside it). Nothing of
  the game is included or downloaded.
- One of:
  - the converter page in a desktop browser (Chromium-based shown to work): it converts in the tab in about
    2½ minutes and needs about 2.2 GB of memory and 1.1 GB of browser storage; then the game plays in the page, or a
    package for Windows, Linux or Android is downloaded;
  - the converter program: `recomp convert --system linux <installer>` (about 2 minutes);
  - a build from source (CMake, a C compiler, Rust, SDL3 with SDL3_ttf and SDL3_mixer): compiling the 360 MB of
    recompiled C takes about 6½ minutes on 8 cores.

## What works

Shown in scripted runs on Linux (input from a script, screenshots compared by eye; the scenario runs repeat
exactly):

- The intro movies with their sound, the menus, the scenario list and player settings.
- A scenario: the adventure map, a town screen, a hero's walk, a battle won with automatic combat, the computer
  players' turns; saving and loading.
- An hour of game time with random input on the adventure map without a fault.
- A network game: a second copy finds the hosting one (TCP/IP), joins and reaches the game set-up.
- The campaign editor starts, creates a map and shows it.
- The same game from the converter's native code, and in the browser (intro, menus, scenario list).

## Known issues

- No campaign or scenario has been played to its end by hand; problems that only show later are possible.
- Sound and music have not been checked by ear; game speed against the original has not been measured.
- Manual combat, spells, the expansions' campaigns, a full network game, and most of the editor's tools have not been
  tried yet.
- GameSpy (internet game lists) does not exist any more; play over the network by IP address or on a LAN.
- Only Linux and the browser have been run. CI is set up to build Windows, macOS and Android, but has never run.
