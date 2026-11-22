# Research

Findings about the game and its binaries, in our own words. Unknowns are marked **(open)**. Addresses may appear
here; disassembly, lifted or decompiled code never does.

## The installer (gog-de)

- Inno Setup 5.6.2 (u), title "Heroes of Might and Magic® 4 - Complete", 406 entries. All file data is in the
  external slice `…-1.bin` (1.05 GB); the `.exe` holds the setup loader and headers.
- Neither CulturesRecomp's `rc-media` (Inno 5.5.0 (u), data in the exe) nor TropicoRecomp's `rc-inno` read it;
  `rc-media` does now (M1), byte-identical to innoextract 1.9. What 5.6.2 changes against 5.5.0 in the setup
  header: three more strings (setup mutex; ChangesEnvironment and ChangesAssociations, which became expressions),
  and from 5.5.7 the image alpha format (1 byte) in place of the image back colour (4 bytes).
- The slice file starts with `idska32\x1a` and its size (12 bytes); chunk offsets count from the start of the
  slice file. Every data entry is its own stored chunk here.
- GOG "Galaxy" layout: every file is installed as `{tmp}\xx\yy\<md5>`, a zlib stream of at most 10 MiB, and the
  file entries' script calls say what it is: `before_install('<md5>', '<path>', <parts>)` on the first part of a
  file, `after_install('<md5>', <stored size>, <size>)` on every part, the parts in order. Every part is a zlib
  stream even when both sizes are equal. Paths use Pascal quotes (`''`). `heroes4.h4r` has 56 parts.
- `check_if_install('de-DE#','32#64#','')` names the languages, architectures and Windows versions a file is for;
  the German installer has one entry per destination except `goggame-1207658915.script` (four, by Windows
  version).
- Some entries are language-tagged (`[de-DE]`), and `goggame-1207658915.script` appears four times with OS
  constraints (Windows 8, 8.1, 10, other); only one is installed.
- The installer's own `[Registry]` section writes only GOG keys. The game's keys come from GOG's post-install
  script `goggame-1207658915.script` (JSON, `setRegistry` actions), which `rc-media` applies too:
  `HKLM\Software\New World Computing\Heroes of Might and Magic IV\1.0` with `AppPath` = `{app}`, `CDDrive` =
  `{drive:{app}}`, `Language` = `07` (German), `Version` = `3.0wow`, `GS` = `1`. Two more actions, only for
  Windows 8 and later, set `full_screen` = 1 and `resolution` = 0x500 under HKCU; they are left out (the game's
  own defaults apply). The script also names `{app}/games` as the save folder.

## Heroes4.exe

- PE32 GUI, linker 6.0 (MSVC 6), image base `0x400000`, entry `0x8E3981`, **no relocations**.
- Sections: `.text` `0x401000` (0x56E30A bytes, 5.43 MiB), `.rdata` `0x970000` (0xD54F9), `.data` `0xA46000`
  (0x8F725 virtual, 0x48000 raw), `.rsrc` `0xAD6000`.
- Link timestamp `0x73655965` is not a plausible date (it reads as ASCII text); the build may be modified by GOG
  or its patcher **(open)**. Not packed: `.text` raw size equals its virtual size, the entry is a normal MSVC CRT
  start-up.
- MSVC RTTI is present (`.?AV…@@` type descriptors, heavy template use: `t_script_action<…>`, `t_effect<…>`):
  vtables can be found through their complete object locators.
- Discovery (M2): 62,636 functions, 3034 vtables (26,396 slots), 6600 C++ EH `FuncInfo`, 210 switch tables; 100 %
  of `.text` classified. The code uses only integer and x87 instructions: no MMX, SSE or `cmov` (the linear sweep's
  few hits were data). The space between functions is often filled with leftover bytes (instruction tails such as
  a stray `ff 15 <iat>`) instead of `int3`: a discovery rule for it. One switch has an `fstp` between its `cmp` and
  `ja`. The 52 indirect jumps left to run time are 46 vtable tail calls (`jmp [reg+n]`), and `jmp eax`/`jmp [ebx]`
  in the CRT's EH and x87 helpers.
- Lifting (M3): 63,293 functions, 1.84 million instructions, 344 MB of C in 87 files of up to 10 MB (one per 64 KiB
  of guest code); the only traps are the CRT's 16 `int 6` (Pentium FDIV check stubs).

### Imports (197)

| DLL | # | Notes |
|-----|--:|-------|
| `KERNEL32` | 108 | file mapping (`CreateFileMappingA`, `OpenFileMappingA`, `MapViewOfFile`, `MapViewOfFileEx`), threads (`CreateThread`, events, mutexes, `OpenMutexA`), `VirtualAlloc`/`VirtualQuery(Ex)`/`VirtualLock`, `CreateProcessA`, `OpenProcess`, `GetDriveTypeA` |
| `USER32` | 42 | one window with a menu bar and accelerators, message pump with `MsgWaitForMultipleObjects`, mouse capture, `LoadImageA` |
| `GDI32` | 6 | `CreateDIBSection`, `BitBlt`, memory DCs |
| `ADVAPI32` | 5 | `RegOpenKeyExA`, `RegQueryValueExA`, `RegCreateKeyExA`, `RegSetValueExA`, `RegCloseKey` |
| `ole32` | 2 | `CoInitializeEx`, `CoCreateInstance` (class **(open)**) |
| `DDRAW` | 1 | `DirectDrawCreateEx` (DirectDraw 7) |
| `DSOUND` | 1 | ordinal 1 = `DirectSoundCreate` |
| `WINMM` | 3 | `timeGetTime`, `timeBeginPeriod`, `timeEndPeriod` |
| `VERSION` | 3 | file version info (of its own exe or a DLL) |
| `WSOCK32` | 9 | by ordinal: 3 `closesocket`, 4 `connect`, 6 `getsockname`, 9 `htons`, 10 `inet_addr`, 11 `inet_ntoa`, 23 `socket`, 52 `gethostbyname`, 57 `gethostname`. No `send`/`recv`/`WSAStartup`: loaded dynamically or through another layer **(open)** |
| `binkw32` | 13 | `BinkOpen`, `BinkOpenDirectSound`, `BinkSetSoundSystem`, `BinkSetSoundTrack`, `BinkSetVolume`, `BinkDoFrame`, `BinkNextFrame`, `BinkWait`, `BinkCopyToBuffer`, `BinkGetRects`, `BinkPause`, `BinkGetSummary`, `BinkClose` |
| `mss32` | 4 | `AIL_startup`, `AIL_shutdown`, `RIB_find_files_provider`, `RIB_request_interface`: Miles is used only to find and call an ASI codec (MP3), sound output is DirectSound |

Compared by name with CulturesRecomp's HLE layer (2026-11-21), the imports it does not implement yet:

- kernel32: `CreateProcessA`, `OpenProcess`, `VirtualQueryEx`, `VirtualQuery`, `VirtualLock`, `OpenFileMappingA`,
  `MapViewOfFileEx`, `OpenMutexA`, `GetSystemInfo`, `GetProcessHeap`, `GetSystemTimeAsFileTime`
- user32: `MsgWaitForMultipleObjects`; ole32: `CoInitializeEx`; winmm: `timeBeginPeriod`, `timeEndPeriod`
- mss32: `RIB_find_files_provider`, `RIB_request_interface`
- binkw32: all 13 (provided by the lifted DLL)

### Strings of interest

- Archives: `data/heroes4.h4r`, `text`, `movies`, `music`, `storm`, `storm_override`, `storm_updates`, `x2`,
  `x2_override`, `x2_updates`, `updates`, `new` (the `*_updates` and `new` archives are not installed by GOG:
  optional).
- Multiplayer: `main_menu_tcpip`, `join_game_tcpip`, `main_menu_gamespy`, `join_game_gamespy`,
  `master.gamespy.com`, `Software\GameSpy\GameSpy Arcade`.

### GOG build 76223 (six languages)

- GOG's newer installers ("win11 gog 0.2", build 76223) ship a `Heroes4.exe` of 6,885,376 bytes in German,
  English, French, Italian and Spanish: 25023's code with one change. The display mode's function (`0x8d3750`)
  starts with a jump to a cave in padding (`0x411114`) that calls `0x8d22d0(0, width, height)` and jumps back.
  The patch points stay where they were. The five files differ from each other only in padding (`recomp
  codediff`: all 62,643 functions the same); the game's texts are in its data files.
- The Polish `Heroes4.exe` (6,942,720 bytes) and `campaign_editor.exe` are compiled on their own: 58,543 of
  62,643 functions (73.5 % of the code) byte for byte the same as German 76223's, the rest with other register
  choices and stack frames, at shifted addresses. The patches' places were found by the instructions around them
  with absolute addresses and call targets masked, and checked instruction by instruction before and after.
  Two places differ in kind: the resource bar's width is at `[esp+0xc0]` (not `0xb8`), and the mage guild
  compares the width twice (`0x7629e1` for 800×600, `0x7629f9` for 1024 or 1280) where 25023 compares once.

## campaign_editor.exe

- PE32 GUI, MSVC 6, image base `0x400000`, entry `0x662F66`, no relocations, timestamp `0x3E025DBA`
  (2002-12-20). `.text` 3.11 MiB.
- 515 imports, MFC statically linked (no `MFC42.dll`): kernel32 161, user32 194, gdi32 76, comctl32 23, ole32 24,
  oleaut32 9, advapi32 14, shell32 5, comdlg32 3, winspool 3, oledlg 1, olepro32 1, winmm 1.
- About 71 named imports are missing from CulturesRecomp's desktop: MDI, property sheets, image lists and drag
  images, the clipboard, OLE storage, drag and drop and message filters, `CreatePolygonRgn`, `GetRegionData`, …

## Middleware

- `binkw32.dll`: image base `0x10000000`, imports kernel32, user32, winmm, gdi32; ~4100 MMX instructions (paths
  chosen by CPU detection). A different build than the Bink DLLs of TropicoRecomp and CulturesRecomp.
- `Mss32.dll`: Miles Sound System 6.1c, image base `0x21100000`; the same file CulturesRecomp replaces by HLE.
- `Mp3dec.asi`: Miles' MP3 decoder provider, image base `0x26F00000`, imports kernel32 and mss32; contains MMX
  and **3DNow!** code paths (CPU detection). With `cpuid` reporting no 3DNow!, only the plain and MMX paths can
  run. Lifted, its 3DNow! instructions (`pfmul`, `pfadd`, `pfsub`, `pfsubr`, `pf2id`, `femms`: 738) are traps.
- `binkw32.dll` and `Mp3dec.asi` are linked with the same CRT as CulturesRecomp's Bink builds: the same four
  unreferenced x87 error stubs and the x87 argument-load helper need hints.

## Running the game (M4–M7)

- Start-up needs Miles' RIB as plug-ins use it: `Mp3dec.asi` has no exports; its `DllMain` calls
  `RIB_provider_library_handle` and `RIB_register_interface` ("ASI codec": `PROVIDER_query_attribute`,
  `ASI_startup`, …, attributes "Name", "Input file types" …; "ASI stream": `ASI_stream_open`, `…_process`,
  `…_seek`, `…_close`, attributes and the preferences "Requested sample rate", "Requested bit width"). An
  entry is (type, name, token, subtype). The game calls `RIB_find_files_provider("ASI codec", "Input file
  types", "a.mp3", …)`; the attribute's value is a filter list ("MPEG Layer 3 audio files", "*.mp3", …). It
  requests the stream interface with tokens that are the addresses of its own variables, and preferences the
  decoder does not have ("Raw source sample rate", "Layer"), which stay unset. The undecorated
  `RIB_*_interface` functions are stdcall.
- The game throws C++ exceptions in ordinary use: a `bool` in the menus, rethrown inside a catch block (the
  CRT's own `__except` in `CallCatchBlock` catches the rethrow). The shared runtime had no exception
  support; it now dispatches SEH on the guest's chain and unwinds the lifted functions (PLAN M4). Of the
  game's functions, 162 have catch continuations outside their own code (code after a `try` that only the
  catch reaches).
- Its menus copy to the screen only what changed; nothing is presented while the screen stands still.
- The window class has a menu (`lpszMenuName`); in full screen the game sets `WS_POPUP`, sizes the window with
  `AdjustWindowRectEx(…, bMenu = TRUE)` and moves it to y = -20 so that the menu bar lies above the screen.
- Buttons react to the release; the main menu's buttons are hit on their icons, not their labels. Pop-up
  menu items are chosen by the pointer position the game polls (`GetCursorPos`).
- A save of the first scenario after one battle is 134 KB, in `games/` below the user directory (GOG's script
  names `{app}/games` as the save folder).
- Before its `ShowWindow` the game sets the main window's style to `WS_POPUP | WS_VISIBLE` with `SetWindowLongA`,
  so `ShowWindow` finds it visible already; the host window has to follow the style bit (user32 HLE).
- Map scrolling: the adventure map view's constructor sets up a timer of 50 ms (`push 50` at `0x4a3e03`) and a
  step of 25 pixels (`[view + 0x2e4]`, stored at `0x4a41f7`): about 500 pixels a second in 20 steps. The
  HeroesGL wrapper (Verok, MIT) patches both to 16 ms and 8 pixels in the English 3.0 build, whose code lies
  at the same addresses as this German one (its check addresses `0x8d3881`/`0x8d3969` hold the same
  `push 0x90000000`). Here: the opt-in setting `smooth_scroll` (patch points, `games/heroes4/patches.c`).
- Resolutions: the game knows three, in a table at `0xa87760` of {width, height, available} (16-bit
  words and a byte): 800×600, 1024×768, 1280×1024. The display-mode callback (`0x8d2b..`) marks those
  DirectDraw offers at 16 bits; start-up (`0x73d943`) maps the registry's `resolution` (a width) to one of
  them, 1024×768 for anything else. About 15 places choose a layout by the width (800, 1024 or 1280,
  e.g. the map view at `0x4a35a7`). Verok's "Resolution Mod" (2020, closed, a plug-in of his wrapper)
  offers three sizes per aspect ratio (16:10: 960×600, 1280×800, 1680×1050; 16:9: 1024×600, 1366×768,
  1920×1080; and 2560×1080) behind the game's three choices; the town and combat screens stay their size,
  centred. His wrapper's source (github.com/Nemirtingas/HeroesGL, `src/Heroes4GL`) has the mode list and
  letterboxing (`ImageAspect`) and loads plug-ins (`mods\*.mod`) that may add data packages to the game; the
  Resolution Mod is such a plug-in, its patches and art unpublished.
- Layouts by resolution: the data holds one per size and screen, named with it: `adventure.800/.1024/.1280`,
  `combat.…`, `town.…`, `menu.main.…`, `dialog.win_lose.prologue_…`, `…logo_…`, `dialog.mage_guild.*_1024/_1280`.
  The switches that pick them (width from a window's rectangle, `[+0x98] - [+0x90]`, or an argument):
  adventure map view `0x4a35a7` and `0x4a397e`, its resource bar `0x4accf4` (stack argument read again at
  `0x4ad917`, `0x4adf59`), `0x4b56f1`, a size-class helper `0x584260` (0 for 800×600, 1 for 1024×768, 2
  otherwise; three callers), a dialog's background and border `0x62b8da`, an animation `0x69fbca`, the main
  menu `0x76da14` (its art; any other width loads none and crashes) and `0x76e534` (buttons), the credits
  `0x773e80`, the town `0x8ac64a`, a clock `0x8c8f8f`, the options page `0x7ca845`, `0x7cab59`, `0x7cba73`,
  `0x7cbe5b`. Height checks: `0x4837b4`, `0x7d8591`.
- How Verok's Resolution Mod (v1.51, 2022; a 52 MB package: his wrapper `DDRAW.dll`, the plug-in
  `mods/res_mod.mod`, a version check `mods/versions.mod`, and the data package `data/res_mod.h4r`) does it:
  - It replaces the game's three resolutions instead of adding any: per aspect ratio ("Mode" in
    `config.ini`, chosen from the screen's shape when unset) three sizes take the three slots: 4:3 the
    originals, 16:10 960×600, 1280×800, 1680×1050, 16:9 1024×600, 1366×768, 1920×1080; 21:9 and 32:9 only
    the third slot (2560×1080, 3840×1080), the other two staying 4:3.
  - Its patch table for this build (selected like the wrapper's, by the two `push 0x90000000` and the "800"
    at `0xa7592c`) holds about 70 places: the constants 800/1024/1280 (and 600/768/1024) of every layout
    switch, the mode table `0xa87760`, the display-mode callback, start-up, the options page and its
    labels ("%d x %d" written into the game's three strings), each rewritten to the slot's new size;
    a few functions replaced (the size-class helper `0x584260`; a centring of fixed 800×600 pictures in
    the screen; town and combat drawing); and the layout names of the adventure screen, town, combat,
    main menu, world view and the win/lose dialogs pointed at names formatted with the size
    (`adventure.%04dx%04d`, `town.alpha.…` when "Transparent" is set, …).
  - The art: `res_mod.h4r` (H4R version 5: header, then gzip streams, a directory at the end of
    {offset, packed size, size, time, name length, name, two words}) holds 85 layer files, new layouts
    for each size: `layers.adventure.1366x0768.h4d` and so on for adventure, combat (and combat.alpha),
    town (and town.alpha), menu.main, dialog.view_world, the win/lose prologue, epilogue and logo. The
    game builds its screens from those layouts, so with them the adventure map fills the width and the
    town and combat have wide pictures. The plug-in has the game load the package beside its own
    (`0x7ff910`, the loader start-up calls for `data/new.h4r` at `0x73cfed`).
- Packages: an entry of the directory is {offset, packed size, size, time}, the name, the artist's source
  path and a second name (each a 16-bit length and the bytes), and a word (3). Entries are gzip streams,
  or empty with the second name naming the real one. `updates.h4r` repeats `layers.adventure.*` (it wins
  over `heroes4.h4r`).
- Layout files (`layers.*.h4d`): a 16-bit count of layers (41 in `adventure.1024`), then per layer a short
  header (a 16-bit count and that many small fields; the first always 1, then 8: 8-bit pixels), the name
  (16-bit length), a byte 1, the rectangle (four 32-bit numbers: left, top, right, bottom) and the pixels
  in rows (below). Verok's `adventure.1366x0768` keeps every layer of
  `adventure.1024`: all of the right panel (mini map, buttons, resource numbers, hero and town lists, day
  scroll, end turn: 23 layers) moved right by the extra width, 342; the frame's top edge `Top` widened
  from 21–631 to 21–973; the left edge `LEft` unchanged; so did `Top_Border`, `Bottom_Border` and `Bottom`.
- Layout files in detail (from the reader, `0x595880`, called for each layer by `0x58e2e0`): per layer a
  16-bit colour count, a 16-bit word (1), the palette (3 bytes a colour), the name, a type byte (0 opaque, 1
  palette index 0 transparent, 4 and more with alpha), the rectangle, one row entry per line {16-bit first
  column, 16-bit end column, 32-bit offset of its first pixel}, the pixels (palette indices), and for type 4
  and more 4-bit alpha (low nibble first) and a summary of 2 bits for each 32 pixels (0 transparent, 1 opaque,
  2 both, 3 partly transparent; the original encoder chose 3 a little more often). In memory a layer is 0x68
  bytes (name +0x04, type +0x14, rectangle +0x18, shared buffers {next, previous, data} for rows +0x28, alpha
  +0x34, summary +0x40, pixels +0x4c, palette +0x58); after reading, `0x58e2e0` sorts the layers and copies
  them into a map by name (from `0x58e467` on). Buffers are allocated with `0x8d98a0` and given with
  `0x596c50`.
- Layouts as Verok widened them (his 1366×768 versions against the originals): the adventure screen's right
  part (from the map's right frame on) moves by the extra width and the strips along the map (`Top`,
  `Top_Border`, `Bottom_Border`, `Bottom`) grow; town, combat and main menu move to the middle and get a
  background as large as the screen (type 1, transparent where the battlefield or the town is drawn); the
  intro, prologue and epilogue pictures (`dialog.win_lose.logo_*`, `prologue_*`, `epilogue_*`: a frame of four
  pieces around a film) keep the film in the middle and the pieces at the edges reach on; the world view
  (an 800×600 dialog) grows in its map. His code (beside the constants) places what the game draws itself:
  the main menu's picture (`0x599f5a`), the battlefield (`0x62c608`, `0x62ae28`, the offset passed to
  `0x5a6500`), the town's picture window (its x at `0x8b6833`: the `town_image` layer's left), the town's
  800×600 dialogs (`0x75fe3a`), the map view's offset for 800×600 things (`0x4a35a7`: `[view + 0xcc]`,
  `[view + 0xd0]`), the screen rectangle the town scales its picture by (`0x8d2e20` at five places: the
  class's), and the screen's window after a resolution change (`0x71eee0` from `0x773da0`, main menu, and
  `0x4b5630`, map).
- The main window's class has `CS_DBLCLKS` (style `0xb`); the hero screen (a portrait in the panel) and
  more open on `WM_LBUTTONDBLCLK`. The user32 HLE made double clicks only for programs with child windows,
  not on its direct path for a game's main window: no double click reached the game.
- The mage guild's dialog (`0x75f7c0`, layouts `dialog.mage_guild.<school>_1024/_1280`: a frame of four
  pieces around `Book_Pages`) is of the class's size but a dialog the game centres; Verok left it alone, and so
  must the screen rule. At `0x75fa98` it takes its 800×600 form below a width of 1024, the `_1024` layout at
  exactly 1024 and `_1280` above (`0x75faa4`): a wide 768 screen got the 1280×1024 one; it is a layout
  switch like the others.
- Right button on the adventure map: the game gets `WM_RBUTTONDOWN` at once (window procedure `0x8d1e5e`, to
  the window under the point, `vtable + 0x4c`) and `WM_RBUTTONUP` (`0x8d21f0`); the information box appears
  only after the release and stays until the next click, without a key state queried in between (only
  `GetCursorPos`). That is the game's own design, unlike its predecessors (which show it while the button is
  held): the release goes to the window under the point (`vtable + 0x50`), and the base window's handler
  (`0x7200a0`) opens the help box (`0x845290`) when the window has a help text (`+0x88`), else hands the
  release to its parent; the press (`+0x4c`, `0x720050`) is only handed on.
- The left release goes to the window under the point as a click (`0x8d1c95`, `[vtable + 0x28]`); the adventure
  map's window (vtable `0x979bc0`) then sets the hero's path there, after a drag too. A patch point hands a
  release that ends a drag on the map to `ret 4` (`0x40f610`).
- The pointer: the game draws its own (`cursor.*`, e.g. `cursor.move`, `cursor.combat.melee.*`, static
  resource references) and only polls `GetCursorPos`; no `ShowCursor` or `SetCursor`, and its window class has
  no cursor. `0x71d850` finds the window under the pointer and `0x71d8c0` takes its pointer (`0x721740`) as the
  current one (`[0xabacb8]`, shown and hidden by `0x71f5f0`; `0xa6db28` whether pointers are shown)
  **(open: where the pointer object keeps its image)**.
- The town's layouts differ by class: the picture's placeholder is `town_image` in the 1024 one and
  `Town_Image` in the 800 and 1280 ones; `Town_Name` and `Town_list` are in all three. The picture window's
  x (`0x8b6835`) is the screen's middle in a wide mode.
- The 1280 adventure layout's `Top` runs on over the right frame (0–934, the frame from 880): the strips along
  the map widen where they meet the frame, not at their end.
- Here (`games/heroes4/widescreen.c`) the game keeps its classes until it sets the mode (`0x8d3750`); the
  layout switches see the class's width again; the layouts are made wide at `0x58e467`, by rules over the
  layer names and places, the new columns continuing the strip's pattern or reflecting the picture next to
  them; a background's sides are a plain parchment, as Verok's side panels: the calmest 48×48 patch of
  the adventure screen's panel (between its printed ornaments; lines counting far more than grain), its
  brightness evened out, kept when that layout is read (before any town or battle) and laid reflected to and
  fro in each background's nearest colours, with a dark line along the middle (not palette index 0, which
  type 1 layers leave transparent); the places above are patch points.

## Open questions

See the list in [PLAN.md](PLAN.md#research-questions).

## Network game set-up

- The game's packets over DirectPlay 8: one byte (1), u32 compressed size, u32 size, zlib data; the first
  byte of the data is the packet type (0x0d: the game set-up with the map's name, conditions and the six
  slots; 0x0e and 0x0f: small state packets). The host sends with GUARANTEED | NOLOOPBACK; it does not
  answer INDICATE_CONNECT with reply data.
- A slot is taken with the small box at the right of its name field, which only slots the map leaves to
  humans have ("Banditen des Rifen-Labyrinths" has one; "Ein unbedeutender Minen-Krieg" two).
- The scenario list's first selection (the highlighted map, its description shown) does not reach the set-up:
  "Weiter" starts another map ("Banditen des Rifen-Labyrinths" instead of "Ein unbedeutender Minen-Krieg")
  until a row has been clicked. The list is the game's own code, without a Win32 control or file access;
  probably the original does the same **(open: to compare on Windows)**.
