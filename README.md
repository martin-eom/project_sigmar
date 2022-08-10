## Project Sigmar
Project sigmar (the name is wip) will be a pseudo-turn-based strategy game inspired by the real-time-strategy series *Total War*. This is a passion project. It is not intended to lead to a deep video game with beautiful graphics, a story, a campaign map or anything too complex. The main goal is to create a basic ruleset that has some depth and balance in order to train a neural network in playing it. You could say it is a machine-learning exercise preceded by a rather longer exercise in software development and coding.
# Modified ruleset
The turn-based element will be facilitated by having one player give orders too his units, then running the simulation for a certain time, then having the other player give orders and running the simulation for the same time (and so on). This change to the *Total War* formula is made because presumably it will be easier for a neural network to learn.
# Tools and some credit
Since I've never worked on a complex piece of software before, the basic structure of the game is following a wonderful tutorial in basic game design with [pygame](https://www.pygame.org) found [here](http://ezide.com/games/writing-games.html). So far the project is written entirely in *Python*. For easy interfacing with a network the game is running in a server-client architecture. This is implemented using Python's [twisted](https://twisted.org/) package.
# Current state
The basic architecture to have a map with players and units is implemented. Only very basic networking is implemented that allows connecting to localhost. Anything complex such as remembering a host when disconnecting and allowing reconnection or simply retrying connection on failure during the first approach are not implemented yet.
A client with a display showing the map and units and basic controls (arrow-keys left and right for switching units, clicking once for position and a second time for orientation to give orders to a unit) is implemented. Only very basic physics are implemented so far. The soldiers are treated as non-interacting, dampened harmonic oscillators with respect to their target position.
Some algorithms have been implemented to reform ranks after soldier deaths and unit rotation.
# "Immediate" to-do list
* collision detection and resolution
* pathfinding (amd properly implementing the Map object with actual obstacles)
* giving soldiers an orientation and changing their movement from any direction to forward and turning (except for very small distances)
