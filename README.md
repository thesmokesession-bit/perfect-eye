# Perfect Dark for the PlayStation 5

[Dab's Mod](https://github.com/DabDavis/perfect-dark-dabs-mod) (a fork of the
[Perfect Dark PC port](https://github.com/perfect-dark-pc-port/perfect_dark)) built as native PS5
homebrew. It runs as an installed title (`PPSA64007`), draws with Dab's Mod's Vulkan renderer on
[Mihawk's RADV port](https://github.com/mihawk-99/PS5_Vulkan), and presents at the TV's own
resolution (3840x2160 on a 4K set). The PS5 toolchain comes from
[Marice's Rush 2 PS5 port](https://github.com/Marice/rush2-ps5).

Build: Dab's Mod `3ee8ed0cd` (dev channel, patch notes 66). Online play needs everyone on the same
build: PC players use the Dab's Mod dev build of that commit.

## Installing

1. Copy the `PPSA64007` folder (and `PPSA64008` for the GoldenEye tile) to `/data/homebrew/` on
   the console over FTP.
2. Put your **Perfect Dark (USA) (Rev 1)** ROM in that folder. Any file name, `.z64`, `.n64` or
   `.v64`. On the first start the game copies it to `data/pd.ntsc-final.z64` in the right byte
   order (md5 `e03b088b6ac9e0080440efed07c1e40f`).
3. Optional: a **GoldenEye 007 (USA)** ROM in the same folder is moved to `added-content/` and
   converted into GoldenEye's levels and missions on the next start.
4. Let ShadowMountPlus register the title, then start Perfect Dark from the home screen.

Everything the game writes (settings in `pd.ini`, saves, the log) stays in that folder.

## What the PS5 build adds

- **120 fps** on a TV that takes the PS5's 120 Hz output (the title declares it in
  `param.json`), in Performance mode. On a 60 Hz TV it runs at 60.
- **Updates from the game:** **Check for Updates** on the Perfect Menu (and a notice there when one is out) fetches the newest
  PS5 build from this repo's `ps5-dev` release, which follows Dab's Mod's dev builds (see
  `.github/workflows/follow-dabs-mod.yml`). Quit and start it again from the home screen to run it.
- **Online without IPv6:** if the console cannot open a dual-stack socket, online play falls back
  to IPv4 by itself.
- **Light bars** in each player's colour (blue, red, green, pink), or the team's colour in a team
  match on one console.
- **PlayStation button names** (CROSS, CIRCLE, L1, R2, OPTIONS) in the controls menus.
- **Gyro aiming:** each DualSense's motion sensor turns the view on top of the twin-stick
  controls, in split-screen too. `Input.PS5Gyro=0` turns it off and `Input.PS5GyroSpeed`
  (0.1 to 5, default 1: the view turns as far as the controller) sets the speed.
- **Adaptive triggers:** R2 feels like the gun in hand: a clean break for pistols, a heavy pull for
  the shotgun and launchers, a long one for the sniper rifle and Farsight, a buzz while an automatic
  fires. L2 has a light stop for aiming. Every shot also rumbles. `Input.PS5AdaptiveTriggers=0`
  turns the triggers off.
- **5.1 surround:** with Sound Mode set to Surround, the game sends six channels straight to the
  PS5's audio out, which hands them to a receiver as 5.1 or mixes them for a TV or the console's 3D
  audio on headphones. **Headphone Surround** (`Audio.SurroundOutput=3`) is the game's own mix
  for a stereo headset instead.
- **Quality and Performance:** Quality draws at the TV's full resolution at 60 fps; Performance
  draws at two thirds of it, sharpened back up with FSR 1, at 120 fps on a 120 Hz TV.
  `Video.PS5DisplayMode` (0 or 1); until it is chosen, a 120 Hz TV gets Performance.
- **A settings page at the first start** with the display mode, texture pack, gyro and adaptive
  triggers in one place. It is **Extended Options → PS5** afterwards.
- **Retro mode:** the original 4:3 picture between black bars, through a CRT filter (scanlines,
  an aperture grille, a slight curve). On the PS5 Settings page, or `Video.RetroMode=1`.
- **Rest mode:** the game picks up where it was when the console wakes, rather than running the
  minutes it slept in one step. An online match does not survive it.
- **Get Mods and Maps:** a page under **Perfect Dark Mods** that downloads mods and Combat
  Simulator maps straight into `mods/`, where the game unpacks and imports them as usual. The
  list is `assets/mod-catalogue.txt` in this repo (one mod a line), which the game reads from the
  `ps5-dev` release, so a mod added there reaches every console without a new build.
- **USB keyboard and mouse:** plug them into the console for PC-style controls and mouselook,
  with the PC version's key bindings (Extended Options → Mouse and the controls pages). The
  keyboard goes through the PS5's keyboard library. The mouse library is not in the SDK, so the
  game loads it when it starts; `pd.log` says whether it found it (`input: ... mouse ready`).
- **The PS5's keyboard** opens for every text field: the online name and PIN, chat and the
  menus' keyboard.
- **A GoldenEye tile:** `PPSA64008` is the same game installed as a second title that opens
  GoldenEye as soon as it starts. Its folder needs both ROMs (Perfect Dark and GoldenEye USA).
- **Faster HD texture loading:** four decode threads instead of one, and a 1.5 GB store of
  decoded textures (`Mod.TexturePackCacheMB`) instead of 512 MB.
- **A shader cache** built on a PC (`vulkan-shaders.bin`), so the first visit to each level
  compiles fewer shaders.

## From a Mac (or any PC)

Two tools sit beside the title folders. On a Mac, double-click them (the first time, right-click
and Open); elsewhere, run them in a terminal. Each asks for the PS5's address and FTP port once
(the one the jailbreak's FTP server shows) and remembers it.

- **`ps5-push.command`** sends the title folders over the network: the first time everything,
  after that the game's own files and anything new (a ROM, a texture pack). It never touches the
  console's saves or settings.
- **`ps5-sync-saves.command`** keeps the saves (agent files, unlocks, Combat Simulator setups,
  GoldenEye's best times) the same on the PS5 and a PC copy of the same build. Close the game on
  both first. A save changed in one place goes everywhere; changed in both, the newest wins (you
  pick, in a terminal), and everything replaced is kept in the PC save folder's `.ps5-sync/backups`.
  Give it the PC copy's save folder (`save dir` in its `pd.log`) if it does not find it.

## HD textures

A texture pack goes in `texture-packs/` in the title folder, as a folder. With no pack chosen the
game uses the first one it finds, so a single pack there is on from the first start. Change or
turn it off under **Options → Extended Options → Texture Packs**. That page's **Community Packs**
can also download packs straight to the console.

For the N64 look, use **PD Forever Plus HD** (Retro Foundry). The Ultimate and XBLA packs are made
for the Xbox 360 release's models, which need that release in `added-content/`.

## Multiplayer

- **Split-screen:** connect up to four controllers and start a Combat Simulator match as on the N64.
- **Online:** **Online Game** on the main menu. Sign in with a Ghost Trials name and PIN (a new name
  makes the account), then Browse Rooms or Create Room. Co-op and counter-op missions work too.
  Every room shows an eight-character **room code** next to its name: friends pick **Join by
  Code** in the room list and type it to go straight in. The code is the lobby's own room id, so a
  PC player's room has one too, though only the PS5 build shows it.
  Hosting works without opening ports; forwarding **UDP 27100** to the PS5 makes joins direct.

## When something goes wrong

`pd.log` in the title folder has the game's log and, after a crash, the faulting address and the
code addresses on the stack (match them to `eboot.bin.elf` with `llvm-symbolizer`).

## Not on the PS5

- HDR: the display layer (Mihawk's VideoOut WSI) only offers an 8-bit SDR surface, and the
  game's art is SDR. The PS5 already shows an SDR picture correctly on an HDR TV.

- Video recording: use the controller's Create button.
- The Create and PS buttons belong to the system and do not reach the game.

## Building

Linux with Docker, Python 3 (Pillow, etcpak) and git:

```sh
tools/setup-source.sh            # Dab's Mod + patches/dabsmod.patch, Mihawk's RADV/SDK, shaderc
docker build -t pd-ps5-build docker
./build.sh radv                  # once: RADV, the SDK fork, the title converter
./build.sh shaderc               # once: static shaderc for the console
./build.sh configure
./build.sh
tools/check-imports.sh           # every import comes from a usable system module
tools/package.sh                 # dist/PPSA64007
tools/upload.sh                  # dist/ to /data/homebrew over FTP (tools/ps5-push.command)
tools/gen-shader-cache.sh <dir>  # vulkan-shaders.bin from a Linux build of the same commit
```

`patches/dabsmod.patch` is applied to Dab's Mod without committing, so the build keeps
upstream's commit hash, which is what online play compares.

## Licences

This port's own files are GPL-3.0-or-later, like the Rush 2 PS5 port they come from. Dab's Mod
and the PC port are MIT. RADV, PS5_Vulkan and the payload SDK fork: see their repositories.
Perfect Dark is © Rare / Microsoft. This project ships no game data and is not affiliated with
Rare, Microsoft, Sony or the authors above.
