# Targets

This file lists every input the project accepts, pinned by SHA-256. It must stay in sync with
`games/<id>/game.toml` (from M0), which becomes the machine-readable source.

The recompiler refuses any binary whose hash is not listed. A new build (another language, another store,
another patch level) is added here and to the manifest first. Only then is it lifted.

## Installers and media

| Game      | Variant  | File | Size (bytes) | SHA-256 |
|-----------|----------|------|-------------:|---------|
| `heroes4` | `gog-de` | `setup_heroes_of_might_and_magic_4_complete_3.0_(german)_(25023).exe`   |     807112 | `9609dd49f48a3fb325e9383053a1607e59e247db3f9ab8df484113b16dcff4bf` |
| `heroes4` | `gog-de` | `setup_heroes_of_might_and_magic_4_complete_3.0_(german)_(25023)-1.bin` | 1054210930 | `f2a371b683d1af3ec1c8b7605deee168b18859b842f8b888b396900d891fadbb` |

The GOG installer is Inno Setup **5.6.2 (unicode)** with its file data in one external slice (`-1.bin`).
GOG product id `1207658915`, "Heroes of Might and Magic® 4 - Complete" (base game, *The Gathering Storm*,
*Winds of War*).

Wanted later (not yet available, add by hash): the GOG installers of the other languages, retail CDs with the
official 3.0 patch.

## Binaries

| File | Variant | Size (bytes) | SHA-256 | Treatment |
|------|---------|-------------:|---------|-----------|
| `Heroes4.exe`         | `gog-de` | 7877375 | `b6debc1c20065cdcef9524c856d1dba793d34368c786ba9c42eed7fa70a54a3b` | lift (main) |
| `campaign_editor.exe` | `gog-de` | 4513792 | `5deed9f176cc039c2465bceb4f6b84fa3d023848831834987d545284a2ced56f` | lift (tool, M11) |
| `binkw32.dll`         | `gog-de` |  261120 | `545f402be018bc905c03305322072e194d77b1a5ec20b62c34cdbc4c81c961fa` | lift |
| `Mss32.dll`           | `gog-de` |  348160 | `2a515199cc7a2e1ccd8d756efdaab07cedd5c3052309ef1c7499c2560e2097bd` | hle (same build as CulturesRecomp's Cultures 1 DE / Cultures 2) |
| `Mp3dec.asi`          | `gog-de` |  125952 | `0a6db88a5fad28168c1dac700473c5709b17802c33e36b2668c3300c1e580fad` | lift (proposal, M6) |

`binkw32.dll`, `Mss32.dll` and `Mp3dec.asi` are installed twice, in the game folder and in `Data/`, with the same
hash.

Skipped by name (installer and store helpers, not pinned): `__redist/ISI/scriptinterpreter.exe`,
`goggame-1207658915.*`, `app/webcache.zip`, `commonappdata/GOG.com/…`, the installer's `tmp/` files.

## Data files (not pinned, read by the game)

`Data/heroes4.h4r` (568 MiB), `text.h4r`, `music.h4r`, `movies.h4r`, `Storm.h4r`, `storm_override.h4r`, `x2.h4r`,
`x2_override.h4r`, `updates.h4r`, `*.aop`, `default.pbg`, `high_scores.dat`; `maps/` (`.h4c` maps and
`Contest Maps`); manuals (`*.pdf`, `Readme.txt`, `Liesmich*.txt`).
