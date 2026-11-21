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

## Open questions

See the list in [PLAN.md](PLAN.md#research-questions).
