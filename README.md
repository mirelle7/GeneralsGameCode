# Welcome to the Generals Game Code Project

Mirelle7/GeneralsGameCode is a single person project. It's goal is to make the best console version of generals.

Also the best state of this project is being archived. Either having upstreamed all this code to any of the larger forks.
Such as GO, TSH, Vibecoded flavour of the Day. If you're truly bored please help me reach version 0 (see urbit kelvin versioning).

You can compete on any of this code.

As I'm unemployed it's imperative to include a Properly Tested NixOS image of the entire GeneralsMD psychofauna. And to not use it.
I love reading them.

Estimated cost:
1 tinybox for but for video games https://tinygrad.org/#tinybox
as many monitors 4 RTX cards support, 240hz atleast
as many input devices this motherboard and its antennas supports, 4/16 gamepads are possible. usb also supports a pretty high number for mk/b

Legi/GO/TSH/bgfx/CO/multiview and either sdl3 or gameinput support

and im not paying for it (except for my own seat ofc)

Also we should include gambling. In spectator mode I do want to be able to bet on who will when, the faster and the more correct i guess the higher the score.

score = base × (1 − time_locked / match_length) + accuracy_bonus

essentially, 3 async game modus in one

Additionally, there is a complementary project repository for fixing and improving game data and assets such as
INI scripts, GUI, AI, maps, models, textures, audio, localization. You can find it
[here](https://github.com/TheSuperHackers/GeneralsGamePatch/) and contribute to it as well.

## Project Overview

The game was originally developed using Visual Studio 6 and C++98. We've updated the code to be compatible with Visual
Studio 2022 and C++20.

The initial goal of this project is to fix critical bugs and implement improvements while maintaining compatibility with
the original *Generals* version 1.08 and *Zero Hour* version 1.04. Once we can break retail compatibility, more fixes
and features will be possible to implement.

## Current Focus and Future Plans

Here's an overview of our current focus and future plans

- Minimize the codebase. 

## Running the Game

To run *Generals* or *Zero Hour* using this project, you need to have the original *Command & Conquer: Generals and Zero Hour* game
installed. The easiest way to get it is through *Command & Conquer The Ultimate Collection*
on [Steam](https://store.steampowered.com/bundle/39394). Once the game is ready, download the latest version of the
project from [GitHub Releases](https://github.com/TheSuperHackers/GeneralsGameCode/releases), extract the necessary 
files, and follow the instructions in the [Wiki](https://github.com/TheSuperHackers/GeneralsGameCode/wiki).


## Joining the Community

You can chat and discuss the development of the project on our [Discord channel](https://www.community-outpost.com/discord) to get the latest updates,
report bugs, and contribute to the project!

## Building the Game Yourself

We provide support for building the project on Windows and Linux. For detailed build instructions, check the
[Wiki](https://github.com/TheSuperHackers/GeneralsGameCode/wiki/build_guides), which includes guides for VS6, VS2022,
Docker, CLion, and links to forks supporting additional versions.

### Quick Start

**Windows (Visual Studio 2022)**
```bash
cmake --preset win32
cmake --build build/win32 --config Release
```

**Linux (via Docker)**
```bash
./scripts/docker-build.sh              # Build using Docker
./scripts/docker-install.sh --detect # Install to your game
```

### Dependency management

The repository uses a vcpkg manifest (`vcpkg.json`). Dependency versions come from the `builtin-baseline` commit
recorded there, with per-port `overrides` when a specific version is required. Update the baseline to pick up new
versions. GitHub Actions consumes these ports through a vcpkg binary cache backed by a NuGet feed on GitHub
Packages, keyed by vcpkg's own ABI hashes, so the first CI build warms the feed and subsequent builds pull prebuilt
binaries instead of re-compiling everything. Pull requests from forks restore from the feed but cannot write to it.

### Profiling

Tracy profiling is supported in the CMake preset `win32-profile`.
Use `tracy-profiler.exe` from [Tracy v0.13.1](https://github.com/wolfpld/tracy/releases/tag/v0.13.1).
If you get an error when using Tracy, try removing `dbghelp.dll` from the game binary directory.

## Contributing

We welcome contributions to the project! If you’re interested in contributing, you need to have knowledge of C++. Join
the developer chat on Discord for more information on how to get started. Please make sure to read our
[Contributing Guidelines](CONTRIBUTING.md) before submitting a pull request. You can also check out 
the [Wiki](https://github.com/TheSuperHackers/GeneralsGameCode/wiki) for more detailed documentation.


## License & Legal Disclaimer

EA has not endorsed and does not support this product. All trademarks are the property of their respective owners.

This project is licensed under the [GPL-3.0 License](https://www.gnu.org/licenses/gpl-3.0.html), which allows you to
freely modify and distribute the source code under the terms of this license. Please see [LICENSE.md](LICENSE.md) 
for details.
