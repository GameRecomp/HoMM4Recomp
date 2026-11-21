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
| M0  | Foundation                        | ⬜ | Shared code imported from CulturesRecomp and renamed; CMake and Cargo build; CI and audit hook; `games/heroes4/game.toml` pins the installer and every binary; `recomp identify` recognises them. |
| M1  | Read the inputs                   | ⬜ | `recomp extract` unpacks the GOG installer with its `-1.bin` slice into `orig/heroes4/gog-de/`, byte-identical to innoextract, without third-party tools. The runtime loader maps `Heroes4.exe`. `--check` lists the missing imports. |
| M2  | Find the code                     | ⬜ | Coverage report for `Heroes4.exe`, `binkw32.dll` (and `Mp3dec.asi` if lifted): 100 % classified, every leftover settled by analysis or a manifest hint, as in both sister projects. |
| M3  | Lift and build                    | ⬜ | Every function lifts; no trap in reachable code; the generated C compiles warning-free (gcc, clang, MSVC in CI) and links into `homm4-heroes4`; build time and memory measured and acceptable (see "Build size"). |
| M4  | Boot to the main loop             | ⬜ | CRT start-up, registry, archives, threads; the game creates its window and asks DirectDraw 7 for a mode. |
| M5  | Picture and input                 | ⬜ | Intro movies (Bink) and the main menu render; mouse and keyboard work; windowed and full-screen. |
| M6  | Sound and music                   | ⬜ | DirectSound effects and ambience; MP3 music from `music.h4r` through the Miles RIB MP3 codec; Bink movie sound. |
| M7  | Adventure map and combat          | ⬜ | A scenario map starts: adventure map, town screen, hero screen, a combat fought to the end, an AI turn. Save and load round-trip. |
| M8  | In game                           | ⬜ | A campaign map of the base game played through by hand; a scripted one-hour soak without trap; hotseat with two players; game speed and AI turn time measured. |
| M9  | **Release v0.1**                  | ⬜ | Base game, *Gathering Storm* and *Winds of War* campaigns each start and their first map is played through; Linux, Windows and the CLI converter; release notes with the known issues. |
| M10 | Multiplayer                       | ⬜ | TCP/IP games between two recompiled builds over LAN (host sockets under `wsock32`); GameSpy menu entries answer "not available" cleanly. |
| M11 | Campaign editor                   | ⬜ | `campaign_editor.exe` lifted and running on the shared user32 desktop: open, edit and save a campaign; the game plays it. |
| M12 | Converter and platforms           | ⬜ | `recomp convert` and the browser page turn the installer (exe + bin) into Windows, Linux, Android and browser builds; all four start and reach a map. |
| M13 | Polish                            | ⬜ | Opt-in: resolutions following the window and widescreen, integer scaling, touch controls (Android, browser), faster AI turns, settings in `settings.ini`. |

### M0 – Foundation

- [ ] Copy from CulturesRecomp at a pinned commit (record it in the decision log): `recomp/` (all crates),
      `runtime/`, `hle/`, `platform/`, `cmake/`, `third_party/`, `tools/`, `web/`, `android/`, `.github/`,
      `.clang-format`, `.editorconfig`, `.gitattributes`, `CMakePresets.json`, `CONTRIBUTING.md`
- [ ] Remove what Heroes IV does not use from the build, without deleting shared modules that stay general:
      the five Cultures game directories, their icons, the Cultures-only manifests; keep `dinput`, `dmusic`,
      `dshow`, `dplay` in the tree (shared, unused) unless they cost build time
- [ ] Rename: project `homm4recomp`, CLI help, window titles, user directory (`homm4recomp/<game>`), web page,
      Android package id, AppImage names, release asset names (`HoMM4Recomp-converter-<system>-<arch>`)
- [ ] `games/heroes4/game.toml`: the installer (both files), every binary with role and treatment (below),
      `gog_id = "1207658915"`; `games/heroes4/CMakeLists.txt` → `rc_add_game(heroes4 …)` → `homm4-heroes4`
- [ ] `docs/ARCHITECTURE.md`, `docs/WORKFLOWS.md`, `docs/DEVELOPMENT.md` adapted from CulturesRecomp
- [ ] `tools/audit.sh`, `tools/hooks/` (pre-commit, commit-msg) enabled; `.gitignore` covers `orig/`,
      `generated/`, `*.bin`, `*.h4r`, `*.h4c`, `*.h4s`, `*.bik`
- [ ] CI green on Linux (audit, Rust fmt/clippy/test, C build and tests with the null platform); Windows and
      macOS runners configured as in the sister projects
- [ ] Icon (`tools/icons/heroes4.svg`, our own drawing, no game art)

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

- [ ] `rc-media`: Inno Setup 5.6.2 (u): the header and entry layouts that differ from 5.5.0 (setup header
      fields, file entry flags), checked against the innoextract source as a reference (behaviour only)
- [ ] External data slices: `setup_….exe` + `setup_…-1.bin` (the `.bin` holds all file data, 1.05 GB);
      `Archive` gets a "slice provider" so the CLI opens sibling files and the browser asks for both files
- [ ] GOG language components: the German installer lists several language-tagged entries for the same path
      (`goggame-*.script`, `[de-DE]` files); select by the manifest variant's language
- [ ] Files installed twice (`binkw32.dll`, `Mss32.dll`, `Mp3dec.asi` in the root and in `Data/`): keep both,
      as the installer does (same hash)
- [ ] Installer registry entries → `registry-install.txt` (as for Cultures 2): the game reads
      `HKLM\SOFTWARE\New World Computing\Heroes of Might and Magic IV\1.0`
- [ ] Verified byte-identical against innoextract 1.9; ignored test `orig_extract` for `orig/`
- [ ] Streaming: the `.bin` is read sequentially without holding it in memory (the browser has 1 GB in a `File`)
- [ ] `homm4-heroes4 --check` loads `Heroes4.exe` and lists unimplemented imports (expected: those in RESEARCH)

### M2 – Find the code

- [ ] Discovery on `Heroes4.exe` with the shared rules; note how long it takes (5.4 MiB, ~8 times Cultures 1)
- [ ] RTTI: `Heroes4.exe` has MSVC RTTI (`.?AV…@@` type descriptors). Find vtables through the complete object
      locators (the slot before each vtable points to a COL whose type descriptor is in `.data`) — a general
      rule in `rc-discover`, more precise than the constructor-store heuristic C1 needed. Check it does not
      change the result for the Cultures and Tropico binaries (run their tests against the copied crate)
- [ ] C++ EH `FuncInfo` and SEH tables (MSVC 6), `switch` tables, x87 CRT second entries: shared rules; new
      patterns become general rules, not per-address hints, where possible
- [ ] `binkw32.dll` (this build) and `Mp3dec.asi`: discovery with their relocations, rebased to manifest bases
      if they collide (`binkw32` prefers `0x10000000`, `Mp3dec.asi` `0x26F00000`, `Mss32` is HLE)
- [ ] 100 % classified, no unknown byte; every `no-return?` candidate checked; ignored test `orig_discover`
- [ ] CPU features used: the linear-sweep counts show almost no MMX/SSE in `Heroes4.exe` (likely data) — confirm
      from the code map. `binkw32.dll` has MMX paths (lifted since Cultures M10); `Mp3dec.asi` has MMX and
      **3DNow!** paths: `cpuid` must not report 3DNow! (extended leaf `0x80000001`), so those stay dead

### M3 – Lift and build

- [ ] `recomp lift` on all lifted binaries; traps listed (expected: CRT `int 6`, `int3` only)
- [ ] Difftest `--coverage` on every lifted binary: every instruction form used is tested
- [ ] **Build size** (main risk, see "Risks"): Cultures 1 gives ~42 MB of C for 0.64 MiB of code, so expect
      ~350 MB for `Heroes4.exe`. Measure and then decide:
  - [ ] files of bounded size (split `code_XXXX.c` by function count or bytes, not only by 64 KiB of guest space)
        so `make -j` parallelises and no single file needs gigabytes of compiler memory
  - [ ] optimisation level for generated code per build type (`-O1` in dev, `-O2` in release), measured
  - [ ] ccache in CI; the lift is deterministic, so unchanged files are not recompiled (already true)
  - [ ] Cranelift (`rc-emit-native`) compile time for the converter; wasm module size for the browser
- [ ] Warning-free with the full warning set on gcc and clang; MSVC in CI
- [ ] Links into `homm4-heroes4`; dispatch tables for all lifted binaries

### M4 – Boot to the main loop

- [ ] New kernel32 imports: `GetSystemInfo`, `GetProcessHeap`, `GetSystemTimeAsFileTime`, `VirtualQuery(Ex)`,
      `VirtualLock`, `OpenFileMappingA`, `MapViewOfFileEx`, `OpenMutexA`, `OpenProcess`, `CreateProcessA`
      (find out what it starts: probably the GameSpy Arcade or a browser → refuse cleanly); ole32
      `CoInitializeEx`; winmm `timeBeginPeriod`/`timeEndPeriod`; user32 `MsgWaitForMultipleObjects`
- [ ] Single-instance check (`CreateMutexA`/`OpenMutexA`) answers "first instance"
- [ ] Registry: the installer's keys first, then the user's `registry.txt`
- [ ] Archives: the game opens `data/*.h4r` (`heroes4`, `text`, `movies`, `music`, `storm`, `storm_override`,
      `x2`, `x2_override`, `updates`; `new`, `storm_updates`, `x2_updates` are probed and may be missing).
      `CreateFileMapping`/`MapViewOfFile(Ex)` on these files: how much is mapped at once and where
      (**open**, RESEARCH). Native: map the host file into guest memory; browser: read through
      `rc_host_fread` in blocks (decision 2026-11-21 of CulturesRecomp)
- [ ] Threads: `CreateThread`, events, mutexes, `WaitForSingleObject`, `MsgWaitForMultipleObjects` on the shared
      thread layer; find out what each thread does (loading, AI, sound streaming) (**open**)
- [ ] `CoCreateInstance`: which class (**open**: DirectSound, DirectDraw, or something optional)
- [ ] The game creates its window (with a menu bar: `LoadMenuA`/`SetMenu` — windowed mode) and calls
      `DirectDrawCreateEx`

### M5 – Picture and input

- [ ] `ddraw` DirectDraw 7: the game's modes (800 × 600 and larger, 16-bit; confirm), surfaces, `Lock`/`Unlock`,
      blits, `Flip`; windowed mode with a clipper and `Blt` to the primary surface; GDI `CreateDIBSection` +
      `BitBlt` paths
- [ ] Window menu (`LoadMenuA`, `SetMenu`, `CheckMenuItem`, `EnableMenuItem`, accelerators): drawn by our
      user32 or hidden; its commands reachable (decision in M5)
- [ ] Input from window messages: mouse (`SetCapture`, `WindowFromPoint`, `GetCursorPos`, `ClientToScreen`),
      keyboard (`GetKeyState`, accelerators); cursors from the game's resources (`LoadImageA`, `LoadCursorA`)
- [ ] Bink intro movies from `movies.h4r` through the lifted `binkw32.dll` (`BinkCopyToBuffer` into a surface)
- [ ] Main menu, the scenario list, the options screen render; scripted runs (`RC_INPUT`, `RC_SCREENSHOT`)

### M6 – Sound and music

- [ ] DirectSound (`DirectSoundCreate`, CulturesRecomp's `dsound` on the platform PCM streams): the game's
      buffers, looping ambience, 3D/pan if used, `Bink`'s DirectSound track
- [ ] Miles RIB: `AIL_startup`, `AIL_shutdown`, `RIB_find_files_provider` (finds `*.asi` providers by attribute,
      e.g. "Input file types" = `*.mp3`), `RIB_request_interface` (returns the provider's function table).
      Two options, decide by effort:
  - **lift `Mp3dec.asi`** (proposal): our `mss32` loads the lifted provider, runs its `RIB_Main` registration and
    hands its interface to the game. Faithful and consistent with "lift middleware with a narrow API".
  - **HLE the codec**: `ASI_stream_open`/`ASI_stream_process`/… in `hle/mss32` on a vendored MP3 decoder
    (minimp3, CC0). Smaller code for wasm, but a second MP3 implementation to keep right.
- [ ] Music tracks from `music.h4r` stream without gaps; volume sliders
- [ ] By ear on a desktop: balance of music, effects and movie sound

### M7 – Adventure map and combat

- [ ] New game on a scenario map (single player against the AI): adventure map scrolls, heroes move, towns open
- [ ] Combat screen: a battle fought to the end (manual and quick combat), spells, the AI's moves
- [ ] AI turn completes; its duration measured against the original on Windows (if available) or a reference
- [ ] Save and load (`.h4s` in the user directory), quick save, the high-score table (`high_scores.dat`, copied
      to the user directory on first write)
- [ ] Screens: hero, town (all town types), kingdom overview, marketplace, the map editor's random-map options
      if present

### M8 – In game

- [ ] A base-game campaign map played through by hand; campaign progress kept between maps
- [ ] Scripted soak: one hour of random clicks with `RC_CLOCK=virtual` on the adventure map and in combat, no trap
- [ ] Hotseat game with two human players
- [ ] Record and replay input (`RC_RECORD`) used for regression runs of a fixed opening
- [ ] Timing: animation and scrolling speed match the original (`timeGetTime`, `timeBeginPeriod(1)`)

### M9 – Release v0.1

- [ ] Campaign start and first map of: the base game, *The Gathering Storm*, *Winds of War*
- [ ] Linux (AppImage) and Windows (zip) shells from CI; the CLI converter for both
- [ ] `docs/RELEASE-NOTES.md` with known issues; README for players
- [ ] English GOG installer pinned if available (another `[[variant]]`, same plan otherwise)

### M10 – Multiplayer

- [ ] `wsock32` by ordinal on host sockets (CulturesRecomp's `hle/wsock` is offline so far): `socket`,
      `connect`, `closesocket`, `htons`, `inet_addr`, `inet_ntoa`,
      `gethostbyname`, `gethostname`, `getsockname`; find where `send`/`recv`/`WSAStartup` come from
      (dynamic `GetProcAddress`? **open**)
- [ ] TCP/IP host and join between two recompiled builds on one machine and on a LAN; a full game played
- [ ] GameSpy: the menu entries fail cleanly; no connection attempts to dead servers (patch if needed)
- [ ] Browser: no raw sockets — WebSocket relay or "not available" (decision when there)

### M11 – Campaign editor

- [ ] Discover and lift `campaign_editor.exe` (3.1 MiB `.text`, statically linked MFC: expect many vtables and
      message maps)
- [ ] The ~71 imports CulturesRecomp's desktop lacks (RESEARCH): MDI (`DefFrameProcA`, `DefMDIChildProcA`,
      `TranslateMDISysAccel`), property sheets, image lists and drag images, clipboard, OLE structured storage and
      drag and drop (`StgCreateDocfileOnILockBytes`, `RegisterDragDrop`, …), `oledlg`, `OLEPRO32`, `OLEAUT32`
- [ ] Open a campaign, change it, save it; the game plays it

### M12 – Converter and platforms

- [ ] `recomp convert --system <linux|windows|android|web> setup_….exe` (finds the `-1.bin` beside it)
- [ ] Browser page: picks both files (or a folder), reads the 1 GB slice in streamed blocks, stores the
      unpacked game in the origin's storage (OPFS) once; the player loads it from there
- [ ] Code size in the browser: measure the wasm module for `Heroes4.exe`; if too large for comfortable
      compilation, split into lazily compiled modules or reduce per-function overhead in `rc-emit-wasm`
- [ ] Android: APK from the template, data in app storage (1 GB), touch input (tap = left click, long press =
      right click info popups, two-finger drag = scroll), hardware back = Escape
- [ ] Release workflow: converter binaries for Windows and Linux, shell templates, the page on GitHub Pages

### M13 – Polish

- [ ] Resolutions following the window and widescreen (the adventure map can show more; the UI screens are
      fixed-size and centred), as patches in `games/heroes4/patches.c` with manifest patch points
- [ ] Integer and smooth scaling, hardware cursor, fps limit (shared `settings.ini`)
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
