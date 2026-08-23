#ifndef UNIT_FUNCTIONS
#define UNIT_FUNCTIONS

#include <units.h>
#include <map.h>
#include <base.h>
#include <pathfinding.h>
//#include <soldier_functions.h>
#include <physics.h>


bool Unit::CurrentOrderCompleted() {
	if(placed) {
		Order* currentOrder = orders.at(this->currentOrder);
		switch(currentOrder->type) {
		case ORDER_ATTACK:
			return (dynamic_cast<AttackOrder*>(currentOrder)->target->nLiveSoldiers) <= 0; break;
		case ORDER_TARGET:
			return (dynamic_cast<TargetOrder*>(currentOrder)->target->nLiveSoldiers) <= 0; break;
		case ORDER_MOVE: {
			debug("Checking order completion...");
			MoveOrder* mo = dynamic_cast<MoveOrder*>(currentOrder);
			if(mo->moveType == MOVE_FORMUP) {
				return nSoldiersArrived >= 0.9*nLiveSoldiers; break;
			}
			else {
				return nSoldiersArrived > 0; break;
			}
		}
		default:
			return false; break;
		}
	}
}

void Unit::AdvanceOrder(Map* map) {
	if(currentOrder == (orders.size() - 1) 
		&& (orders.at(currentOrder)->type == ORDER_ATTACK
			|| orders.at(currentOrder)->type == ORDER_TARGET))
		PostCombatFormup();
	if(orders.size() > (currentOrder +1)) {
		NextOrder(map);
		Reform();
		MoveTarget();
	}
}

void Unit::NextOrder(Map* map) {
	Order* o = orders.at(currentOrder);
	Order* no = orders.at(currentOrder + 1);
	//bool needsPathFinding = false;
	if(!(o->_transition) && !(no->_transition))
		NextOrderPathfinding(o, no, map);

	currentOrder++;
	nSoldiersArrived = 0;

	//Order* no = orders.at(currentOrder);
	if(o->type == ORDER_ATTACK) {
		//std::erase(dynamic_cast<AttackOrder*>(o)->target->targetedBy, this);
		if(!no->target || (no->type == ORDER_ATTACK && no->target != o->target))
			enemyContact = false;
	}
}

void Unit::RenewOrders(EventManager* em) {
	std::vector<Order*> newOrders(orders.begin() + currentOrder, orders.end());
	std::cout << "GiveOrdersRequest with " << newOrders.size() << " orders.\n";
	GiveOrdersRequest gor(this, newOrders, true);
	//em->Post(new GiveOrdersRequest(this, newOrders));
	em->Post(&gor);
}

void Unit::DeleteObsoleteOrder() {
	if(orders.size() > 1) {
		orders.erase(orders.begin());
		currentOrder--;
		std::vector<std::vector<Soldier*>> soldiers = this->soldiers;
		for(auto row : soldiers) {
			for(auto soldier: row) {
				if(soldier->alive)
					soldier->currentOrder--;
				if(soldier->currentOrder == 0 && soldier->placed && soldier->alive) {
					nSoldiersOnFirstOrder++;
				}
			}
		}
		debug("Deleted first order.");
	}
}

void Unit::StripTransitionOrders() {
	orders.erase(std::remove_if(orders.begin() + currentOrder, 
		orders.end(),
		[](const Order* o) {
			return o->_transition;
		}), orders.end()
	);
}

void Unit::MoveTarget() {
	std::vector<std::vector<Soldier*>>* soldiers = &(this->soldiers);
	std::vector<std::vector<Eigen::Vector2d>>* posInUnit = &(this->posInUnit);
	for(int i = 0; i < nrows; i++) {
		for(int j = 0; j < ncols; j++) {
			Soldier* soldier = soldiers->at(i).at(j);
			if(soldier->placed && soldier->alive) {	//change to something like soldier->alive
				Order* o = orders.at(soldier->currentOrder);
				if(o->type == ORDER_MOVE ||true) {
					//MoveOrder* mo = dynamic_cast<MoveOrder*>(o);
					//soldier->posTarget = mo->pos + mo->rot * posInUnit->at(i).at(j);
					soldier->posTarget = o->pos + o->rot * posInUnit->at(i).at(j);
					soldier->rotTarget = o->rot;
					soldier->angleTarget = o->angleTarget;
				}
			}
		}
	}
	Order* o = orders.at(currentOrder);
	if(o->type == ORDER_MOVE || true) {
		//MoveOrder* mo = dynamic_cast<MoveOrder*>(o);
		posTarget = o->pos;
		this->rot = o->rot;
		rotTarget = o->rot;
	}
}

void Unit::NextOrderPathfinding(Order* oldOrder, Order* newOrder, Map* map) {
	std::vector<Order*> newOrders;
	double rad = ncols*(yspacing - 1);
	MapWaypoint w1(newOrder->pos, rad);
	MapWaypoint w2(oldOrder->pos, rad);
	Eigen::Matrix2d Rot;
	if(!FreePath(&w1, &w2, map)) {
		std::cout << "Unit::NextOrderPathfinding findPath start...\n";
		std::vector<Eigen::Vector2d> positions = findPath(&w1, &w2, map, NULL);
		std::cout << "Unit::NextOrderPathfinidng findPath done.\n";
		for(int npos = 1; npos < positions.size(); npos++) {
			Eigen::Vector2d diff = positions.at(npos) - positions.at(npos - 1);
			double d = diff.norm();
			double cos = diff.coeff(0)/d;
			double sin = diff.coeff(1)/d;
			if(d > 0)
				Rot << cos, -sin, sin, cos;
			else
				Rot << 1, 0, 0, 1;
			if(newOrder->type == ORDER_ATTACK && npos == positions.size() - 1) {
				int movetype = MOVE_FORMUP;
				if(enemyContact || true)
					movetype = MOVE_PASSINGTHROUGH;
				newOrders.push_back(new MoveOrder(positions.at(npos-1), Rot, movetype, true, true, newOrder->target));
				newOrders.at(newOrders.size() - 1)->setCombat();
				newOrder->rot = Rot;
			}
			else if(newOrder->type == ORDER_ATTACK) {
				newOrders.push_back(new MoveOrder(positions.at(npos-1), Rot, MOVE_PASSINGTHROUGH, true, true, newOrder->target));
				newOrders.at(newOrders.size()-1)->setCombat();
			}
			else
				newOrders.push_back(new MoveOrder(positions.at(npos-1), Rot, MOVE_PASSINGTHROUGH, true, true));
		}
	}
	orders.insert(orders.begin() + currentOrder + 1, newOrders.begin(), newOrders.end());
	// THE FOLLOWING IS IN THE WRONG PLACE
	// let soldiers on currentOrder instant-complete the first new order if they have los on the second
	for(auto row: soldiers) {
		for(auto soldier: row) {
			if(soldier->currentOrder == currentOrder && currentOrder < orders.size() - 1) {
				Eigen::Vector2d nextPos = orders.at(currentOrder+1)->pos;
				MapWaypoint w1(soldier->pos, soldier->rad);
				MapWaypoint w2(nextPos, soldier->rad);
				if((nextPos - soldier->pos).norm() <= 50 && FreePath(&w1, &w2, map)) {
					soldier->arrived = true;
					nSoldiersArrived++;
				}
			}
		}
	}
}

bool Unit::UseOrderTarget(Map* map) {
	Order* current = orders.at(currentOrder);
	if(current->type == ORDER_TARGET && current->target->nLiveSoldiers > 0 && (current->target->pos - pos).norm() < range) {
		Circle c1 = Circle(pos, OnSpotUnitRectangle(this).hw*0.7);
		Circle c2 = Circle(current->target->pos, OnSpotUnitRectangle(current->target).hw*0.7);
		if(FreePath(&c1, &c2, map, true)) {
			rangedTarget = current->target;
			return true;
		}
	}
	return false;
}

void Unit::UpdateTargetPath(Map* map, EventManager* em) {
	targetUpdateTimer.decrement();
	if(targetUpdateTimer.done()) {
		if(orders.at(currentOrder)->type == ORDER_ATTACK && enemyContact) {
			bool ranAway = CheckIfTargetHasRunAway();
			if(ranAway)
				ResetCharging();
			posTarget = orders.at(currentOrder)->target->pos;// this should happen every step
			MoveTarget();
		}
		else {
			//redo pathfinding to target here!
			double rad = ncols * (yspacing - 1);
			Unit* target = orders.at(currentOrder)->target;
			Circle w1(pos, rad);
			Circle w2(target->pos, rad);
			bool need_order_renewal = false;
			if(FreePath(&w1, &w2, map)) {
				posTarget = target->pos;
				orders.at(currentOrder)->pos = target->pos;
				MoveTarget();
				need_order_renewal = true;
			}
			if(!need_order_renewal) {
				StripTransitionOrders();
				RenewOrders(em);
			}

			// if they are on the attack order and DO NOT HAVE LINE OF SIGHT they need to get a temporary waypoint and a give orders event
			// remove all transition orders between current order and attack order
			//unit->orders.erase(unit->orders.begin() + unit->currentOrder, unit->orders.begin() + attackOrder);
			// do orderpathfinding between current order and attack order
			//unit->NextOrderPathfinding(unit->orders.at(unit->currentOrder), unit->orders.at(unit->currentOrder + 1), map);
		}
		targetUpdateTimer.reset();
	}
	else if(orders.at(currentOrder)->type == ORDER_ATTACK) {
		Eigen::Vector2d newPosTarget = dynamic_cast<AttackOrder*>(orders.at(currentOrder))->target->pos;
		//posTarget = newPosTarget;
		orders.at(currentOrder)->pos = newPosTarget;
		MoveTarget();
	}
}

bool Unit::CheckIfTargetHasRunAway() {
	bool ranAway = true;
	for(auto row: soldiers) {
		for(auto soldier: row) {
			if(soldier->currentOrder == currentOrder) {
				// soldier->enemiesInMeleeRange is a priority queue that can only be accessed from one side, so we have to copy it into a vector and then restore it
				// getting delete-copy of pripority queue
				std::vector<SoldierNeighbourContainer> enemiesCopy;
				while(!soldier->enemiesInMeleeRange.empty()) {
					enemiesCopy.push_back(soldier->enemiesInMeleeRange.top());
					soldier->enemiesInMeleeRange.pop();
				}
				// searching through copy to find target in range
				for(auto enemy: enemiesCopy) {
					if(enemy.soldier->unit == orders.at(currentOrder)->target) {
						ranAway = false;
						break;
					}
				}
				// restoring priority queue
				for(auto enemy: enemiesCopy) {
					soldier->enemiesInMeleeRange.push(enemy);
				}
			}
		}
	}
	return ranAway;
}

void Unit::ResetCharging() {
	for(auto row: soldiers) {
		for(auto soldier: row) {
			if(soldier->currentOrder == currentOrder) {
				soldier->charging = true;
				soldier->chargeTimer.reset();
			}
		}
	}
	enemyContact = false;
}

void Unit::PostCombatFormup() {
	orders.push_back(new MoveOrder(pos, rot, MOVE_FORMUP, true));
}

void Unit::FindRangedTarget(std::vector<Player*> players) {
	std::vector<UnitDistance> inRange;
	Eigen::Matrix2d rangedCone;
	rangedCone << std::cos(0.5*M_PI*rangedAngle), -std::sin(0.5*M_PI*rangedAngle), 
		std::sin(0.5*M_PI*rangedAngle), std::cos(0.5*M_PI*rangedAngle);
	for(auto player2 : players) {
		if(player2 != player) {
			for(auto unit2 : player2->units) {
				if(unit2->placed) {
					Circle circ(unit2->pos, 0);
					if(ConeCircleCollision(pos, rot, rangedCone, range, &circ)
						&& (pos - unit2->pos).norm() < range) {
						inRange.push_back(UnitDistance(this, unit2));
					}
				}
			}
		}
	}
	std::sort(inRange.begin(), inRange.end(), compareUnitDistance);
	if(!inRange.empty()) {
		rangedTarget = inRange.at(0).unit;
	}
	else {
		rangedTarget = NULL;
	}
}

void Unit::SoldierMovement(Map* map, double* dt) {//, double* time1, double* time2, double* time3, double* time4, double* timePass1, double* timePass2, double* timePass3) {
	for(int i = 0; i < nrows; i++) {
		for(int j = 0; j < ncols; j++) {
			Soldier* soldier = soldiers.at(i).at(j);
			if(soldier && soldier->alive) {
				soldier->debugFlag1 = false;
				soldier->debugFlag2 = false;
				soldier->debugFlag3 = false;

				//possibly advancing soldier order during combat
				int co = soldier->currentOrder;
				//auto start = std::chrono::system_clock::now();
				//auto end = std::chrono::system_clock::now();
				//start = std::chrono::system_clock::now();
				if(!soldier->charging && orders.at(co)->type != ORDER_ATTACK && orders.size() > (co + 1) && enemyContact) {
					Order* no = orders.at(co + 1);
					if(orders.at(co)->target && no->target) {
						Circle c1(soldier->pos, soldier->rad);
						Eigen::Vector2d nextPos = no->pos + no->rot * posInUnit.at(i).at(j);
						Circle c2(nextPos, soldier->rad);
						if(!soldier->arrived) {
							//if(FreePath(&c1,&c2,map)) {
							if((nextPos - soldier->pos).norm() <= 50 && FreePath(&c1,&c2,map)) {
								soldier->arrived = true;
								if(soldier->currentOrder == currentOrder)
									nSoldiersArrived++;
							}
						}
					}
				}
				//end = std::chrono::system_clock::now();
				//if(time1)
				//	*time1 += std::chrono::duration<double>(end - start).count();

				//start = std::chrono::system_clock::now();
				//check if need to do indiv pathfinding, but only do this every second or so!
				soldier->IndivPathProgression(map);//, timePass1, timePass2, timePass3);
				//end = std::chrono::system_clock::now();
				//if(time2)
				//	*time2 += std::chrono::duration<double>(end - start).count();

				//start = std::chrono::system_clock::now();
				//physics step
				if(soldier->placed && soldier->alive) {
					TimeStep(soldier, *dt);
				}
				//end = std::chrono::system_clock::now();
				//if(time3)
				//	*time3 += std::chrono::duration<double>(end - start).count();

				//start = std::chrono::system_clock::now();
				//advancing soldier order
				if(soldier->alive && soldier->arrived) {
					if(soldier->currentOrder < currentOrder) {
						SoldierNextOrder(soldier, posInUnit.at(i).at(j));
					}
				}
				//end = std::chrono::system_clock::now();
			}
		}
	}

}

#endif