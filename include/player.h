#ifndef PLAYER
#define PLAYER

#include <base.h>
#include <vector>
#include <map>

class Unit;
class Model;

enum PlayerTypes {
	PLAYER_LOCAL,
	PLAYER_SIMPLEAI
};


class Player {
	public:
		bool player1;
		int type;
		std::vector<Unit*> units;
		Model* model;
		inline static std::map<std::string, int> playerTypeDict {
			{"local", PLAYER_LOCAL},
			{"simpleai", PLAYER_SIMPLEAI}
		};
		
		Player(bool player1, int type = PLAYER_LOCAL) {
			this->player1 = player1;
			this->type = type;
		};

		int getUnitID(Unit* unit) {
			return std::find(units.begin(), units.end(), unit) - units.begin();
		}
};

/*std::map<std::string, int> Player::playerTypeDict {
	playerTypeDict["local"] = PLAYER_LOCAL,
	playerTypeDict["simpleai"] = PLAYER_SIMPLEAI
};*/


#endif