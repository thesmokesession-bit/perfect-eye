# Update notes

## 1.1.0 (unreleased)

- **GoldenEye's Xbox 360 version:** with your own copy in the GoldenEye tile's `added-content/`,
  GoldenEye uses its HD levels, characters, guns, props, skies and folder screens.
- The game unpacks the Xbox 360 package itself, as it comes off the console, the first time it
  starts. No tools needed on your computer.
- **PS5 Settings on the GoldenEye tile:** EXTRA on GoldenEye's mode select has a third row,
  PS5 Settings (gyro, adaptive triggers, display mode, texture pack and the rest). Done goes back
  to the folder.
- The GoldenEye tile's online pages and settings page play GoldenEye's folder music instead of
  Perfect Dark's menu music.

## 1.0.0 (2026-10-19)

The first public release: Perfect Dark and GoldenEye 007 as two native PS5 tiles.

### GoldenEye's own tile
- GoldenEye 007 is its own game on its own tile (`PPSA64008`). It opens straight into
  GoldenEye's gun barrel intro in about a quarter of a second, with none of Perfect Dark's
  opening, menus or music on the way.
- The Perfect Dark tile no longer shows GoldenEye rows on its menu.
- **ONLINE** on GoldenEye's mode select opens the online lobby in GoldenEye's mode. Back returns
  to the GoldenEye folder.
- The online pages on the GoldenEye tile are drawn in GoldenEye's colours: its dark desk behind,
  folder-tab title bars, GOLDENEYE 007 where the Perfect Dark logo was.
- Online in GoldenEye's mode starts on a GoldenEye map, and Player Setup lists GoldenEye's
  characters first (Bond first), Perfect Dark's after.
- The GoldenEye tile makes its own save the first time, without asking.
- HD textures for GoldenEye: GoldenEye 007 HD (intermissionfb), converted for this game. See the
  README.

### Fixes
- **Player Setup online no longer closes the game.** Opening the page read part of the game's own
  code as text, which the PS5 doesn't allow.
- Each tile was opening the other's game; fixed.
- No more crash or hang when going online: name lookup, secure connections and the lobby's
  sockets all work on the PS5 now, and backing out of the lobby no longer closes the game.
- The lobby answers faster (connections are kept open between requests).
- No more crash at Choose Your Reality or on the first frame (the keyboard and on-screen keyboard
  libraries are loaded only when the console has them).
- Fewer stutters: the audio queue is deeper and the shader cache is saved in the background.
- The home screen pictures update with each install (`ps5-push.command` refreshes the console's
  own copy of the art).

### Tiles
- The box covers as the home screen tiles, upscaled to 4K.

### Under the hood
- Crash reports and a watchdog log (`pd-watch.log`) in the title folder: frames, memory, and
  where the game was when it stopped.
- `--dump-ge-raw` writes GoldenEye's converted textures so an emulator pack can be converted for
  this game.

## Before 1.0.0 (2026-10-17 to 2026-10-18)

- The port itself: toolchain, platform layer, packaging, the display plane at the TV's
  resolution, the splash screen.
- 120 fps, Quality and Performance modes, the first-start PS5 settings page.
- Gyro aiming, adaptive triggers, light bars, PlayStation button names.
- 5.1 surround out of the PS5, rest mode.
- Online: IPv4 fallback, room codes, kill cam, hit markers, kill feed, Match Summary, bots in
  online rooms.
- Get Mods and Maps, Retro mode, Xbox 360 Look, faster texture pack loading, a prebuilt shader
  cache.
- `ps5-push.command` and `ps5-sync-saves.command`.
- The in-game updater and the workflow that follows Dab's Mod's dev builds.
