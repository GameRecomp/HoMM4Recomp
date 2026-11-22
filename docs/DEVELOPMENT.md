# HoMM4Recomp for developers

HoMM4Recomp is a **static recompilation** of *Heroes of Might and Magic IV* (New World Computing, 2002) with its
two expansions. It translates the original 32-bit x86 Windows executable ahead of time into portable C (or native
code, or WebAssembly) and runs it on a small runtime that reimplements the parts of Windows, DirectX and the
middleware the game calls, on top of SDL3. It is the third project on the toolchain of
[TropicoRecomp](https://github.com/GameRecomp/TropicoRecomp) and
[CulturesRecomp](https://github.com/GameRecomp/CulturesRecomp), and shares their recompiler, runtime, HLE and
platform code.

Every executable we support is pinned by SHA-256 in [`TARGETS.md`](TARGETS.md) and `games/heroes4/game.toml`.

## Quick start

Requirements: CMake ≥ 3.25, Ninja (the `dev` and `release` presets use it, so the build compiles on every core
without `-j`), a C11 compiler, Rust (stable), and SDL3 (with SDL3_ttf and SDL3_mixer) for a window
and sound. Building the game compiles about 360 MB of generated C: plan for several minutes on 8 cores and about
2 GB of memory per compiler process.

```sh
# Recompiler toolchain (Rust)
cargo build --release --manifest-path recomp/Cargo.toml
cargo test --manifest-path recomp/Cargo.toml

# Identify your game files
recomp/target/release/recomp identify orig/

# Unpack the GOG installer (its -1.bin beside it) → orig/heroes4/gog-de/, binaries verified
recomp/target/release/recomp extract "orig/setup_heroes_of_might_and_magic_4_complete_3.0_(german)_(25023).exe"

# Find the code and lift it (→ generated/heroes4/…, never committed); the build does this itself
recomp/target/release/recomp discover orig/heroes4/gog-de/Heroes4.exe
recomp/target/release/recomp lift orig/heroes4/gog-de

# Runtime, HLE and the game shell (C); configuring unpacks and lifts what is missing
CMAKE_PREFIX_PATH=<your SDL3 prefix> cmake --preset dev
cmake --build --preset dev
ctest --preset dev

# The game
build/dev/games/heroes4/homm4-heroes4 --game-dir orig/heroes4/gog-de
build/dev/games/heroes4/homm4-heroes4 --check --game-dir orig/heroes4/gog-de   # imports the HLE layer lacks
RC_TRACE=imports build/dev/games/heroes4/homm4-heroes4 …                         # every API call
SDL_VIDEO_DRIVER=dummy RC_CLOCK=virtual RC_INPUT=script.txt RC_SCREENSHOT=300:a.bmp RC_FRAMES=310 …   # scripted
tools/run-scenario heroes4 firstturn      # a scripted run, compared with the accepted one
tools/run-netgame heroes4 netgame         # two programs, a network game

# Windows without Windows (Docker): MinGW-w64 cross build, warnings as errors, tests under Wine
tools/check-windows
```

Put your own game files in `orig/` (git-ignored).

## Documentation

| File | Contents |
|------|----------|
| [`PLAN.md`](PLAN.md) | Goals, milestones, order of work, risks, decision log |
| [`ARCHITECTURE.md`](ARCHITECTURE.md) | Pipeline, runtime model, HLE layer, repository layout |
| [`TARGETS.md`](TARGETS.md) | Every supported installer and binary, with hashes |
| [`RESEARCH.md`](RESEARCH.md) | Findings from analysing the installer and the binaries |
| [`CONTRIBUTING.md`](../CONTRIBUTING.md) | Rules, especially regarding copyrighted material |
| [`CLAUDE.md`](../CLAUDE.md) | Short orientation for a new working session: state, conventions, next steps |
