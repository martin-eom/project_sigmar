#ifndef UNIT_ORDER_PROGRESSION
#define UNIT_ORDER_PROGRESSION

#include <units.h>
#include <map.h>
#include <base.h>
#include <pathfinding.h>
#include <individual_pathfinding.h>


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

void Unit::PostCombatFormup() {
	orders.push_back(new MoveOrder(pos, rot, MOVE_FORMUP, true));
}

void Unit::NextOrderPathfinding(Order* oldOrder, Order* newOrder, Map* map) {
	std::vector<Order*> newOrders;
	double rad = ncols*(yspacing - 1);
	MapWaypoint w1(newOrder->pos, rad);
	MapWaypoint w2(oldOrder->pos, rad);
	Eigen::Matrix2d Rot;
	if(!FreePath(&w1, &w2, map)) {
		std::vector<Eigen::Vector2d> positions = findPath(&w1, &w2, map);
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
				MapWaypoint w1(soldier->pos, soldier->rad);
				MapWaypoint w2(orders.at(currentOrder+1)->pos, soldier->rad);
				if(FreePath(&w1, &w2, map)) {
					soldier->arrived = true;
					nSoldiersArrived++;
				}
			}
		}
	}
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

void Unit::AdvanceOrder(Map* map) {
	if(currentOrder == (orders.size() - 1) 
		&& (orders.at(currentOrder)->type == ORDER_ATTACK
			|| orders.at(currentOrder)->type == ORDER_TARGET))
		PostCombatFormup();
	if(orders.size() > (currentOrder +1)) {
		NextOrder(map);
		ReformUnit(this);
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

void Unit::StripTransitionOrders() {
	orders.erase(std::remove_if(orders.begin() + currentOrder, 
		orders.end(),
		[](const Order* o) {
			return o->_transition;
		}), orders.end()
	);
}

void Unit::RenewOrders(EventManager* em) {
	std::vector<Order*> newOrders(orders.begin() + currentOrder, orders.end());
	em->Post(new GiveOrdersRequest(this, newOrders));
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
}

void Unit::SoldierMovement(Map* map, double* dt) {
	for(int i = 0; i < nrows; i++) {
		for(int j = 0; j < ncols; j++) {
			Soldier* soldier = soldiers.at(i).at(j);
			if(soldier && soldier->alive) {
				soldier->debugFlag1 = false;
				soldier->debugFlag2 = false;
				soldier->debugFlag3 = false;

				//possibly advancing soldier order during combat
				int co = soldier->currentOrder;
				if(!soldier->charging && orders.at(co)->type != ORDER_ATTACK && orders.size() > (co + 1) && enemyContact) { // I don't get this line, should it be type == ORDER_ATTACK?
					Order* no = orders.at(co + 1);
					if(orders.at(co)->target && no->target) {
						Circle c1(soldier->pos, soldier->rad);
						Eigen::Vector2d nextPos = no->pos + no->rot * posInUnit.at(i).at(j);
						Circle c2(nextPos, soldier->rad);
						if(!soldier->arrived && FreePath(&c1, &c2, map)) {
							soldier->arrived = true;
							if(soldier->currentOrder == currentOrder)
								nSoldiersArrived++;
						}
					}
				}

				//check if need to do indiv pathfinding, but only do this every second or so!
				soldier->IndivPathProgression(map);

				//physics step
				if(soldier->placed && soldier->alive) {
					TimeStep(soldier, *dt);
				}

				//advancing soldier order
				if(soldier->alive && soldier->arrived) {
					if(soldier->currentOrder < currentOrder) {
						SoldierNextOrder(soldier, posInUnit.at(i).at(j));
					}
				}
			}
		}
	}

}

#endif