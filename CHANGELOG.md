# Update notes

## 1.3.2 (2026-11-02)

- **Chat with a controller.** Triangle opens the chat on the Briefing Room (the Main Menu Chat in the
  bottom right) and in a room's Game Lobby, in both Perfect Dark and GoldenEye. The PS5's own
  keyboard comes up, and what you type there is sent when you press OK. The Game Lobby's Chat row
  opens the same full-length line. Before, the Briefing Room's chat only opened with Space or T on
  a keyboard.
- **PS5 button names on the online pages.** The hints at the bottom of the online pages name the
  DualSense's buttons (Cross, Circle, Triangle, Create, L2) instead of a PC's.
- **GoldenEye's watch on the shoulder buttons.** While the watch is up, L1 turns to the page on the
  left and R1 to the page on the right, and L2 is OK, as R2 is. Before, R1 turned left, L2 turned
  right and L1 moved down a row. Out of the watch the buttons do what they did.

## 1.3.1 (2026-11-02)

- **GoldenEye's watch says the right thing for LOOK UP/DOWN.** It had UPRIGHT and REVERSE the wrong
  way round: it said REVERSE when the look was upright. It now names what the look does.
- **GoldenEye's watch settings stay after a reboot.** The options page (LOOK UP/DOWN, the screen
  setting, auto-aim and the rest) and the MUSIC and FX sliders are saved to pd.ini when the watch
  closes, and come back at the next mission. Before, they went back to the defaults every time
  the game started.
- **Finer stick settings.** The four Stick Scale sliders on the Analog Stick Settings page move in
  steps of 0.05 instead of 0.1 and show the number, so you can see you're on 0.40 or 0.65. The
  range is the same as before.

## 1.3.0 (2026-11-02)

- **GoldenEye's cheats.** Beat a mission's target time on its difficulty (or a harder one) and
  you win its cheat, as on the cartridge: Paintball Mode, Invincibility, DK Mode, Turbo Mode,
  Tiny Bond, Fast and Slow Animation, Enemy Rockets, Infinite Ammo, All Guns, the paired guns,
  the Silver and Gold PP7 and the rest, twenty in all. Turn them on and off under **EXTRA →
  Cheats** on the GoldenEye tile; the mission report says **New cheat available!** when you win
  one. Times don't count while a cheat is on. Invisibility gives Perfect Dark's cloaking device,
  the nearest thing the engine has. Offline only.
- **Check for Updates on the GoldenEye tile:** EXTRA, then Check for Updates.
- **The PS5 Settings page has Analog Stick Settings and GoldenEye: Locked Progression**, so the
  GoldenEye tile can change both without editing pd.ini. Locked Progression opens GoldenEye's
  missions one at a time, as on the cartridge.
- **Sharp menu text on the GoldenEye tile:** its pop-up pages (Cheats, PS5 Settings, Check for
  Updates, online) use the HD font from Perfect Dark's Xbox 360 package, so put a copy of that
  package in `PPSA64008/added-content/` too. Without it they draw the N64's small font, which looks
  blocky at 4K. A title can't read the other title's folder, so each needs its own copy.
- **The right stick is less twitchy on the DualSense:** its sensitivity starts at 0.6 instead of
  1.0. If you had already changed yours, it stays as you set it.
- The Check for Updates page on the PS5 drops the dev and stable channel rows and says which
  version you have in plain words.
- Fewer stutters when a page of small text opens for the first time (Check for Updates, What's
  New) with the Xbox 360 look on.

## 1.2.0 (2026-10-19)

**From 1.0.0 or 1.1.0, install this one from your computer** (`ps5-push.command` or FTP): the
updater in those versions can freeze the game. From 1.2.0 on, updates install from the game.

- **Updates offered as the game starts.** When a newer release is out, an Update Available box
  comes up on the first screen: Perfect Dark's Choose Your Reality, or GoldenEye's folder.
  **Update Now** downloads it, installs it and restarts the game by itself. **Later**, **Skip This
  Version** and **Don't Show Update Notices** are there too.
- Check for Updates' **Restart Now** starts the new build straight away on the PS5, instead of
  quitting to the home screen.
- **Download and Install no longer freezes the game.** The check before it left a network
  connection to close as its thread ended, and on the PS5 that thread never finished, so the
  install waited on it forever. The updater no longer waits on a finished check, and no
  connection is closed at a thread's end any more.
- **An update installed from the game starts.** The downloaded eboot.bin had no permission to
  run, so the PS5 said "Can't start the game or app". It gets the same permissions FTP gives it.
- **The update check answers in a second or two** instead of a minute: requests share one open
  connection instead of closing one each time, which the PS5 was slow to do.

## 1.1.0 (2026-10-19)

- **GoldenEye's Xbox 360 version:** with your own copy in the GoldenEye tile's `added-content/`,
  GoldenEye uses its HD levels, characters, guns, props, skies and folder screens.
- The game unpacks the Xbox 360 package itself, as it comes off the console, the first time it
  starts. No tools needed on your computer.
- **PS5 Settings on the GoldenEye tile:** EXTRA on GoldenEye's mode select has a third row,
  PS5 Settings (gyro, adaptive triggers, display mode, texture pack and the rest). Done goes back
  to the folder.
- The GoldenEye tile's online pages and settings page play GoldenEye's folder music instead of
  Perfect Dark's menu music.
- **Check for Updates finds new releases.** Every build said it was the same version, so the game
  never offered an update. 1.0.0 offers 1.1.0, and each release after it is told apart.

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
