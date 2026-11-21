# HoMM4Recomp – orientation for a new working session

Static recompilation of *Heroes of Might and Magic IV* (Complete: base game, The Gathering Storm, Winds of War)
into portable C on an SDL3 runtime. Third project on the toolchain of `../TropicoRecomp` and `../CulturesRecomp`;
CulturesRecomp is the template for all shared code. Start here, then read `docs/PLAN.md` (milestones,
checklists, decision log), `docs/RESEARCH.md` and `docs/TARGETS.md`.

## State (2026-11-21)

Planning: docs only. Next is **M0**: copy the shared code from CulturesRecomp at a pinned commit, rename, add
`games/heroes4/game.toml`, get CMake/Cargo/CI building. Then M1: Inno 5.6.2 (u) with an external `-1.bin` slice
in `rc-media`.

## Facts that matter

- Input: `orig/setup_heroes_of_might_and_magic_4_complete_3.0_(german)_(25023).exe` + `…-1.bin` (git-ignored).
  Neither sister project's extractor reads it yet; innoextract 1.9 does (use only as a reference / stopgap).
- `Heroes4.exe`: MSVC 6, base `0x400000`, no relocations, 5.43 MiB `.text`, RTTI present. DirectDraw 7,
  DirectSound, window-message input, Miles only for its MP3 codec (RIB), Bink lifted, WSOCK32 by ordinal, threads,
  file mappings of large `.h4r` archives.
- Expected generated C: ~350 MB for `Heroes4.exe` (scale of Cultures 1's 42 MB per 0.64 MiB): build size is the
  main technical risk.

## Conventions (same as the sister projects)

- **Share code**: one recompiler, runtime, HLE and platform layer. Game-specific behaviour goes into
  `games/heroes4/` (manifest, patches), never into a copy of shared code. Keep changes general so they can go back
  to TropicoRecomp and CulturesRecomp.
- Never commit, upload or paste game files, generated C, disassembly or decompiled code. Addresses and behaviour
  described in our own words are fine.
- Every source file starts with `SPDX-License-Identifier: MIT`; no absolute home paths.
- Docs, code and comments in English (the maintainer writes in German).
- Commits: author **and** committer `TerminShock <357108427+TerminShock@users.noreply.github.com>` (as in the
  initial commit), e.g. `GIT_COMMITTER_NAME=… GIT_COMMITTER_EMAIL=… git commit --author="TerminShock <…>"`.
  Never the local git identity. Messages describe the change only: no AI tools, models or attribution trailers
  (from M0 on, `tools/hooks/commit-msg` enforces it).
- No personal data anywhere in the repository: no real names, e-mail addresses, user names or home paths.
- Keep the docs current: tick `docs/PLAN.md`, add findings to `docs/RESEARCH.md` (unknowns **(open)**),
  decisions to the decision log, new builds to `docs/TARGETS.md` and the manifest first.
