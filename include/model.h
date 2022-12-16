#ifndef MODEL
#define MODEL

#include <base.h>
#include <soldiers.h>
#include <units.h>
#include <map.h>
#include <physics.h>
#include <player.h>
#include <vector>

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
					unit->orders.clear();
					unit->orders = oev->orders;
					unit->currentOrder = 0;
					unit->nSoldiersArrived = 0;
					Order* o = unit->orders.at(0);
					std::vector<std::vector<Soldier*>>* soldiers = unit->soldiers();
					for(int i = 0; i < unit->nrows(); i++) {
						for(int j = 0; j < unit->width(); j++) {
							Soldier* soldier = soldiers->at(i).at(j);
							if(soldier) {
								soldier->currentOrder = 0;
								soldier->arrived = false;
							}
						}
					}
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