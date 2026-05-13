# MCLEClient (Minecraft Legacy Edition)

[![Discord](https://img.shields.io/badge/Discord-Join%20Server-5865F2?logo=discord&logoColor=white)](https://discord.gg/u9f67jaWyP)

This project is based on [MCLCE/MinecraftConsoles](https://github.com/MCLCE/MinecraftConsoles) With Modified Networking to allow Custom Authentication, Relayed servers & Relayed Player Worlds. And various Improvments & Changes. 

## Download 
### Client
Windows users can Look at our [Release Tags](https://github.com/mclemp/MCLEClient/tags) And choose what version to download.
### Server
If you're looking for Dedicated Server software, Look at its [Release Tags](https://github.com/mclemp/MCLEClient/tags) And choose what version to download.

## Platform Support

- **Windows**: Supported for building and running the project
- **macOS / Linux**: All Builds should work on Wine 8.0 Above & Crossover, This is tested by maintainers but not frequently.
- **Android**: VIA x86 EMULATORS (like GameNative) ONLY! Does run but has stability / frametime pacing issues.
- **iOS**: No current support
- **All Consoles**: Console support remains in the code, but maintainers are not currently verifying console functionality.

## Features

- Relayed Multiplayer & Discovery
- Support for keyboard and mouse input
- Fullscreen mode support (toggle using F11)
- Splitscreen Multiplayer support (connect to servers, etc)
- Added a high-resolution timer path on Windows for smoother high-FPS gameplay timing
- Device's screen resolution will be used as the game resolution instead of using a fixed resolution (1920x1080)

## Controls (Keyboard & Mouse)

- **Movement**: `W` `A` `S` `D`
- **Jump / Fly (Up)**: `Space`
- **Sneak / Fly (Down)**: `Shift` (Hold)
- **Sprint**: `Ctrl` (Hold) or Double-tap `W`
- **Inventory**: `E`
- **Chat**: `T`
- **Drop Item**: `Q`
- **Crafting**: `C` To open, `Q` and `E` to move through tabs (cycles Left/Right)
- **Change to 3rd, 2nd or 1st person**: `F5`
- **Fullscreen**: `F11`
- **Pause Menu**: `Esc`
- **Attack / Destroy**: `Left Click`
- **Use / Place**: `Right Click`
- **Select Item**: `Mouse Wheel` or keys `1` to `9`
- **Accept or Decline Tutorial hints**: `Enter` to accept and `B` to decline
- **Game Info (Player list and Host Options)**: `TAB`
- **Toggle HUD**: `F1`
- **Toggle F3 Menu**: `F3`

## Contributors
Would you like to contribute to this project? Please read our [Contributor's Guide](CONTRIBUTING.md) before doing so! This document includes our current goals, standards for inclusions, rules, and more.

## Client Launch Arguments

| Argument           | Description                                                                                         |
|--------------------|-----------------------------------------------------------------------------------------------------|
| `-fullscreen`      | Launches the game in Fullscreen mode                                                                |

Example:
```
Minecraft.Client.exe -fullscreen
```

## Multiplayer
Multiplayer is relayed to allow anyone to easily host!

- Hosting a multiplayer world automatically advertises it on the in-game Join Game menu to all players
- Other players on the same version can discover the session from the in-game Join Game menu
- Split-screen players can join in, even in Multiplayer!

## Credits
Special thanks to [neoLegacy](https://github.com/neoStudiosLCE/neoLegacy) for allowing us to use there version for Higher TU's
Special thanks to [DrPerky](https://github.com/DrPerkyLegit) for making the Backend used for Authentication, Leaderboards, & Relaying, And the original Networking.

## Build & Run

1. Install [Visual Studio 2022](https://aka.ms/vs/17/release/vs_community.exe) or [newer](https://visualstudio.microsoft.com/downloads/).
2. Clone the repository with submodules. If you don't, you will get a build error!
    - `git clone --recurse-submodules https://github.com/MCLCE/MinecraftConsoles.git` 
3. Open the project folder from Visual Studio.
4. Set the build configuration to **Windows64 - Debug** (Release is also ok but missing some debug features), then build and run.

### CMake (Windows x64)

```powershell
cmake --preset windows64
cmake --build --preset windows64-debug --target Minecraft.Client
```

For more information, see [COMPILE.md](COMPILE.md).

## Star History

<a href="https://www.star-history.com/?repos=mclemp%2FMCLEClient-Archive%2Cmclemp%2FMCLEClient&type=date&legend=top-left">
 <picture>
   <source media="(prefers-color-scheme: dark)" srcset="https://api.star-history.com/chart?repos=mclemp/MCLEClient-Archive%2Cmclemp/MCLEClient&type=date&theme=dark&legend=top-left" />
   <source media="(prefers-color-scheme: light)" srcset="https://api.star-history.com/chart?repos=mclemp/MCLEClient-Archive%2Cmclemp/MCLEClient&type=date&legend=top-left" />
   <img alt="Star History Chart" src="https://api.star-history.com/chart?repos=mclemp/MCLEClient-Archive%2Cmclemp/MCLEClient&type=date&legend=top-left" />
 </picture>
</a>