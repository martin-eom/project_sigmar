#ifndef PLAYER
#define PLAYER

#include <base.h>
#include <vector>

class Player {
	public:
		std::vector<Unit*> units;
		
		Player() {};
};

#endif