#define SERVER

#ifndef BASE
#include "base.h"
#endif
#ifndef UNITS
#include "units.h"
#endif
#ifndef PHYSICS
#include "physics.h"
#endif
#ifndef MAP
#include "map.h"
#endif
#ifndef MODEL
#include "model.h"
#endif
#ifndef PLAYER
#include "player.h"
#endif

#include <cstdio>

class Game : public Listener {
	public:
		Game(EventManager* em) : Listener(em) {};
	private:
		void Notify(Event* ev) {};
};