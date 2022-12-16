# Project Sigmar
Project sigmar (the name is wip) will be a pseudo-turn-based strategy game inspired by the real-time-strategy series *Total War*.
This is a passion project. It is not intended to lead to a deep video game with beautiful graphics, a story, a campaign map or anything too complex.
The main goal is to create a basic ruleset that has some depth and balance in order to train a neural network in playing it.
You could say it is a machine-learning exercise preceded by a rather longer exercise in software development and coding.
## Modified ruleset
The turn-based element will be facilitated by having one player give orders too his units, then running the simulation for a certain time, then having the other player give orders and running the simulation for the same time (and so on).
This change to the *Total War* formula is made because presumably it will be easier for a neural network to learn.
## Tools and some credit
Since I've never worked on a complex piece of software before, the basic structure of the game is following a wonderful tutorial in basic game design with [pygame](https://www.pygame.org) found [here](http://ezide.com/games/writing-games.html).
Originally the project was written entirely in Python. However, due to major performance problems the entire project was ported to C++. The handling of input and graphics rendering is performed by [SDL2](https://www.libsdl.org/), including text rendering with [SDL_ttf](https://github.com/libsdl-org/SDL_ttf/releases).
Simple vector operations are done using the [Eigen3](https://eigen.tuxfamily.org/index.php?title=Main_Page) library.
Storing maps in json format is done using the [nlohmann/json](https://github.com/nlohmann/json) class.
## Current state
The basic architecture of the game is implemented. There is a model for having multiple players with units of soldiers, which can be placed and given orders to move on a map.
Victory/Defeat game logic is not yet present, partially because the conditions have not yet been decided upon.
The soldiers within a unit will organize in a proper formation, which they will reorder and restore when disturbed.
A version of collision physics has also been implemented and somewhat refined. They can collide with each other and bounce off of map objects.
For now it is planned to keep all soldiers as circular objects, since this massively simplifies the complexity of the collision model.
The soldiers can turn and move forwards (or sideways and backwards at low speeds).
A server-client architecture is planned, but the implementation will be reserved for much later in the development process, since it is not necessary for testing of game mechanics.
There is a map editor to create and modify maps, which can be loaded into the main game.
## (Immediate) to-do list
* main game ui and controls similar to the map editor
* zoomable and moveable map
* pathfinding
* combat (mechanics and changes to movement, connected to a "state"-variable for every soldier)
* A server-client architecture is planned, but the implementation will be reserved for much later in the development process, since it is not necessary for testing of game mechanics.
## Requirements
* C++20
* Eigen3 library
* SDL2 library
* SDL_ttf library
## Compilation
### Linux
The code is written for a graphics apllication on Windows. Subsequently there is the usage of *Windows.h* to create a console window to display debug messages.
The rest of the code should be platform independent. When compiling on Linux remember to remove the lines responsible for the console window in *server.cpp*.
All header files are in the *include* folder, so make sure to include this folder when compiling.
Additionally you might have to alter the include statements for SDL2 (*SDL.h*) and Eigen3 (*Dense*) to something like *SDL2/SDL.h* and *Eigen/Dense* in all files depending on your setup of the libraries.
### Windows
For installation on Windows an installer will be included in all releases starting with cdev0.2. The map editor, game and *maps* folder have to be within the same installation folder, because the map editor will save to *maps* and both programs will load from *maps*. It is not recommended to install into *Program Files*, despite this being the default suggested installation directory, because then the map editor would require administrator rights to be able to write into the *maps* folder.
## Controlls
### Map Editor
At any stage hold down h to view the available contols.
### Game
The controls being shown on screen can be toggled by pressing h.
