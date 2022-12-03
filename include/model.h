#ifndef MODEL
#define MODEL

#include <base.h>
#include <soldiers.h>
#include <units.h>
#include <map.h>
#include <physics.h>
#include <player.h>

class Model : public Listener{
	public:
		LinkedList<Player*> players;
		Map* map;
		double* dt;
		Node<Player*>* selectedPlayerNode;
		Node<Unit*>* selectedUnitNode;
	
		Model(EventManager* em, Map* map) : Listener(em) {
			this->map = map;
			dt = &(em->dt);
			selectedPlayerNode = NULL;
			selectedUnitNode = NULL;
		};
		void SetPlayer() {
			if(players.head) {
				selectedPlayerNode = players.head;
				std::cout << "First player was selected.\n";
			}
			else {
				std::cout << "Can't select player, none available.\n";
			}
		}
		void SetUnit() {
			if(selectedPlayerNode) {
				selectedUnitNode = selectedPlayerNode->data->units.head;
				if(!selectedUnitNode) {
					std::cout << "Couldn't select unit, none available.\n";
				}
				else {
					std::cout << "First unit was selected.\n";
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
				if(selectedPlayerNode) {
					if(selectedUnitNode) {
						if(uev->unitID == -1) {
							if(selectedUnitNode->prev) {
								selectedUnitNode = selectedUnitNode->prev;
							}
							else {
								selectedUnitNode = selectedPlayerNode->data->units.tail;
							}
						}
						else if(uev->unitID == 1) {
							if(selectedUnitNode->next) {
								selectedUnitNode = selectedUnitNode->next;
							}
							else {
								selectedUnitNode = selectedPlayerNode->data->units.head;
							}
						}
					}
				}
				else {
					std::cout << "Player was not set. Attempting to set player..\n";
					SetPlayer();
					std::cout << "Attempting to set unit..\n";
					SetUnit();
				}
			}
			else if(ev->type == PLAYER_SELECT_EVENT) {
				PlayerSelectEvent* pev = dynamic_cast<PlayerSelectEvent*>(ev);
				if(selectedPlayerNode) {
					if(pev->playerID == -1) {
						if(selectedPlayerNode->prev) {
							selectedPlayerNode = selectedPlayerNode->prev;
						}
						else {
							selectedPlayerNode = players.tail;
						}
					}
					else if(pev->playerID == 1) {
						if(selectedPlayerNode->next) {
							selectedPlayerNode = selectedPlayerNode->next;
						}
						else {
							selectedPlayerNode = players.head;
						}
					}
					SetUnit();
				}
				else {
					std::cout << "Player was not set. Attempting to set player..\n";
					SetPlayer();
					SetUnit();
				}
			}
			else if (ev->type == REFORM_EVENT) {
				Node<Player*>* currentPlayer;
				Node<Unit*>* currentUnit;
				currentPlayer = players.head;
				while(currentPlayer) {
					currentUnit = currentPlayer->data->units.head;
					while(currentUnit) {
						Unit* unit = currentUnit->data;
						if(unit->placed) {
							ReformUnit(unit);
							MoveTarget(unit);
						}
						currentUnit = currentUnit->next;
					}
					currentPlayer = currentPlayer->next;
				}			
			}
			else if (ev->type == TICK_EVENT) {
				Node<Player*>* currentPlayer;
				Node<Unit*>* currentUnit;
				currentPlayer = players.head;
				while(currentPlayer) {
					currentUnit = currentPlayer->data->units.head;
					while(currentUnit) {
						Unit* unit = currentUnit->data;
						if(unit->placed) {
							if(!unit->nSoldiersOnFirstOrder) {DeleteObsoleteOrder(unit);}
							CollisionScrying(map, unit);
						}
						currentUnit = currentUnit->next;
					}
					currentPlayer = currentPlayer->next;
				}
				CollisionResolution(map);
				MapObjectCollisionHandling(map);	//resolution handled in timestep
				map->Cleangrid();
				currentPlayer = players.head;
				while(currentPlayer) {
					currentUnit = currentPlayer->data->units.head;
					while(currentUnit) {
						Unit* unit = currentUnit->data;
						if(!unit->placed) {
							if(unit->orders.size()>0) {
								Order* o = unit->orders.at(0);
								if(o->type == ORDER_MOVE) {
									MoveOrder* mo = dynamic_cast<MoveOrder*>(o);
									UnitPlaceRequest* pev = new UnitPlaceRequest(unit, mo->pos, mo->rot);
									em->Post(pev);
								}
							}
						}
						if(unit->placed) {
							if(unit->orders.size() > (unit->currentOrder + 1) && CurrentOrderCompleted(unit)) {
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
									Soldier* soldier = (*soldiers).at(i).at(j);
									if(soldier->placed) {
										TimeStep(soldier, *dt);
									}
									if(soldier->arrived && soldier->currentOrder < unit->currentOrder) {
										SoldierNextOrder(soldier, posInUnit->at(i).at(j));
									}
								}
							}
						}
						currentUnit = currentUnit->next;
					}
					currentPlayer = currentPlayer->next;
				}
			}
		}
};

#endif