# Plan

## Goals

1. **Heroes of Might and Magic IV runs natively** from its original executable, with both expansions
   (*The Gathering Storm*, *Winds of War*): the GOG "Complete" release first (German, `3.0`, see
   [TARGETS.md](TARGETS.md)), then the other GOG languages as their installers are pinned.
2. **Same platforms as TropicoRecomp and CulturesRecomp:** Windows, Linux and Android (x86-64 and ARM64) and
   the browser (WebAssembly); macOS is built in CI but not offered (`platforms.json`, as in both projects).
3. **SDL3 is the only platform dependency** (SDL3, SDL3_ttf, SDL3_mixer), through the shared `platform/` layer.
4. **Same architecture and the same code** as the two sister projects: the Rust recompiler (`recomp/`), the C11
   runtime, HLE and platform layers, the manifests, the converter (CLI and browser page), CI and the packaging.
   Shared code stays general, so fixes can travel between the three repositories.
5. **Faithful first, improvements second.** The game logic is the original code. Resolutions, widescreen, faster
   AI turns and touch controls come as opt-in patches once the game plays through.
6. **Later:** the campaign editor, and TCP/IP multiplayer between recompiled builds.

Non-goals: shipping game code or data, remastering assets, GameSpy (the service is gone), supporting unknown or
cracked executables, network compatibility with original Windows builds.

## Starting point

This repository is the third project on the same toolchain. Everything below "the game" is already written:

| Piece | Source | State there | Work here |
|-------|--------|-------------|-----------|
| `rc-pe`, `rc-manifest`, `rc-discover`, `rc-ir`, `rc-x86`, `rc-emit-c`, `rc-difftest` | CulturesRecomp | 100 % discovery, difftest 18720/18720 + MMX | scale to a 5.4 MiB `.text`, RTTI-based vtables |
| `rc-media` (Inno, LZMA, ISO, zip) | CulturesRecomp | Inno 5.5.0 (u), one file | **Inno 5.6.2 (u) with an external `-1.bin` slice** |
| `rc-emit-native` (Cranelift), `rc-emit-wasm`, `rc-convert`, `rc-web` | TropicoRecomp → CulturesRecomp | both games convert in the browser | a 1 GB second input file, a much larger code module |
| `runtime/` (memory, CPU, x87, dispatch, loader, threads, clock, host files) | both | runs two engines and four DLL sets | file mapping of large archives, more threads |
| `hle/` kernel32, user32, gdi32, advapi32, ole32, ddraw, dsound, mss32, winmm, wsock, comctl32, comdlg32 | CulturesRecomp | | ~30 new imports for the game, ~70 for the editor, Miles RIB |
| `platform/` SDL3 + null | both | | none expected |
| `web/`, `android/`, `tools/`, `.github/workflows` | CulturesRecomp | | rename, two-file input, icons |

**CulturesRecomp is the template**, because it already contains TropicoRecomp's shared code plus the general
additions (several lifted binaries per game, DLL rebasing, COM class registry, DirectSound, the editor desktop).
The code is copied at a pinned commit (decision log), not vendored as a submodule, as CulturesRecomp did with
TropicoRecomp.

The game, measured from the German GOG installer (details in [RESEARCH.md](RESEARCH.md)):

- `Heroes4.exe`: PE32, MSVC 6 linker, image base `0x400000`, **no relocations**, `.text` 5.43 MiB (8.5 times
  Cultures 1, about 2.5 times Tropico 2), RTTI present, 197 imports. 167 of them are already implemented by
  CulturesRecomp's HLE layer; 13 are Bink (lifted), 2 Miles RIB, ~15 small kernel32/user32/winmm/ole32 calls.
- Picture: DirectDraw 7 (`DirectDrawCreateEx`); input: window messages only (no DirectInput); sound:
  DirectSound directly (`DirectSoundCreate`), Bink movies through `BinkOpenDirectSound`; music: MP3 decoded
  through the Miles RIB interface (`RIB_find_files_provider`, `RIB_request_interface` → `Mp3dec.asi`).
- Network: `WSOCK32` by ordinal (TCP/IP) and GameSpy strings.
- Threads: `CreateThread`, events, mutexes, critical sections, `MsgWaitForMultipleObjects`.
- Data: `.h4r` archives (568 MB `heroes4.h4r`), read by the game itself, partly through `MapViewOfFile(Ex)`.
- `campaign_editor.exe`: MFC (statically linked), OLE, MDI, property sheets, 515 imports.

## Order of work

```
 M0 ─ M1 ─ M2 ─ M3 ─ M4 ─ M5 ─ M6 ─ M7 ─ M8 ─ M9            Heroes4.exe (game, gog-de)
                                         │
                                         ├─ M10 multiplayer (hotseat in M8, TCP/IP here)
                                         ├─ M11 campaign editor
                                         ├─ M12 converter, packages, browser, Android
                                         └─ M13 polish (resolutions, touch, speed)
```

M10–M13 run in parallel once M8 has the game playable. M12 starts early in a reduced form (the CLI convert for
Linux) because the code size of `Heroes4.exe` is the main technical risk for the browser and Android builds and
should be measured early (see "Risks").

## Milestones

Status: ✅ done · 🔶 in progress · ⬜ not started

| #   | Milestone                         | Status | Exit criterion |
|-----|-----------------------------------|--------|----------------|
| M0  | Foundation                        | ✅ | Shared code imported from CulturesRecomp and renamed; CMake and Cargo build; CI and audit hook; `games/heroes4/game.toml` pins the installer and every binary; `recomp identify` recognises them. |
| M1  | Read the inputs                   | ✅ | `recomp extract` unpacks the GOG installer with its `-1.bin` slice into `orig/heroes4/gog-de/`, byte-identical to innoextract, without third-party tools. The runtime loader maps `Heroes4.exe`. `--check` lists the missing imports. |
| M2  | Find the code                     | ✅ | Coverage report for `Heroes4.exe`, `binkw32.dll` (and `Mp3dec.asi` if lifted): 100 % classified, every leftover settled by analysis or a manifest hint, as in both sister projects. |
| M3  | Lift and build                    | ✅ | Every function lifts; no trap in reachable code; the generated C compiles warning-free (gcc, clang, MSVC in CI) and links into `homm4-heroes4`; build time and memory measured and acceptable (see "Build size"). |
| M4  | Boot to the main loop             | ✅ | CRT start-up, registry, archives, threads; the game creates its window and asks DirectDraw 7 for a mode. |
| M5  | Picture and input                 | 🔶 | Intro movies (Bink) and the main menu render; mouse and keyboard work; windowed and full-screen. |
| M6  | Sound and music                   | 🔶 | DirectSound effects and ambience; MP3 music from `music.h4r` through the Miles RIB MP3 codec; Bink movie sound. |
| M7  | Adventure map and combat          | 🔶 | A scenario map starts: adventure map, town screen, hero screen, a combat fought to the end, an AI turn. Save and load round-trip. |
| M8  | In game                           | 🔶 | A campaign map of the base game played through by hand; a scripted one-hour soak without trap; hotseat with two players; game speed and AI turn time measured. |
| M9  | **Release v0.1**                  | ⬜ | Base game, *Gathering Storm* and *Winds of War* campaigns each start and their first map is played through; Linux, Windows and the CLI converter; release notes with the known issues. |
| M10 | Multiplayer                       | 🔶 | TCP/IP games between two recompiled builds over LAN (host sockets under `wsock32`); GameSpy menu entries answer "not available" cleanly. |
| M11 | Campaign editor                   | 🔶 | `campaign_editor.exe` lifted and running on the shared user32 desktop: open, edit and save a campaign; the game plays it. |
| M12 | Converter and platforms           | 🔶 | `recomp convert` and the browser page turn the installer (exe + bin) into Windows, Linux, Android and browser builds; all four start and reach a map. |
| M13 | Polish                            | ⬜ | Opt-in: resolutions following the window and widescreen, integer scaling, touch controls (Android, browser), faster AI turns, settings in `settings.ini`. |

### M0 – Foundation

- [x] Copy from CulturesRecomp at a pinned commit (record it in the decision log): `recomp/` (all crates),
      `runtime/`, `hle/`, `platform/`, `cmake/`, `third_party/`, `tools/`, `web/`, `android/`, `.github/`,
      `.clang-format`, `.editorconfig`, `.gitattributes`, `CMakePresets.json`, `CONTRIBUTING.md`
- [x] Remove what Heroes IV does not use from the build, without deleting shared modules that stay general:
      the five Cultures game directories, their icons, the Cultures-only manifests; `dinput`, `dmusic`,
      `dshow`, `dplay` stay in the tree (shared, unused)
- [x] Rename: project `homm4recomp`, CLI help, window titles, user directory (`homm4recomp/<game>`), web page,
      Android package id, AppImage names, release asset names (`HoMM4Recomp-converter-<system>-<arch>`)
- [x] `games/heroes4/game.toml`: the installer (both files), every binary with role and treatment (below),
      `gog_id = "1207658915"`; `games/heroes4/CMakeLists.txt` → `rc_add_game(heroes4 …)` → `homm4-heroes4`
- [x] `docs/ARCHITECTURE.md`, `docs/DEVELOPMENT.md` adapted from CulturesRecomp (`WORKFLOWS.md` with M12)
- [x] `tools/audit.sh`, `tools/hooks/` (pre-commit, commit-msg) enabled; `.gitignore` covers `orig/`,
      `generated/`, `*.bin`, `*.h4r`, `*.h4c`, `*.h4s`, `*.bik`
- [x] CI configuration as in the sister projects (audit, Rust fmt/clippy/test, C build and tests with the null
      platform on Linux, Windows, macOS); the same steps pass locally on Linux (CI has not run: nothing pushed)
- [x] Icon (`tools/icons/heroes4.svg`, our own drawing, no game art)

Binary treatment (first proposal, see M2 and M6):

| File | Role | Treatment | Why |
|------|------|-----------|-----|
| `Heroes4.exe` | main | lift | the game |
| `binkw32.dll` | dll | lift | Bink 1 movies; game-side code with a narrow API (as in both projects); a different build than theirs |
| `Mss32.dll` | dll | hle | the same build (MSS 6.1c) that CulturesRecomp already replaces |
| `Mp3dec.asi` | dll | lift (proposal) | the MP3 codec Miles loads; narrow API below it (kernel32, `RIB_register`) — decision in M6 |
| `campaign_editor.exe` | tool | lift (M11) | MFC editor |
| `__redist/ISI/scriptinterpreter.exe`, GOG files | – | skip | installer helpers |

### M1 – Read the inputs

- [x] `rc-media`: Inno Setup 5.6.2 (u): three more header strings and the image alpha format in place of the
      image back colour (RESEARCH); the entry layouts are unchanged
- [x] External data slices: `setup_….exe` + `setup_…-1.bin` (the `.bin` holds all file data, 1.05 GB);
      `Archive` gets a "slice provider" so the CLI opens sibling files and the browser asks for both files
- [x] GOG Galaxy files: each file is stored as zlib parts in `{tmp}` and named by script calls; they are read
      as the files they make. Language-tagged duplicates: the German installer has one entry per path except
      the GOG script (four, by Windows version): the first is kept
- [x] Files installed twice (`binkw32.dll`, `Mss32.dll`, `Mp3dec.asi` in the root and in `Data/`): keep both,
      as the installer does (same hash)
- [x] Installer registry entries → `registry-install.txt` (as for Cultures 2), with the `setRegistry` actions
      of GOG's `goggame-1207658915.script`, which write the game's own key
      `HKLM\Software\New World Computing\Heroes of Might and Magic IV\1.0`
- [x] Verified byte-identical against innoextract 1.9; ignored test `orig_extract` for `orig/`
- [x] Streaming: the `.bin` is read sequentially without holding it in memory (the browser has 1 GB in a `File`)
- [x] `homm4-heroes4 --check` loads `Heroes4.exe` and lists unimplemented imports: 167 of 197 implemented, the
      30 expected ones missing (RESEARCH)

### M2 – Find the code

- [x] Discovery on `Heroes4.exe` with the shared rules: 99.96 % at once, 18 s
- [x] Vtables: the shared constructor-store and pointer rules already find 3034 vtables (26,396 slots), every
      byte is classified, so an RTTI rule was not needed (it may still help other games later)
- [x] C++ EH `FuncInfo` and SEH tables (MSVC 6), `switch` tables, x87 CRT second entries: shared rules. Three new
      general rules (alignment fill of leftover bytes, dead tails after no-return calls, x87 instructions between
      `cmp` and `ja`), checked to change nothing on CulturesRecomp's binaries
- [x] `binkw32.dll` (this build) and `Mp3dec.asi`: 100 % with and without their relocations; their bases do not
      collide (`0x10000000`, `0x26F00000`; `Mss32` is HLE)
- [x] 100 % classified, no unknown byte; every `no-return?` candidate checked; ignored test `orig_discover`
- [x] CPU features: `Heroes4.exe` uses only integer and x87 code. `binkw32.dll` has MMX paths (lifted);
      `Mp3dec.asi` has MMX and **3DNow!** paths, and the runtime's `cpuid` reports no extended leaves, so
      3DNow! stays dead

### M3 – Lift and build

- [x] `recomp lift` on all lifted binaries: `Heroes4.exe` only the CRT's 16 `int 6`; `binkw32.dll` the same;
      `Mp3dec.asi` also its 3DNow! paths (dead: no 3DNow! in `cpuid`)
- [ ] Difftest `--coverage` on every lifted binary: every instruction form used is tested
- [ ] **Build size** (main risk, see "Risks"): Cultures 1 gives ~42 MB of C for 0.64 MiB of code, so expect
      ~350 MB for `Heroes4.exe`. Measure and then decide:
  - [x] file sizes: 87 files of up to 10 MB (one per 64 KiB of guest code) compile in parallel without
        trouble; a full build takes 6½ minutes on 8 cores (38 CPU-minutes), about 2 GB per compiler
  - [x] each code file declares only the functions it names, so a re-lift that adds a function recompiles
        only the files that changed (before: every file, through `funcs.h`)
  - [x] optimisation level: `-O1` for generated code (as in the sister projects); `-O2` to be measured in M13
  - [ ] ccache in CI; the lift is deterministic, so unchanged files are not recompiled (already true)
  - [ ] Cranelift (`rc-emit-native`) compile time for the converter; wasm module size for the browser
- [x] Warning-free with the full warning set on gcc (clang and MSVC in CI)
- [x] Links into `homm4-heroes4` (600 MB with debug info); dispatch tables for all lifted binaries

### M4 – Boot to the main loop

- [x] New kernel32 imports: `GetSystemInfo`, `GetProcessHeap`, `GetSystemTimeAsFileTime`, `VirtualQuery(Ex)`,
      `VirtualLock`, `OpenFileMappingA`, `MapViewOfFileEx`, `OpenMutexA`, `OpenProcess`, `CreateProcessA`
      (find out what it starts: probably the GameSpy Arcade or a browser → refuse cleanly); ole32
      `CoInitializeEx`; winmm `timeBeginPeriod`/`timeEndPeriod`; user32 `MsgWaitForMultipleObjects`
- [x] Single-instance check (`CreateMutexA`/`OpenMutexA`) answers "first instance"
- [x] Registry: the installer's keys first, then the user's `registry.txt`
- [x] Archives: the game opens `data/*.h4r` (`heroes4`, `text`, `movies`, `music`, `storm`, `storm_override`,
      `x2`, `x2_override`, `updates`; `new`, `storm_updates`, `x2_updates` are probed and may be missing).
      `CreateFileMapping`/`MapViewOfFile(Ex)` on these files: how much is mapped at once and where
      (**open**, RESEARCH). Native: map the host file into guest memory; browser: read through
      `rc_host_fread` in blocks (decision 2026-11-21 of CulturesRecomp)
- [x] Threads: `CreateThread`, events, mutexes, `WaitForSingleObject`, `MsgWaitForMultipleObjects` on the shared
      thread layer (what each thread does: still **open**)
- [x] `CoCreateInstance`: not reached up to the main menu
- [x] The game creates its window and calls `DirectDrawCreateEx`; the intro movie and the main menu render
- [x] Structured exceptions (new in the shared runtime): `RaiseException` and `RtlUnwind` walk the guest's
      `fs:[0]` chain and call its handlers (MSVC's own `__CxxFrameHandler`, `__except_handler3`, lifted); a
      handler's jump to a frame further up unwinds the lifted functions to it (a check after every call).
      The native and WebAssembly back ends need the same check (M12)

### M5 – Picture and input

- [x] `ddraw` DirectDraw 7: the game's 1024 × 768 16-bit mode, surfaces, `Lock`/`Unlock`, blits; `GetBltStatus`,
      `GetFlipStatus`, `PageLock`. The game copies to the screen only what changed: an idle frame after 50 ms
      without one keeps scripted runs, screenshots and the frame hook going
- [x] Window geometry: the game window gets its class's menu (as Windows does), and a game window counts the
      menu bar's room in its client area: Heroes IV moves its full-screen window to y = -20 (above the screen
      by the bar), and without that its picture and every point it converted were 20 pixels off
- [x] Input from window messages: mouse (the game converts points with `ClientToScreen` and hit-tests in
      screen space; it draws its own cursor from `GetCursorPos`), keyboard (Escape skips the movies)
- [x] Bink intro movies from `movies.h4r` through the lifted `binkw32.dll`
- [x] Main menu, the New Game menu, the scenario list, the player settings, the adventure map, a town screen,
      the combat screen and its options, the system menu, the save and load dialogs render; scripted runs
      (`tools/run-scenario heroes4 menu|firstturn|battle`) repeat exactly
- [ ] By hand on a desktop: windowed mode and its menu bar (the game offers a window), full screen, Alt+Enter

### M6 – Sound and music

- [x] DirectSound (`DirectSoundCreate`, CulturesRecomp's `dsound` on the platform PCM streams): the game's
      buffers and Bink's DirectSound track play; on the virtual clock the play position follows that clock
      (scripted runs repeat)
- [x] Miles RIB: `Mp3dec.asi` (lifted) registers "ASI codec" and "ASI stream" from its `DllMain`;
      `RIB_find_files_provider` matches its file-type filter list, `RIB_request_interface` writes the entries
      into the game's variables (the RIB functions are stdcall); Miles starts the codec (`ASI_startup`)
      before handing it out. The game decodes its music through it into DirectSound. Decided: **lift**
      (below, the alternative kept for reference)
  - **lift `Mp3dec.asi`** (proposal): our `mss32` loads the lifted provider, runs its `RIB_Main` registration and
    hands its interface to the game. Faithful and consistent with "lift middleware with a narrow API".
  - **HLE the codec**: `ASI_stream_open`/`ASI_stream_process`/… in `hle/mss32` on a vendored MP3 decoder
    (minimp3, CC0). Smaller code for wasm, but a second MP3 implementation to keep right.
- [ ] Music tracks from `music.h4r` stream without gaps (decoding runs; to be heard); volume sliders
- [ ] By ear on a desktop: balance of music, effects and movie sound

### M7 – Adventure map and combat

- [x] New game on a scenario map (single player against the AI): a hero moves (path shown), a town opens
- [x] Combat screen: a battle fought to the end with automatic combat, the result screen
- [x] Manual combat: the active unit attacks a chosen enemy, the next unit (the hero) gets its turn
- [ ] Spells
- [x] AI turn completes (the computer players' first turn; duration still to measure)
- [x] Save and load (`.h4s` in the user directory's `games/`): saved over the autosave from the system menu,
      loaded again from the main menu; quick save and the high-score table still to check
- [ ] Screens: hero, town (all town types), kingdom overview, marketplace, the map editor's random-map options
      if present

### M8 – In game

- [ ] A base-game campaign map played through by hand; campaign progress kept between maps
- [x] Scripted soak: 74,000 frames (about an hour of game time) of random clicks, right clicks, Escape and
      Enter on the adventure map from the first scenario (`tools/soak-input`, seed 1), no trap, 18 minutes
- [ ] Hotseat game with two human players
- [ ] Record and replay input (`RC_RECORD`) used for regression runs of a fixed opening
- [ ] Timing: animation and scrolling speed match the original (`timeGetTime`, `timeBeginPeriod(1)`)

### M9 – Release v0.1

- [x] Campaign start and first map of: the base game, *The Gathering Storm*, *Winds of War* (scenarios
      `campaign-h4`, `campaign-gs`, `campaign-wow`); playing them through is still to do
- [ ] Linux (AppImage) and Windows (zip) shells from CI; the CLI converter for both
- [x] `docs/RELEASE-NOTES.md` with known issues (draft); README for players
- [ ] English GOG installer pinned if available (another `[[variant]]`, same plan otherwise)

### M10 – Multiplayer

- [x] Transport found: TCP/IP and LAN games go through DirectPlay 8 (`CoCreateInstance` of the DirectPlay 8
      objects), which CulturesRecomp's `dpnet` on its own network (`hle/dplay/net.c`) provides; the game uses
      `WSOCK32` only for its host name and address and GameSpy. Hosting a TCP/IP game reaches the network
      game's scenario list
- [x] TCP/IP host and join between two recompiled programs on one machine: the joining program finds the
      host's game at 127.0.0.1 (server, map, ping), joins with a player name and reaches the game set-up, where
      the host lists it (two scripted runs lined up with `shot`/`wait`)
- [x] The host shows its network address; the joining program receives the host's whole game set-up (the
      shared network held only 256 bytes of DirectPlay application data, Heroes IV uses 304)
- [x] Both players take a slot (the box at the right of a slot the map leaves to humans), tick Ready, the host
      starts: both programs are on the adventure map, the joining one with the game it received
      (`tools/run-netgame heroes4 netgame`)
- [x] Turns pass: the host ends its turn, the joining player gets theirs (same scenario, three runs of three)
- [ ] A full network game played; on a LAN between two machines
- [ ] GameSpy: the menu entries fail cleanly; no connection attempts to dead servers (patch if needed)
- [ ] Browser: no raw sockets — WebSocket relay or "not available" (decision when there)

### M11 – Campaign editor

- [x] Discover and lift `campaign_editor.exe`: 100 % (41,063 functions, 2148 vtables), 213 MB of C, linked into
      `homm4-heroes4` (`--exe campaign_editor.exe`, with `RC_DESKTOP=1024x768`)
- [x] It starts: main window, MDI document, tool bars, the object palette in a tab control, status bar, the New
      Campaign dialog; a new map is created and drawn
- [x] New in the shared HLE for it: MDI, the program's own clipboard, a tab control, region data and polygon
      regions, the OLE calls MFC makes at start-up (no other programs to share objects with)
- [ ] Still missing (on demand): property sheets, image lists and drag images, OLE structured storage
      (`StgCreateDocfileOnILockBytes`), `oledlg`, `OLEPRO32`, `OLEAUT32`
- [x] Save: a new map saved through our Save As dialog lands in the user folder's `maps/`; the game reads it
      with its own maps (it lists only maps with players, which a fresh map does not have)
- [ ] Open a campaign, change it (towns, heroes, players), save it; the game plays it

### M12 – Converter and platforms

- [x] The native back end (Cranelift) compiles the whole game in 1 min 22 s (58 MB of x86-64 code); a package
      made with `recomp template` + `recomp package` from a shell without lifted code plays the `firstturn`
      scenario with exactly the C build's API calls and screenshot. Exceptions are unwound in the native and
      WebAssembly back ends as in C
- [x] `recomp convert --template T.zip setup_….exe` (finds the `-1.bin` beside it): a Linux package of the game
      and its editor in 2 minutes
- [ ] Windows, macOS and Android shells and templates built in CI (no Android SDK here; CI has never run)
- [x] Browser page: picks both files, reads the 1 GB slice in streamed blocks, stores the unpacked game in the
      origin's storage (OPFS); shown in headless Chromium: conversion in 2½ minutes, then the player runs the
      intro, the main menu, the New Game menu (C++ exceptions) and the scenario list
- [x] Code size in the browser: the module is 109 MB; browsers compile it lazily and it loads without
      trouble (2.2 GB peak while converting); splitting is not needed for now
- [ ] Android: APK from the template, data in app storage (1 GB), touch input (tap = left click, long press =
      right click info popups, two-finger drag = scroll), hardware back = Escape
- [ ] Release workflow: converter binaries for Windows and Linux, shell templates, the page on GitHub Pages

### M13 – Polish

- [x] The game's own resolutions (800×600, 1024×768, 1280×1024) switch at run time through its settings
      (scenario `resolution`); settings and high scores screens open
- [ ] Resolutions following the window and widescreen (the adventure map can show more; the UI screens are
      fixed-size and centred), as patches in `games/heroes4/patches.c` with manifest patch points
- [ ] Integer and smooth scaling, hardware cursor, fps limit (shared `settings.ini`: present from the shared
      platform layer, not yet checked by eye with this game)
- [ ] Faster AI turns (profiling the lifted code; optional "skip AI animation" if the game has no such option)
- [ ] Performance pass: hot functions in the AI and pathfinding, `-O2` vs. Cranelift vs. wasm

## Definition of done (per release)

- Installer → build with one command (`recomp convert`) or from source with the documented steps.
- The game starts from a clean user directory and from an existing one; settings and saves persist.
- Base campaign + both expansion campaigns start; their first maps can be won; save/load works in all screens.
- One hour of scripted play without trap or leak; no unimplemented import reached.
- No copyrighted material in the repository, CI artifacts or release assets (`tools/audit.sh`).

## Risks

| Risk | Impact | Plan |
|------|--------|------|
| Code size: 5.4 MiB of x86 → ~350 MB of C | build time, compiler memory, CI limits, wasm size | measure in M3; bounded files, `-O1` for dev, Cranelift for the converter; lazy wasm modules if needed |
| Large file mappings (568 MB archive) | 4 GiB guest space, browser memory | find the mapping pattern in M4; map windows on demand; browser through `rc_host_fread` |
| Threads with real concurrency (loading, AI) | races, deadlocks in the cooperative thread layer | identify each thread in M4; real threads only where needed, as in the sister projects |
| Inno 5.6.2 + external slice | no extraction in M1 | reference behaviour of innoextract; the slice format is the same container, only the layout changes |
| AI turn speed (CPU-bound C++ code) | slow turns on phones and in the browser | measure in M7/M8; optimisation pass in M13 |
| MFC editor surface (OLE, MDI) | large HLE work in M11 | editor is optional and comes after the game |
| The GOG `Heroes4.exe` is a modified build (implausible link timestamp) | other builds differ | pin by hash; `recomp codediff` against other builds when they appear |

## Research questions

Tracked with answers in [RESEARCH.md](RESEARCH.md):

1. How does the game map the `.h4r` archives (whole file, windows, at fixed addresses with `MapViewOfFileEx`)?
2. What do the threads do, and do they need real concurrency?
3. What does `CreateProcessA` start, and what does `OpenProcess`/`VirtualQueryEx` inspect?
4. Which class does `CoCreateInstance` create?
5. Where do `send`/`recv`/`WSAStartup` come from (only 9 `WSOCK32` ordinals are imported)?
6. Which Miles RIB interface does the game request from `Mp3dec.asi`, and how does it feed DirectSound?
7. Which display modes does the game offer, and does it use 16-bit only?
8. Is the windowed-mode menu bar part of the normal game or a debug build leftover?
9. Is the GOG `Heroes4.exe` the 3.0 patch as released by 3DO/Ubisoft or modified by GOG (timestamp `0x73655965`)?

## Decision log

| Date       | Decision |
|------------|----------|
| 2026-11-21 | HoMM4Recomp follows TropicoRecomp and CulturesRecomp: MIT license, generated code and runtime in C11, the toolchain in Rust, SDL3 as the only platform dependency, the same platforms (Windows, Linux, Android, browser; macOS built in CI, not offered). |
| 2026-11-21 | CulturesRecomp is the template for the shared code (it contains TropicoRecomp's runtime, HLE and platform layers plus the general additions). Code is copied at a pinned commit and kept general, so fixes can go back to both projects. |
| 2026-11-21 | First target: the German GOG "Complete" installer (`3.0`, build 25023, Inno 5.6.2 with one external slice). Other languages and stores are added by hash only. |
| 2026-11-21 | Game id `heroes4`, executable `homm4-heroes4`; the campaign editor becomes a second program of the same game (M11). |
| 2026-11-21 | `binkw32.dll` is lifted, `Mss32.dll` is HLE (same build as Cultures'), `Mp3dec.asi` is proposed for lifting (final decision in M6). |
