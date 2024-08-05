#ifndef PLAYER
#define PLAYER

#include <base.h>
#include <vector>

class Unit;

class Player {
	public:
		bool player1;
		std::vector<Unit*> units;
		
		Player(bool player1) {
			this->player1 = player1;
		};

		int getUnitID(Unit* unit) {
			return std::find(units.begin(), units.end(), unit) - units.begin();
		}
};

#endif