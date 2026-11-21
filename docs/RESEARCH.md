# Research

Findings about the game and its binaries, in our own words. Unknowns are marked **(open)**. Addresses may appear
here; disassembly, lifted or decompiled code never does.

## The installer (gog-de)

- Inno Setup 5.6.2 (u), title "Heroes of Might and Magic® 4 - Complete", 406 entries. All file data is in the
  external slice `…-1.bin` (1.05 GB); the `.exe` holds the setup loader and headers.
- Neither CulturesRecomp's `rc-media` (Inno 5.5.0 (u), data in the exe) nor TropicoRecomp's `rc-inno` reads it
  (M1). innoextract 1.9 lists and extracts it.
- Some entries are language-tagged (`[de-DE]`), and `goggame-1207658915.script` appears four times with OS
  constraints (Windows 8, 8.1, 10, other); only one is installed.
- The game expects its registry key `HKLM\SOFTWARE\New World Computing\Heroes of Might and Magic IV\1.0`
  (string in `Heroes4.exe`); what the installer writes there is **(open)** until `rc-media` reads the registry
  section.

## Heroes4.exe

- PE32 GUI, linker 6.0 (MSVC 6), image base `0x400000`, entry `0x8E3981`, **no relocations**.
- Sections: `.text` `0x401000` (0x56E30A bytes, 5.43 MiB), `.rdata` `0x970000` (0xD54F9), `.data` `0xA46000`
  (0x8F725 virtual, 0x48000 raw), `.rsrc` `0xAD6000`.
- Link timestamp `0x73655965` is not a plausible date (it reads as ASCII text); the build may be modified by GOG
  or its patcher **(open)**. Not packed: `.text` raw size equals its virtual size, the entry is a normal MSVC CRT
  start-up.
- MSVC RTTI is present (`.?AV…@@` type descriptors, heavy template use: `t_script_action<…>`, `t_effect<…>`):
  vtables can be found through their complete object locators.
- A linear sweep (objdump) shows only a handful of MMX/SSE/`cmov` instructions, probably data decoded as code;
  to confirm from the code map (M2).

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
  run.

## Open questions

See the list in [PLAN.md](PLAN.md#research-questions).
