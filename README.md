# Project Sigmar
Project sigmar (the name is wip) is a pseudo-turn-based strategy game inspired by the real-time-strategy series *Total War*.
This is a passion project. It is not intended to lead to a deep video game with beautiful graphics, a story, a campaign map or anything too complex.
The main goal is to create a basic ruleset that has some depth and balance in order to train a neural network in playing it.
You could say it is a machine-learning exercise preceded by a rather longer exercise in software development and coding.
## Modified ruleset
The turn-based element will be facilitated by having one player give orders to his units, then running the simulation for a certain time, then having the other player give orders and running the simulation for the same time (and so on).
During their first turn each player has to deploy all their units within their own deployment zone.
Victory is achieved in a death-match style by defeating all enemy soldiers.
## Tools and some credit
The basic structure of the game is following a wonderful tutorial in basic game design with [pygame](https://www.pygame.org) found [here](http://ezide.com/games/writing-games.html).
Originally the project was written entirely in Python. However, due to major performance problems the entire project was ported to C++. The handling of input and graphics rendering is performed by [SDL2](https://www.libsdl.org/), including text rendering with [SDL_ttf](https://github.com/libsdl-org/SDL_ttf/releases).
Simple vector operations are done using the [Eigen3](https://eigen.tuxfamily.org/index.php?title=Main_Page) library.
Storing maps in json format is done using the [nlohmann/json](https://github.com/nlohmann/json) class.
Some elements are parallelized using OpenMP.
## Current state
The game can be run in a hot seat mode right now. There was a server-client architecture during the Python-phase of development and it is planned to return in the C++ version.

The game rules are essentially fully implemented. The soldiers within a unit will try to keep a proper formation. They are subject to collision physics with other soldiers and map objects. All soldiers are treated as circular objects. 
Pathfinding is implemented for units. Given a well designed map they will choose the shortest path between any 2 points while not running directly into map objects.
The shortest paths are computed during map creation using the [Floyd-Warshall algorithm](https://en.wikipedia.org/wiki/Floyd%E2%80%93Warshall_algorithm).
There is a map editor to create and modify maps, which can be loaded into the main game. Additionally the soldier classes, units and army compositions can be freely customized from the config files. Custom pixelart can easily be added if you can draw it.

Melee and ranged combat systems inspired by those of the Total War games (Melee Attack/Defense stats, unit traits) are implemented. Units can be ordered to hold a point, which will only make them attack enemies if they get within range, or they can be ordered to attack a unit.

The soldiers are animated with simple self-made pixelart. There is a single drawn map available.

At the moment the game runs at its smooth 30fps with over 2000 soldiers on an AMD Ryzen 5 3600.

## major to-do list
* some more refactoring
* server-client architecture
* ai to play against

## Requirements
* CMake 3.21+
* A C++20 compiler (GCC/Clang on Linux; MinGW-w64 for cross-compiling the Windows build from Linux)
* [vcpkg](https://github.com/microsoft/vcpkg) in manifest mode. Set the `VCPKG_ROOT` environment variable to your vcpkg checkout.
* An OpenMP-capable compiler/runtime (resolved separately via `find_package(OpenMP)`, not through vcpkg)
* [NSIS](https://nsis.sourceforge.io/) (`makensis`), only needed if you want to build the Windows installer
## Compilation
The project builds via CMake.

### Linux (untested)
```
export VCPKG_ROOT=/path/to/vcpkg
cmake --preset linux-release
cmake --build --preset linux-release
```

### Windows (cross-compiled from Linux)
Windows binaries are produced by cross-compiling with MinGW-w64:
```
export VCPKG_ROOT=/path/to/vcpkg
cmake --preset mingw-release
cmake --build --preset mingw-release
```
*(Building natively on Windows with MSVC via Visual Studio or VS Code should also work unmodified against the same `CMakePresets.json`/`vcpkg.json`, but only the MinGW cross-compile route above has been tested.)*

The game and map editor have to be run in the directory that has `maps/`, `config/`, `textures/`, and `VeraMono.ttf`.


### Packaging
[CPack](https://cmake.org/cmake/help/latest/module/CPack.html) turns either build into a distributable bundle:
```
cd build/mingw-release && cpack -G NSIS   # Windows installer (.exe)
cd build/linux-release && cpack -G TGZ    # Linux archive (.tar.gz)
```

## Controls
For both the map editor and game the controls being shown on screen can be toggled by pressing h.
