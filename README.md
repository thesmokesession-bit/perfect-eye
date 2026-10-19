# Perfect Eye: Perfect Dark and GoldenEye on the PS5

Perfect Eye is [Dab's Mod](https://github.com/DabDavis/perfect-dark-dabs-mod) (a fork of the
[Perfect Dark PC port](https://github.com/perfect-dark-pc-port/perfect_dark)) built as native
PS5 homebrew, with GoldenEye 007 on a home screen tile of its own.

- **Two tiles, two games.** `PPSA64007` is Perfect Dark. `PPSA64008` is GoldenEye 007: it opens
  straight into GoldenEye's own intro and folder screens, and nothing of Perfect Dark shows on it.
- **Native, not emulated.** The game runs as an installed PS5 title and draws with Dab's Mod's
  Vulkan renderer on [Mihawk's RADV port](https://github.com/mihawk-99/PS5_Vulkan), at the TV's
  own resolution (3840x2160 on a 4K set).
- **Online with PC players.** Dab's Mod's lobby: rooms, room codes, co-op, counter-op, kill cam,
  match summaries. A PS5 and a PC on the same build play together.
- **HD for both games:** Perfect Dark's Xbox 360 remaster (from your own copy of the release),
  HD texture packs for Perfect Dark, and GoldenEye 007 HD (see [HD textures](#hd-textures)).

You need your own ROMs. This project ships no game data.

Build: Dab's Mod `3ee8ed0cd` (dev channel). Online play needs everyone on the same build: PC
players use the Dab's Mod dev build of that commit.

---

## What you need

- A PS5 running a homebrew setup that installs titles from `/data/homebrew` (for example
  etaHEN with ShadowMountPlus) and an FTP server on the console.
- **Perfect Dark (USA) (Rev 1)** ROM: `.z64`, `.n64` or `.v64`, any file name
  (md5 of the `.z64`: `e03b088b6ac9e0080440efed07c1e40f`).
- For the GoldenEye tile, also a **GoldenEye 007 (USA)** ROM. The GoldenEye tile runs on the
  Perfect Dark engine, so its folder needs both ROMs.
- Optional, for GoldenEye in HD: your own copy of **GoldenEye 007 for the Xbox 360** (the XBLA
  version). It goes on top of the GoldenEye ROM and gives GoldenEye that version's HD levels,
  characters, guns, props, skies and menus.
- Optional, for Perfect Dark in HD: your own copy of the **Xbox 360 (XBLA) release** of Perfect
  Dark. It goes on top of the N64 ROM, which the game still needs, and gives Perfect Dark the Xbox
  360 version's HD models and textures (see [HD textures](#hd-textures)).

## Installing

1. Download `PerfectEye-<version>.zip` from the
   [latest release](https://github.com/thesmokesession-bit/perfect-eye/releases/latest) and unzip it.
2. Put your ROMs in the title folders:
   - `PPSA64007/` gets the Perfect Dark ROM, and optionally the GoldenEye ROM.
   - `PPSA64008/` gets both ROMs.
3. Send the folders to `/data/homebrew/` on the console:
   - **From a Mac or Linux PC:** double-click `ps5-push.command` (the first time, right-click
     and Open). It asks for the PS5's address and FTP port once (the ones your FTP server
     shows) and sends everything.
   - **Any FTP client works too:** copy `PPSA64007` and `PPSA64008` into `/data/homebrew/`.
4. Let your installer register the titles, then start Perfect Dark or GoldenEye from the home
   screen.

The first start of each tile puts the ROMs in place (and converts GoldenEye's levels on the
GoldenEye tile, which takes a minute). Everything the game writes (settings in `pd.ini`, saves,
`pd.log`) stays in its title folder.

If the home screen pictures don't show the box art after an update, restart the PS5: the home
screen keeps its own copy of each tile's art, which `ps5-push.command` refreshes.

## Updating

- **From the game (1.2.0 and later):** when a new version is out, an **Update Available** box comes
  up on the first screen (Choose Your Reality on Perfect Dark, the mode select on GoldenEye).
  **Update Now** downloads it, installs it and restarts the game. On Perfect Dark, **Check for
  Updates** on the Perfect Menu does the same by hand.
- **From 1.0.0 or 1.1.0:** update from your computer once. Their in-game updater can freeze the
  game.
- **From your computer:** download the new release and run `ps5-push.command` again. It sends the
  game's own files and anything new, and never touches your saves or settings.

## The GoldenEye tile

- Opens straight into GoldenEye's gun barrel intro and folder screens.
- Solo missions, multiplayer and the Cinema are GoldenEye's own, converted from your ROM.
- **ONLINE** is the right-hand tab on GoldenEye's mode select. It opens the online lobby in
  GoldenEye's mode, drawn in GoldenEye's colours, with GoldenEye's maps, weapons and characters
  first (Bond first). Perfect Dark's characters are still there after them. Back returns to the
  GoldenEye folder.

## HD textures

A texture pack goes in `texture-packs/` in the title folder, as a folder. Pick or turn off packs
under **Options → Extended Options → Texture Packs**.

- **Perfect Dark, the Xbox 360 remaster:** your own copy of the 2010 Xbox Live Arcade release
  of Perfect Dark gives the game that release's HD models, textures, rooms, font and explosions.
  Put its package in `PPSA64007/added-content/`: the file as it comes off an Xbox 360 (a
  42-character name like `4287CC74...`), or an archive named `Perfect Dark XBLA.7z` or
  `Perfect Dark XBLA.zip`. Then turn on **Xbox 360 Look** on the PS5 Settings page (Extended
  Options → PS5). Not included here: it's game data, like the ROMs.
- **Perfect Dark, the N64 look:** **PD Forever Plus HD** (Retro Foundry). The Texture Packs
  page's **Community Packs** button downloads it straight to the console. Its **Ultimate** and
  **XBLA** packs go on top of the Xbox 360 release, so they need that package too.
- **GoldenEye, the Xbox 360 version:** put your own copy in `PPSA64008/added-content/`, any of:
  the package as it comes off an Xbox 360 (the `584108A9` folder and everything in it; the game
  unpacks it into `added-content/GoldenEye 007 XBLA/` the first time it starts, about a minute),
  the folder it unpacks to (the one holding `default.xex` and `files/`), or a `.7z` or `.zip` of
  that folder. Levels, characters, guns, props, skies and the folder screens then come from it.
- **GoldenEye, texture pack:** download `PerfectEye-GoldenEye-HD-Textures.zip` from the release and unzip it into
  `PPSA64008/texture-packs/`. It is [GoldenEye 007 HD](https://github.com/GhostlyDark/GoldenEye-007-HD)
  by intermissionfb (font textures by GhostlyDark), converted to this game's texture numbers and
  capped at 1024 pixels. It covers the levels and weapons (nearly every surface in the missions
  and multiplayer maps); explosions, smoke, menus and the gun barrel intro keep their original look.

## What the PS5 build adds

- **120 fps** in Performance mode on a TV that takes the PS5's 120 Hz output. On a 60 Hz TV it
  runs at 60.
- **Quality and Performance:** Quality draws at the TV's full resolution at 60 fps; Performance
  draws at two thirds of it, sharpened with FSR 1. `Video.PS5DisplayMode` (0 or 1).
- **A settings page at the first start** with the display mode, texture pack, gyro and adaptive
  triggers. It is **Extended Options → PS5** afterwards on the Perfect Dark tile, and
  **EXTRA → PS5 Settings** on the GoldenEye tile.
- **Gyro aiming** on every DualSense, in split-screen too. `Input.PS5Gyro=0` turns it off,
  `Input.PS5GyroSpeed` (0.1 to 5) sets the speed.
- **Adaptive triggers:** R2 feels like the gun in hand (a clean break for pistols, a heavy pull
  for shotguns and launchers, a buzz while an automatic fires); L2 has a light stop for aiming.
  `Input.PS5AdaptiveTriggers=0` turns them off.
- **Light bars** in each player's colour, or the team's colour in a team match.
- **PlayStation button names** (CROSS, CIRCLE, L1, R2, OPTIONS) in the controls menus.
- **5.1 surround** with Sound Mode set to Surround; **Headphone Surround**
  (`Audio.SurroundOutput=3`) for a stereo headset.
- **Retro mode:** the original 4:3 picture through a CRT filter. `Video.RetroMode=1`.
- **Rest mode:** the game picks up where it was when the console wakes. An online match does not
  survive it.
- **Get Mods and Maps** under **Perfect Dark Mods**: downloads mods and Combat Simulator maps
  straight into `mods/`. The list is `assets/mod-catalogue.txt`, read from the `ps5-dev` release.
- **Xbox 360 Look** on the PS5 Settings page: the Xbox Live Arcade release's models, textures and
  rooms, once your own copy of its package is in `added-content/` (see [HD textures](#hd-textures)).
- **Faster HD texture loading:** four decode threads and a 1.5 GB store of decoded textures
  (`Mod.TexturePackCacheMB`).
- **A shader cache** built on a PC (`vulkan-shaders.bin`), so the first visit to each level
  compiles fewer shaders.
- **Online without IPv6:** falls back to IPv4 by itself.

## Multiplayer

- **Split-screen:** connect up to four controllers and start a Combat Simulator match.
- **Online:** **Online Game** on the Perfect Menu (or **ONLINE** on the GoldenEye tile). Sign in
  with a name and PIN (a new name makes the account), then Browse Rooms or Create Room. Every room
  shows an eight-character **room code**: friends pick **Join by Code** and type it.
  - **Kill cam** (`Mod.KillCam`), **hit markers** (`Mod.HitMarkers`), **kill feed**
    (`Mod.KillFeed`) and a **Match Summary** at the end (`Mod.MatchSummary`).
  - **Bots Fill Online Rooms** (PS5 Settings, `Net.BotBackfill`): simulants take the seats no
    player took.
  - Hosting works without opening ports; forwarding **UDP 27100** to the PS5 makes joins direct.
  - The lobby server is Dab's Mod's own. Nothing to pay for or set up.

## From a Mac or Linux PC

Two tools sit beside the title folders. Each asks for the PS5's address and FTP port once and
remembers it.

- **`ps5-push.command`** installs and updates the title folders over the network: the first time
  everything, after that the game's own files and anything new (a ROM, a texture pack). It never
  touches the console's saves or settings.
- **`ps5-sync-saves.command`** keeps the saves the same on the PS5 and a PC copy of the same build.
  Close the game on both first. A save changed in one place goes everywhere; changed in both, you
  pick; everything replaced is kept in the PC save folder's `.ps5-sync/backups`.

## Known issues

- **USB keyboard and mouse:** the current jailbreaks don't let a title load the PS5's keyboard
  and mouse libraries, so they don't work yet. Text fields use the game's own on-screen keyboard.
- **120 Hz** depends on the TV and on the PS5's own 120 Hz output setting. If either is off the
  game runs at 60.
- **HDR** is not supported: the display layer only offers an 8-bit SDR surface. The PS5 already
  shows an SDR picture correctly on an HDR TV.
- **The Create and PS buttons** belong to the system and don't reach the game. Use the Create
  button to record video.

## When something goes wrong

`pd.log` in the title folder has the game's log. `pd-watch.log` beside it has the watchdog's
heartbeat and, after a crash, the faulting address and the code addresses on the stack (match
them to `eboot.bin.elf` from the release with `llvm-symbolizer`). Open an issue with both files
and what you pressed.

## Building

Linux with Docker, Python 3 (Pillow, etcpak) and git:

```sh
tools/setup-source.sh            # Dab's Mod + patches/dabsmod.patch, Mihawk's RADV/SDK, shaderc
docker build -t pd-ps5-build docker
./build.sh radv                  # once: RADV, the SDK fork, the title converter
./build.sh shaderc               # once: static shaderc for the console
PS5_UPDATE_REPO=https://github.com/thesmokesession-bit/perfect-eye/releases ./build.sh configure
./build.sh
tools/check-imports.sh           # every import comes from a usable system module
tools/package.sh --release       # dist/PPSA64007, dist/PPSA64008 and the release zip
tools/upload.sh                  # dist/ to /data/homebrew over FTP (tools/ps5-push.command)
```

`patches/dabsmod.patch` is applied to Dab's Mod without committing, so the build keeps
upstream's commit hash, which is what online play compares. The
[Follow Dab's Mod](.github/workflows/follow-dabs-mod.yml) workflow builds a new `ps5-dev`
release whenever Dab's Mod's dev build moves on.

### Converting a GoldenEye emulator texture pack

```sh
./pd.x86_64 --dump-ge-raw /tmp/geraw          # a desktop build with GoldenEye converted
python3 src-upstream/tools/texpack/riceconvert.py --dump /tmp/geraw \
    --pack /path/to/GOLDENEYE --out /tmp/gehd   # a Rice/GLideN64 pack's folder
# then put /tmp/gehd/textures/*.png in texture-packs/<name>/ge-n64/
```

## Update notes

See [CHANGELOG.md](CHANGELOG.md).

## Credits

- **Dab's Mod:** DabDavis and its contributors.
- **Perfect Dark PC port:** fgsfds and the Perfect Dark decompilation team.
- **RADV on the PS5 and the payload SDK fork:** Mihawk.
- **PS5 toolchain:** Marice's Rush 2 PS5 port.
- **GoldenEye 007 HD:** intermissionfb (textures), GhostlyDark (font textures).
- **PD Forever Plus HD:** Retro Foundry.
- **PS5 port:** Jean.

## Licences

This port's own files are GPL-3.0-or-later, like the Rush 2 PS5 port they come from. Dab's Mod
and the PC port are MIT. RADV, PS5_Vulkan and the payload SDK fork: see their repositories.
Texture packs belong to their authors. Perfect Dark and GoldenEye 007 are © Rare / Microsoft /
Nintendo. The tile art is the games' box covers. This project ships no game data and is not
affiliated with Rare, Microsoft, Nintendo, Sony, MGM or the authors above.
