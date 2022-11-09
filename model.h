#define MODEL

#ifndef BASE
#include "base.h"
#endif
#ifndef SOLDIERS
#include "soldiers.h"
#endif
#ifndef UNITS
#include "units.h"
#endif
#ifndef MAP
#include "map.h"
#endif
#ifndef PHYSICS
#include "physics.h"
#endif
#ifndef PLAYER
#include "player.h"
#endif

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
			if(ev->type == UNIT_PLACE_REQUEST) {
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
			else if(ev->type == UNIT_MOVE_REQUEST) {
				UnitMoveRequest* mev = dynamic_cast<UnitMoveRequest*>(ev);
				if(mev->unit) {
					if(mev->unit->placed) {
						ReformUnit(mev->unit, mev->rot);
						MoveTarget(mev->unit, mev->pos, mev->rot);
					}
					else {
						std::cout << "Unit was not placed.\n";
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
							ReformUnit(unit, unit->rotTarget);
							MoveTarget(unit, unit->posTarget, unit->rotTarget);
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
							CollisionScrying(map, unit);
						}
						currentUnit = currentUnit->next;
					}
					currentPlayer = currentPlayer->next;
				}
				CollisionResolution(map);
				map->Cleangrid();
				currentPlayer = players.head;
				while(currentPlayer) {
					currentUnit = currentPlayer->data->units.head;
					while(currentUnit) {
						Unit* unit = currentUnit->data;
						if(unit->placed) {
							std::vector<std::vector<Soldier*>>* soldiers = unit->soldiers();
							for(int i = 0; i < unit->nrows(); i++) {
								for(int j = 0; j < unit->width(); j++) {
									Soldier* soldier = (*soldiers).at(i).at(j);
									if(soldier->placed) {
										TimeStep(soldier, *dt);
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
