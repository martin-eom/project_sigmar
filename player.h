#define PLAYER

#ifndef BASE
#include "base.h"
#endif

class Player {
	public:
		LinkedList<Unit*> units;
		
		Player() {};
};
