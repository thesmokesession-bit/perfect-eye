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

1. Copy the `PPSA64007` folder to `/data/homebrew/` on the console over FTP.
2. Put your **Perfect Dark (USA) (Rev 1)** ROM in that folder. Any file name, `.z64`, `.n64` or
   `.v64`. On the first start the game copies it to `data/pd.ntsc-final.z64` in the right byte
   order (md5 `e03b088b6ac9e0080440efed07c1e40f`).
3. Optional: a **GoldenEye 007 (USA)** ROM in the same folder is moved to `added-content/` and
   converted into GoldenEye's levels and missions on the next start.
4. Let ShadowMountPlus register the title, then start Perfect Dark from the home screen.

Everything the game writes (settings in `pd.ini`, saves, the log) stays in that folder.

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
  Hosting works without opening ports; forwarding **UDP 27100** to the PS5 makes joins direct.

## When something goes wrong

`pd.log` in the title folder has the game's log and, after a crash, the faulting address and the
code addresses on the stack (match them to `eboot.bin.elf` with `llvm-symbolizer`).

## Not on the PS5

- Check for Updates: replace `eboot.bin` over FTP instead.
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
tools/upload.sh                  # PS5_HOST=<ip>:<port> to /data/homebrew over FTP
```

`patches/dabsmod.patch` is applied to Dab's Mod without committing, so the build keeps
upstream's commit hash, which is what online play compares.

## Licences

This port's own files are GPL-3.0-or-later, like the Rush 2 PS5 port they come from. Dab's Mod
and the PC port are MIT. RADV, PS5_Vulkan and the payload SDK fork: see their repositories.
Perfect Dark is © Rare / Microsoft. This project ships no game data and is not affiliated with
Rare, Microsoft, Sony or the authors above.
