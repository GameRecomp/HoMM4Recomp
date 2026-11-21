# HoMM4Recomp

An unofficial static recompilation of **Heroes of Might and Magic IV** (New World Computing, 2002) with
*The Gathering Storm* and *Winds of War*: the game's 32-bit x86 Windows code is translated ahead of time into
portable C (or straight into native code or WebAssembly), and runs on a small runtime that reimplements the parts
of Windows, DirectX and the middleware it uses, on top of SDL3. No emulator, no Wine.

Targets: **Windows, Linux, Android and the browser**, built from your own GOG copy — the same toolchain and
platforms as [TropicoRecomp](https://github.com/GameRecomp/TropicoRecomp) and
[CulturesRecomp](https://github.com/GameRecomp/CulturesRecomp).

> Unofficial fan project. Not affiliated with Ubisoft, 3DO, New World Computing or GOG.
> This repository contains **no game code and no game data**, and it never will: you bring the game.

## State

Planning. See [docs/PLAN.md](docs/PLAN.md) for the milestones, [docs/TARGETS.md](docs/TARGETS.md) for the
supported installer and [docs/RESEARCH.md](docs/RESEARCH.md) for what is known about the game's binaries.

## License

[MIT](LICENSE) applies to the code in this repository. The game itself is the property of its rights holders.
Buy it (for example on GOG) to use this project.
