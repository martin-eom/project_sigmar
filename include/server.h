#ifndef SERVER
#define SERVER

#include <base.h>
#include <units.h>
#include <physics.h>
#include <map.h>
#include <model.h>
#include <combat_mechanics.h>
#include <unit_order_progression.h>
#include <individual_pathfinding.h>
#include <player.h>

#include <cstdio>

class Game : public Listener {
	public:
		Game(EventManager* em) : Listener(em) {};
	private:
		void Notify(Event* ev) {};
};

#endif