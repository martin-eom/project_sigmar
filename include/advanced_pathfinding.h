#ifndef ADVANCED_PATHFINDING
#define ADVANCED_PATHFINDING

#include <soldiers.h>
#include <orders.h>
#include <units.h>
#include <map.h>
#include <pathfinding.h>

void OrderPathfinding(Unit* unit, Map* map, std::vector<OrderPtr> nos = std::vector<OrderPtr>(), int start_order = 0) {
	std::vector<OrderPtr> newOrders = std::vector<OrderPtr>();
	if(start_order == 0)
		start_order = unit->currentOrder + 1;
		nos = unit->orders;
	for(int i = start_order; i < nos.size(); i++) {
		debug("Pathfinding for an order started");
		OrderPtr mo = nos.at(i);
		if((nos.at(i)->type == ORDER_MOVE && nos.at(i-1)->type == ORDER_MOVE) || true) {
			Order* mo = nos.at(i).get();
			Order* pmo = nos.at(i-1).get();
			//checking if line of sight between orders
			double rad = unit->ncols*(unit->yspacing - 1);
			MapWaypoint w1 = MapWaypoint(mo->pos, rad);
			MapWaypoint w2 = MapWaypoint(pmo->pos, rad);
			Eigen::Matrix2d Rot;
			if(!FreePath(&w1, &w2, map)) {
				std::vector<Eigen::Vector2d> positions = findPath(&w1, &w2, map);
				//translating waypoints to orders
				for(int k = 1; k < positions.size(); k++) {
					Eigen::Vector2d diff = positions.at(k) - positions.at(k-1);
					double d = diff.norm();
					double cos = diff.coeff(0)/d;
					double sin = diff.coeff(1)/d;
					if(d > 0) {
						Rot << cos, -sin, sin, cos;
					}
					else {
						Rot << 1, 0, 0, 1;
					}
					if(mo->type == ORDER_ATTACK && k == positions.size() - 1) {
						int movetype = MOVE_FORMUP;
						if(unit->enemyContact)
							movetype = MOVE_PASSINGTHROUGH;
						newOrders.push_back(std::make_shared<MoveOrder>(positions.at(k-1), Rot, movetype, true, true, mo->target));
						mo->rot = Rot;
					}
					else if(mo->type == ORDER_ATTACK)
						newOrders.push_back(std::make_shared<MoveOrder>(positions.at(k-1), Rot, MOVE_PASSINGTHROUGH, true, false, mo->target));
					else
						newOrders.push_back(std::make_shared<MoveOrder>(positions.at(k-1), Rot, MOVE_PASSINGTHROUGH, true, false));
				}

			}
			else if(mo->type == ORDER_ATTACK) {
				Eigen::Vector2d diff = w1.pos - w2.pos;
				double d = diff.norm();
				double cos = diff.coeff(0)/d;
				double sin = diff.coeff(1)/d;
				Rot << cos, -sin, sin, cos;
				mo->rot = Rot;
				int movetype = MOVE_FORMUP;
				if(unit->enemyContact)
					movetype = MOVE_PASSINGTHROUGH;
				newOrders.push_back(std::make_shared<MoveOrder>(w2.pos, Rot, movetype, true, true, mo->target));
			}
		}
		newOrders.push_back(mo);
		if(mo->type == ORDER_ATTACK)
			newOrders.push_back(std::make_shared<MoveOrder>(mo->pos, mo->rot, MOVE_FORMUP, true, true));
		else if(mo->type == ORDER_TARGET) {
			newOrders.push_back(std::make_shared<MoveOrder>(mo->pos, mo->rot, MOVE_FORMUP, true, true));
		}
	}
	while(nos.size() > start_order) unit->orders.pop_back(); // I cant use nos here, damnit
	unit->orders.insert(unit->orders.end(), newOrders.begin(), newOrders.end());
	debug("Pathfinding done");
}

#endif