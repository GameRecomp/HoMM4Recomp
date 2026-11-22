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
| `heroes4` | `gog-de-76223` | `setup_heroes_of_might_and_magic_4_complete_3.0_win11_gog_0.2_(german)_(76223).exe` | 828336 | `f39c83b6ca36dbb608bcba05fde55303d4f460dc2c855763caa7c6e366ce03ad` |
| `heroes4` | `gog-de-76223` | `setup_heroes_of_might_and_magic_4_complete_3.0_win11_gog_0.2_(german)_(76223)-1.bin` | 1155287336 | `cf469bde6cf70f5cc5bb78eff4bd7beb38ad433705d29294ac3e7a4fdfb5a1e9` |
| `heroes4` | `gog-en-76223` | `setup_heroes_of_might_and_magic_4_complete_3.0_win11_gog_0.2_(76223).exe` | 827904 | `933fa0285e9f08044e86946054a42236e0fe34a8a23709b6ba7101f84cd92f0e` |
| `heroes4` | `gog-en-76223` | `setup_heroes_of_might_and_magic_4_complete_3.0_win11_gog_0.2_(76223)-1.bin` | 1132493510 | `bbf6fc0aca8dfb500e05881c47f69136732659769cfb5d71e58292aec6ec2970` |
| `heroes4` | `gog-fr-76223` | `setup_heroes_of_might_and_magic_4_complete_3.0_win11_gog_0.2_(french)_(76223).exe` | 828160 | `6753f43835d89cb88b082d53f3775c6efe38096de61911235681d7d34203c16e` |
| `heroes4` | `gog-fr-76223` | `setup_heroes_of_might_and_magic_4_complete_3.0_win11_gog_0.2_(french)_(76223)-1.bin` | 1181855524 | `987018f59fb596d63e596e2dc7489554e0807340f7b5db1c6c2e28a7e7abe7e2` |
| `heroes4` | `gog-it-76223` | `setup_heroes_of_might_and_magic_4_complete_3.0_win11_gog_0.2_(italian)_(76223).exe` | 828528 | `48e67359c0691b8ef0f21d8399141a0cb507709d1ad2f5901c50ec8a493b6103` |
| `heroes4` | `gog-it-76223` | `setup_heroes_of_might_and_magic_4_complete_3.0_win11_gog_0.2_(italian)_(76223)-1.bin` | 1152921337 | `2af67f4a627ab421f313cc4dac03e53fc0ea447c9d4cdd83ded8de91c03eb1a1` |
| `heroes4` | `gog-es-76223` | `setup_heroes_of_might_and_magic_4_complete_3.0_win11_gog_0.2_(spanish)_(76223).exe` | 828392 | `019a4c33a56a4d396635f7b080c04fd67348a24f4c4b8a3c7ac314f646ea2a2f` |
| `heroes4` | `gog-es-76223` | `setup_heroes_of_might_and_magic_4_complete_3.0_win11_gog_0.2_(spanish)_(76223)-1.bin` | 1149043163 | `2774dea2099c2fc067b435c0fbe05a9ef87870e399f3aa94a8556582537c1c2b` |
| `heroes4` | `gog-pl-76223` | `setup_heroes_of_might_and_magic_4_complete_3.0_win11_gog_0.2_(polish)_(76223).exe` | 827672 | `76bf84ec53d31e6940b679bc2d9bf5d0cf0758c14b17043b8d1412bc41855ec3` |
| `heroes4` | `gog-pl-76223` | `setup_heroes_of_might_and_magic_4_complete_3.0_win11_gog_0.2_(polish)_(76223)-1.bin` | 1162734184 | `cbb23b8217c7074916c78d6304b308a0475f065fa973d684fdb20cdb640dd936` |

The GOG installer is Inno Setup **5.6.2 (unicode)** with its file data in one external slice (`-1.bin`).
GOG product id `1207658915`, "Heroes of Might and Magic® 4 - Complete" (base game, *The Gathering Storm*,
*Winds of War*).

GOG's newer installers, "win11 gog 0.2" (build 76223), come in six languages (English, German, French,
Italian, Spanish, Polish), each an installer and its slice of the same kind.

Wanted later (not yet available, add by hash): retail CDs with the official 3.0 patch.

## Binaries

| File | Variant | Size (bytes) | SHA-256 | Treatment |
|------|---------|-------------:|---------|-----------|
| `Heroes4.exe`         | `gog-de` | 7877375 | `b6debc1c20065cdcef9524c856d1dba793d34368c786ba9c42eed7fa70a54a3b` | lift (main) |
| `campaign_editor.exe` | `gog-de` | 4513792 | `5deed9f176cc039c2465bceb4f6b84fa3d023848831834987d545284a2ced56f` | lift (tool, M11) |
| `binkw32.dll`         | `gog-de` |  261120 | `545f402be018bc905c03305322072e194d77b1a5ec20b62c34cdbc4c81c961fa` | lift |
| `Mss32.dll`           | `gog-de` |  348160 | `2a515199cc7a2e1ccd8d756efdaab07cedd5c3052309ef1c7499c2560e2097bd` | hle (same build as CulturesRecomp's Cultures 1 DE / Cultures 2) |
| `Mp3dec.asi`          | `gog-de` |  125952 | `0a6db88a5fad28168c1dac700473c5709b17802c33e36b2668c3300c1e580fad` | lift (proposal, M6) |
| `Heroes4.exe`         | `gog-de-76223` | 6885376 | `b171788633a63c38e28016b379ae97e033c880d259a5103fd006adcfcd4e4c57` | lift (main) |
| `Heroes4.exe`         | `gog-en-76223` | 6885376 | `b7c2f6caeeba640b1c5f1d812b172938bb4dd4ec62e34ec93ad827bd1ffaec87` | alias of `gog-de-76223`'s |
| `Heroes4.exe`         | `gog-fr-76223` | 6885376 | `a6f8d14af6add9e630addd37df29a7803f074f88fbc58a744817ae860cfa5701` | alias of `gog-de-76223`'s |
| `Heroes4.exe`         | `gog-it-76223` | 6885376 | `6a3c699d2d3d6e96d8f6fdc9c17fde6e03126a3073d1b7a754cb51bd5b32a023` | alias of `gog-de-76223`'s |
| `Heroes4.exe`         | `gog-es-76223` | 6885376 | `bfc9eb079fbc3c74c879a9e90486479970f2a7cd450a6adad31ca12372809e93` | alias of `gog-de-76223`'s |
| `Heroes4.exe`         | `gog-pl-76223` | 6942720 | `27a85a2a136c353918c85dfaa9c4c2c8c8be9241a420022930d7aaaff1768190` | lift (main) |
| `campaign_editor.exe` | `gog-pl-76223` | 4505600 | `4c137a6d45d6b10b266e7e8ed27df32119c8d205d83ec53219049b2d4ae01e19` | lift (tool) |

The 76223 builds share `binkw32.dll`, `Mss32.dll` and `Mp3dec.asi` with 25023, and except the Polish one its
`campaign_editor.exe`. Their `Heroes4.exe` is 25023's code with one change by GOG: the display mode's
function starts with a jump to a cave in padding that calls another function first. The English, French,
Italian and Spanish files differ from the German one only in bytes no code reaches (`recomp codediff`: every
function the same); the manifest lists them as **aliases**, which run the German one's lifted code. The Polish
build is compiled on its own (73.5 % of the code the same, at other addresses) and lifted on its own; the
patches have its addresses in `games/heroes4/builds.c`.

`binkw32.dll`, `Mss32.dll` and `Mp3dec.asi` are installed twice, in the game folder and in `Data/`, with the same
hash.

Skipped by name (installer and store helpers, not pinned): `__redist/ISI/scriptinterpreter.exe`,
`goggame-1207658915.*`, `app/webcache.zip`, `commonappdata/GOG.com/…`, the installer's `tmp/` files.

## Data files (not pinned, read by the game)

`Data/heroes4.h4r` (568 MiB), `text.h4r`, `music.h4r`, `movies.h4r`, `Storm.h4r`, `storm_override.h4r`, `x2.h4r`,
`x2_override.h4r`, `updates.h4r`, `*.aop`, `default.pbg`, `high_scores.dat`; `maps/` (`.h4c` maps and
`Contest Maps`); manuals (`*.pdf`, `Readme.txt`, `Liesmich*.txt`).
