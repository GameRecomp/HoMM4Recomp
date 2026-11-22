# HoMM4Recomp – orientation for a new working session

Static recompilation of *Heroes of Might and Magic IV* (Complete: base game, The Gathering Storm, Winds of War)
into portable C on an SDL3 runtime. Third project on the toolchain of `../TropicoRecomp` and `../CulturesRecomp`;
CulturesRecomp is the template for all shared code. Start here, then read `docs/PLAN.md` (milestones,
checklists, decision log), `docs/RESEARCH.md` and `docs/TARGETS.md`.

## State (2026-11-22)

M0–M4 done, M5–M13 in progress: the game boots, plays its intro, runs its menus, starts scenarios and the first
map of all three campaigns, a town opens, battles are won (automatic and manual combat), the computer players take
their turn, a game is saved and loaded, the resolution switches to 1280×1024. A network game of two programs starts
and passes the turn (`tools/run-netgame heroes4 netgame`). The browser converter turns the installer into a
WebAssembly build that plays in the page. Scripted runs: `tools/run-scenario heroes4 <name>` (repeat exactly; see
`games/heroes4/scenarios`). Next: a full network game, the editor's tools, sound by ear, CI for Windows/Android,
widescreen (see `docs/PLAN.md`).

```sh
CMAKE_PREFIX_PATH=<your SDL3 prefix> cmake --preset dev && cmake --build --preset dev    # unpacks, lifts, builds
build/dev/games/heroes4/homm4-heroes4 --game-dir orig/heroes4/gog-de
tools/run-scenario heroes4 firstturn                  # headless, virtual clock, compares calls and a screenshot
RC_TRACE=imports|messages  RC_TRACE_SEH=1  RC_RIB_DUMP=1   # traces
```

## Facts that matter

- Input: `orig/setup_heroes_of_might_and_magic_4_complete_3.0_(german)_(25023).exe` + `…-1.bin` (git-ignored).
  `recomp extract` unpacks it (Inno 5.6.2, external slice, GOG Galaxy parts, the GOG script's registry keys)
  byte-identical to innoextract 1.9.
- `Heroes4.exe`: MSVC 6, base `0x400000`, no relocations, 5.43 MiB `.text`, RTTI present. DirectDraw 7,
  DirectSound, window-message input, Miles only for its MP3 codec (RIB), Bink lifted, WSOCK32 by ordinal, threads,
  file mappings of large `.h4r` archives.
- Generated C: 361 MB for `Heroes4.exe`; a full build takes 6½ minutes on 8 cores. Each code file declares only
  the functions it names, so a re-lift recompiles only what changed (a change to `rc/cpu.h` still rebuilds all).
- Exceptions: the runtime dispatches SEH on the guest's `fs:[0]` chain and unwinds lifted functions with a check
  after every call (`rc_unwind_claim`); the native and wasm back ends do not have that yet (M12).

## Conventions (same as the sister projects)

- **Share code**: one recompiler, runtime, HLE and platform layer. Game-specific behaviour goes into
  `games/heroes4/` (manifest, patches), never into a copy of shared code. Keep changes general so they can go back
  to TropicoRecomp and CulturesRecomp.
- Never commit, upload or paste game files, generated C, disassembly or decompiled code. Addresses and behaviour
  described in our own words are fine.
- Every source file starts with `SPDX-License-Identifier: MIT`; no absolute home paths.
- Docs, code and comments in English (the maintainer writes in German).
- Commits in logical steps; author **and** committer `TerminShock <357108427+TerminShock@users.noreply.github.com>` (as in the
  initial commit), e.g. `GIT_COMMITTER_NAME=… GIT_COMMITTER_EMAIL=… git commit --author="TerminShock <…>"`.
  Never the local git identity. Messages describe the change only: no AI tools, models or attribution trailers
  (from M0 on, `tools/hooks/commit-msg` enforces it).
- No personal data anywhere in the repository: no real names, e-mail addresses, user names or home paths.
- Keep the docs current: tick `docs/PLAN.md`, add findings to `docs/RESEARCH.md` (unknowns **(open)**),
  decisions to the decision log, new builds to `docs/TARGETS.md` and the manifest first.
