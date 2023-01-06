#ifndef MODEL
#define MODEL

#include <base.h>
#include <soldiers.h>
#include <units.h>
#include <map.h>
#include <physics.h>
#include <player.h>
#include <vector>
#include <pathfinding.h>

void OrderPathfinding(Unit* unit, Map* map) {
	std::vector<Order*> newOrders = std::vector<Order*>();
	for(int i = unit->currentOrder + 1; i < unit->orders.size(); i++) {
		debug("Pathfinding for an order started");
		if(unit->orders.at(i)->type == ORDER_MOVE && unit->orders.at(i-1)->type == ORDER_MOVE) {
			MoveOrder* mo = dynamic_cast<MoveOrder*>(unit->orders.at(i));
			MoveOrder* pmo = dynamic_cast<MoveOrder*>(unit->orders.at(i-1));
			// checking if line of sight between orders
			if(!FreePath(mo->pos, pmo->pos, map)) {
				// finding waypoints with line of sight to start and goal
				std::vector<int> visibleStart = std::vector<int>();
				std::vector<int> visibleEnd = std::vector<int>();
				for(int j = 0; j < map->waypoints.size(); j++) {
					if(FreePath(pmo->pos, map->waypoints.at(j)->pos, map)) {visibleStart.push_back(j);}
					if(FreePath(mo->pos, map->waypoints.at(j)->pos, map)) {visibleEnd.push_back(j);}
				}
				// finding shortest-total-path combination
				int start, end;
				float mindist = std::numeric_limits<float>::infinity();
				for(auto i1 : visibleStart) {
					for(auto i2 : visibleEnd) {
						float dist = (pmo->pos - map->waypoints.at(i1)->pos).norm()
									+ (mo->pos - map->waypoints.at(i2)->pos).norm()
									+ map->wp_path_dist.at(i1).at(i2);
						if(dist < mindist) {
							mindist = dist;
							start = i1;
							end = i2;
						}
					}
				}
				if(mindist != std::numeric_limits<float>::infinity()) {
					// translating waypoints to orders
					std::vector<Eigen::Vector2d> positions = std::vector<Eigen::Vector2d>();
					positions.push_back(map->waypoints.at(start)->pos);
					int next = map->wp_path_next.at(start).at(end);
					while(next != end) {
						positions.push_back(map->waypoints.at(next)->pos);
						next = map->wp_path_next.at(next).at(end);
					}
					positions.push_back(map->waypoints.at(end)->pos);
					positions.push_back(mo->pos);
					for(int k = 1; k < positions.size(); k++) {
						Eigen::Matrix2d Rot;
						Eigen::Vector2d diff = positions.at(k) - positions.at(k-1);
						double d = diff.norm();
						double cos = diff.coeff(0)/d;
						double sin = diff.coeff(1)/d;
						if(d > 0)
							Rot << cos, -sin, sin, cos;
						else
							Rot << 1, 0, 0, 1;
						newOrders.push_back(new MoveOrder(positions.at(k-1), Rot, MOVE_PASSINGTHROUGH));
					}
					// interesting to know:
					//std::cout << (std::numeric_limits<float>::infinity() == 2*std::numeric_limits<float>::infinity()) << "\n";
					//std::cout << (std::numeric_limits<float>::infinity() < 2*std::numeric_limits<float>::infinity()) << "\n";
					//std::cout << (std::numeric_limits<float>::infinity() > 2*std::numeric_limits<float>::infinity()) << "\n";

				}
			}
		}
		newOrders.push_back(unit->orders.at(i));
	}
	while(unit->orders.size() > unit->currentOrder + 1) unit->orders.pop_back();
	unit->orders.insert(unit->orders.end(), newOrders.begin(), newOrders.end());
	debug("Pathfinding done");
}

class Model : public Listener{
	public:
		std::vector<Player*> players;
		Map* map;
		double* dt;
		Player* selectedPlayer;
		Unit* selectedUnit;
	
		Model(EventManager* em, Map* map) : Listener(em) {
			this->map = map;
			dt = &(em->dt);
			selectedPlayer = NULL;
			selectedUnit = NULL;
		};
		void SetPlayer() {
			if(players.size() > 0) {
				selectedPlayer = players.at(0);
				std::cout << "First player was selected.\n";
			}
			else {
				std::cout << "Can't select player, none available.\n";
			}
		}
		void SetUnit() {
			if(selectedPlayer) {
				if(selectedPlayer->units.size() > 0) {
					selectedUnit = selectedPlayer->units.at(0);
					std::cout << "First unit was selected.\n";
				}
				else {
					std::cout << "Couldn't select unit, none available.\n";				
				}
			}
			else {
				std::cout << "Couldn't select unit because player not selected.\n";
			}
		}
	
	private:
		void Notify(Event* ev) {
			if(ev->type == GIVE_ORDERS_REQUEST) {
				GiveOrdersRequest* oev = dynamic_cast<GiveOrdersRequest*>(ev);
				Unit* unit = oev->unit;
				if(unit) {
					if(unit->placed) {
						// deleting all future orders as well as the current one
						debug("Beginning order deletion");
						while(unit->orders.size() > unit->currentOrder) unit->orders.pop_back();
						debug("Finished order deletion");
						// setting a new current order to the current unit position as starting point for the pathfinding calculation
						unit->orders.push_back(new MoveOrder(unit->pos, unit->rot, MOVE_PASSINGTHROUGH));
					}
					// appending the new orders
					for(auto order : oev->orders) unit->orders.push_back(order);
					debug("Pushed back new orders");
					if(!unit->placed) unit->currentOrder = 0;
					Order* o = unit->orders.at(unit->currentOrder);
					debug("Found new new order");
					for(auto row : *unit->soldiers()) {
						for(auto soldier : row) {
							if(soldier->placed) {
								if(soldier->currentOrder == unit->currentOrder) {
									soldier->arrived = false;
								}
							}
						}
					}
					unit->nSoldiersArrived = 0;
					// doing pathfinding on all orders
					OrderPathfinding(unit, map);
					// reforming so that the new position targets are set
					if(o->type == ORDER_MOVE && unit->placed) {
						ReformUnit(unit);
						MoveTarget(unit);
					}
				}
			}
			if(ev->type == APPEND_ORDERS_REQUEST) {
				AppendOrdersRequest* oev = dynamic_cast<AppendOrdersRequest*>(ev);
				Unit* unit = oev->unit;
				if(unit) {
					for(auto order : oev->orders) {
						unit->orders.push_back(order);
					}
					OrderPathfinding(unit, map);
				}
			}
			else if(ev->type == UNIT_PLACE_REQUEST) {
				UnitPlaceRequest* pev = dynamic_cast<UnitPlaceRequest*>(ev);
				if(pev->unit) {
					if(!(pev->unit->placed)) {
						Place(pev->unit, pev->pos, pev->rot);
					}
					else {
						std::cout << "Unit was already placed.\n";
					}
				}
				else {
					std::cout << "No unit selected. Attempting to set unit.\n";
					SetUnit();
				}
			}
			else if(ev->type == UNIT_SELECT_EVENT) {
				UnitSelectEvent* uev = dynamic_cast<UnitSelectEvent*>(ev);
				if(selectedPlayer) {
					if(selectedUnit && !selectedPlayer->units.empty()) {
						auto it = std::find(selectedPlayer->units.begin(), selectedPlayer->units.end(), selectedUnit);
						switch(uev->unitID) {
						case -1: 
							if(it == selectedPlayer->units.begin()) selectedUnit = *(--selectedPlayer->units.end());
							else selectedUnit = *std::prev(it);
							break;
						case 1:
							if(it == (--selectedPlayer->units.end())) selectedUnit = *selectedPlayer->units.begin();
							else selectedUnit = *std::next(it);
							break;
						}
					}
					else {
						std::cout << "No unit was selected. Attempting to set unit..\n";
					}
				}
				else {
					std::cout << "Player was not set. Attempting to set player..\n";
					SetPlayer();
					std::cout << "Attempting to set unit..\n";
					SetUnit();
				}
				if(selectedUnit) {
					em->Post(new RememberOrders(selectedUnit->orders));
				}
			}
			else if(ev->type == UNIT_ADD_EVENT) {
				if(selectedPlayer) {
					UnitAddEvent* uev = dynamic_cast<UnitAddEvent*>(ev);
					Unit* unit = NULL;
					switch(uev->unitType) {
					case UNIT_INFANTRY:
						unit = new Infantry(selectedPlayer); break;
					case UNIT_CAVALRY:
						unit = new Cavalry(selectedPlayer); break;
					case UNIT_MONSTER:
						unit = new MonsterUnit(selectedPlayer); break;
					case UNIT_LONE_RIDER:
						unit = new LoneRider(selectedPlayer); break;
					}
					if(unit) selectedPlayer->units.push_back(unit);
				}
			}
			else if(ev->type == UNIT_DELETE_EVENT) {
				if(selectedPlayer && selectedUnit) {
					selectedPlayer->units.erase(std::find(selectedPlayer->units.begin(), selectedPlayer->units.end(), selectedUnit));
					SetUnit();
				}
			}
			else if(ev->type == PLAYER_SELECT_EVENT) {
				PlayerSelectEvent* pev = dynamic_cast<PlayerSelectEvent*>(ev);
				if(selectedPlayer && !players.empty()) {
					auto it = std::find(players.begin(), (--players.end()), selectedPlayer);
					switch(pev->playerID) {
					case -1:
						if(it == players.begin()) selectedPlayer = *(--players.end());
						else selectedPlayer = *std::prev(it);
						break;
					case 1:
						if(it == (--players.end())) selectedPlayer = *players.begin();
						else selectedPlayer = *std::next(it);
					}
					SetUnit();
				}
				else {
					std::cout << "Player was not set. Attempting to set player..\n";
					SetPlayer();
					SetUnit();
				}
			}
			else if (ev->type == PLAYER_ADD_EVENT) {
				Player* newP = new Player();
				players.push_back(newP);
				newP->units.push_back(new  Infantry(newP));
				newP->units.push_back(new Cavalry(newP));
				newP->units.push_back(new MonsterUnit(newP));
				if(std::find(players.begin(), players.end(), selectedPlayer) == players.end()) {
					SetPlayer();
					SetUnit();
				}
			}
			else if (ev->type == PLAYER_DELETE_EVENT) {
				if(selectedPlayer) {
					players.erase(std::find(players.begin(), players.end(), selectedPlayer));
					SetPlayer();
					SetUnit();
					if(players.empty()) selectedPlayer = NULL;
				}
			}
			else if (ev->type == REFORM_EVENT) {
				for(auto player : players) {
					for(auto unit : player->units) {
						if(unit->placed) {
							ReformUnit(unit);
							MoveTarget(unit);
						}
					}
				}
			}
			else if (ev->type == TICK_EVENT) {
				for(auto player : players) {
					for(auto unit : player->units) {
						if(unit->placed) {
							if(!unit->nSoldiersOnFirstOrder) DeleteObsoleteOrder(unit);
							CollisionScrying(map, unit);
						}
					}
				}
				CollisionResolution(map);
				MapObjectCollisionHandling(map);	//resolution handled in timestep
				map->Cleangrid();
				for(auto player : players) {
					for(auto unit : player->units) {
						if(!unit->placed) {
							if(!unit->orders.empty()) {
								Order* o = unit->orders.at(0);
								if(o->type == ORDER_MOVE) {
									MoveOrder* mo = dynamic_cast<MoveOrder*>(o);
									UnitPlaceRequest* pev = new UnitPlaceRequest(unit, mo->pos, mo->rot);
									em->Post(pev);
								}
							}
						}
						if(unit->placed) {
							if(unit->orders.size() > (unit->currentOrder +1) && CurrentOrderCompleted(unit)) {
								UnitNextOrder(unit);
								Order* o = unit->orders.at(unit->currentOrder);
								if(o->type == ORDER_MOVE) {
									MoveOrder* mo = dynamic_cast<MoveOrder*>(o);
									ReformUnit(unit);
									MoveTarget(unit);
								}
							}
							std::vector<std::vector<Soldier*>>* soldiers = unit->soldiers();
							std::vector<std::vector<Eigen::Vector2d>>* posInUnit = unit->posInUnit();
							for(int i = 0; i < unit->nrows(); i++) {
								for(int j = 0; j < unit->width(); j++) {
									Soldier* soldier = soldiers->at(i).at(j);
									if(soldier) {
										if(soldier->placed) {
											TimeStep(soldier, *dt);
										}
										if(soldier->arrived && soldier->currentOrder < unit->currentOrder) {
											SoldierNextOrder(soldier, posInUnit->at(i).at(j));
										}
									}
								}
							}
							UpdatePos(unit);
							UpdateVel(unit);
						}
					}
				}
			}
		}
};

#endif