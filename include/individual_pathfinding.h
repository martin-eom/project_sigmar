#ifndef INDIVIDUAL_PATHFINDING
#define INDIVIDUAL_PATHFINDING

#include <soldiers.h>

void Soldier::IndivPathProgression(Map* map) {
	indivPathTimer.decrement();
	if(indivPathTimer.done()) {
		Circle c1(pos, rad);
		Circle c2(NoIPFPosTarget(this), rad);
		if(indivPath.empty()) {
			if(!FreePath(&c1, &c2, map)) {
				//do indiv pathfinding
				indivPath = findPath(&c2, &c1, map);
			}
		}
		else {
			if(FreePath(&c1, &c2, map)) {
				indivPath.clear();
			}
			else {
				Circle c3(indivPath.at(0), rad);
				if(FreePath(&c1, &c3, map)) {
					if(indivPath.size() > 1) {
						Circle c4(indivPath.at(1), rad);
						if(FreePath(&c1, &c4, map))
							std::erase(indivPath, indivPath.at(0));
					}
					else {
						if((c3.pos - c1.pos).norm() < rad)
							std::erase(indivPath, indivPath.at(0));
					}
				}
				else {
					//redo indiv pathfinding
					indivPath = findPath(&c2, &c1, map);
				}
			}
		}
		indivPathTimer.reset();
	}
}

#endif