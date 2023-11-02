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
#include <projectiles.h>

#include <cstdlib>

void OrderPathfinding(Unit* unit, Map* map) {
	std::vector<Order*> newOrders = std::vector<Order*>();
	for(int i = unit->currentOrder + 1; i < unit->orders.size(); i++) {
		debug("Pathfinding for an order started");
		Order* mo = unit->orders.at(i);
		if((unit->orders.at(i)->type == ORDER_MOVE && unit->orders.at(i-1)->type == ORDER_MOVE) || true) {
			Order* mo = unit->orders.at(i);
			Order* pmo = unit->orders.at(i-1);
			// checking if line of sight between orders
			double rad = unit->width*(unit->spacing - 1);
			MapWaypoint w1 = MapWaypoint(mo->pos, rad);
			MapWaypoint w2 = MapWaypoint(pmo->pos, rad);
			Eigen::Matrix2d Rot;
			if(!FreePath(&w1, &w2, map)) {
				std::vector<Eigen::Vector2d> positions = findPath(&w1, &w2, map);
				// finding waypoints with line of sight to start and goal
				/*
				std::vector<int> visibleStart = std::vector<int>();
				std::vector<int> visibleEnd = std::vector<int>();
				for(int j = 0; j < map->waypoints.size(); j++) {
					if(FreePath(&w2, map->waypoints.at(j), map)) {visibleStart.push_back(j);}
					if(FreePath(&w1, map->waypoints.at(j), map)) {visibleEnd.push_back(j);}
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
				}*/
				//if(mindist != std::numeric_limits<float>::infinity()) {
					// determining waypoint positions
					/*std::vector<Eigen::Vector2d> positions = std::vector<Eigen::Vector2d>();
					positions.push_back(map->waypoints.at(start)->pos);
					int next = map->wp_path_next.at(start).at(end);
					while(next != end) {
						positions.push_back(map->waypoints.at(next)->pos);
						next = map->wp_path_next.at(next).at(end);
					}
					positions.push_back(map->waypoints.at(end)->pos);
					positions.push_back(mo->pos);
					*/
					// translating waypoints to orders
				for(int k = 1; k < positions.size(); k++) {
					//Eigen::Matrix2d Rot;
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
						newOrders.push_back(new MoveOrder(positions.at(k-1), Rot, movetype, true, true, mo->target));
						//newOrders.push_back(new MoveOrder(positions.at(k-1), Rot, MOVE_PASSINGTHROUGH, true, true));
						mo->rot = Rot;
					}
					else if(mo->type == ORDER_ATTACK)
						newOrders.push_back(new MoveOrder(positions.at(k-1), Rot, MOVE_PASSINGTHROUGH, true, false, mo->target));
					else
						newOrders.push_back(new MoveOrder(positions.at(k-1), Rot, MOVE_PASSINGTHROUGH, true, false));
				}

				//}
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
				newOrders.push_back(new MoveOrder(w2.pos, Rot, movetype, true, true, mo->target));
			}
		}
		newOrders.push_back(mo);
		if(mo->type == ORDER_ATTACK)
			newOrders.push_back(new MoveOrder(mo->pos, mo->rot, MOVE_FORMUP, true, true));
	}
	while(unit->orders.size() > unit->currentOrder + 1) unit->orders.pop_back();
	unit->orders.insert(unit->orders.end(), newOrders.begin(), newOrders.end());
	debug("Pathfinding done");
}

struct DamageTick {
	Soldier* soldier;
	int dmg;

	DamageTick(Soldier* soldier, double dmg) {
		this->soldier = soldier;
		this->dmg = dmg;
	}
};

class Model : public Listener{
	public:
		std::vector<Player*> players;
		Player* player1;
		Player* player2;
		Map* map;
		double* dt;
		Player* selectedPlayer;
		Unit* selectedUnit;
		std::queue<DamageTick> damages;
		std::vector<Projectile*> projectiles;
	
		Model(EventManager* em, Map* map) : Listener(em) {
			this->map = map;
			dt = &(em->dt);
			selectedPlayer = NULL;
			selectedUnit = NULL;
		}
		void SetPlayer() {
			selectedPlayer = player1;
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
						if(!oev->orders.empty() && oev->orders.at(0)->type == ORDER_ATTACK)
							unit->orders.push_back(new MoveOrder(unit->pos, unit->rot, MOVE_PASSINGTHROUGH, true, true, oev->orders.at(0)->target));
						else
							unit->orders.push_back(new MoveOrder(unit->pos, unit->rot, MOVE_PASSINGTHROUGH, true, true));
					}
					// appending the new orders
					for(auto order : oev->orders) {
						if(order->type == ORDER_ATTACK) {
							Unit* target = dynamic_cast<AttackOrder*>(order)->target;
							if(target->orders.at(target->currentOrder)->type == ORDER_ATTACK)
								order->pos = target->pos;
							else
								order->pos = target->posTarget;
							target->targetedBy.push_back(unit);
						}
						unit->orders.push_back(order);
					}
					debug("Pushed back new orders");
					if(!unit->placed) unit->currentOrder = 0;
					Order* o = unit->orders.at(unit->currentOrder);
					debug("Found new new order");
					for(auto row : unit->soldiers) {
						for(auto soldier : row) {
							if(soldier->placed && soldier->alive) {
								if(soldier->currentOrder == unit->currentOrder) {
									soldier->arrived = false;
								}
							}
						}
					}
					unit->nSoldiersArrived = 0;
					// doing pathfinding on all orders
					OrderPathfinding(unit, map);
					// debug
					// end debug
					// reforming so that the new position targets are set
					if(unit->placed) {
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
						if(order->type == ORDER_ATTACK) {
							Unit* target = dynamic_cast<AttackOrder*>(order)->target;
							if(target->orders.at(target->currentOrder)->type == ORDER_ATTACK)
								order->pos = target->pos;
							else
								order->pos = target->posTarget;
							target->targetedBy.push_back(unit);
						}
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
					UnitRosterModifiedEvent e;
					em->Post(&e);
				}
			}
			else if(ev->type == UNIT_DELETE_EVENT) {
				if(selectedPlayer && selectedUnit) {
					selectedPlayer->units.erase(std::find(selectedPlayer->units.begin(), selectedPlayer->units.end(), selectedUnit));
					SetUnit();
					UnitRosterModifiedEvent e;
					em->Post(&e);
				}
			}
			else if(ev->type == PLAYER_SELECT_EVENT) {
				PlayerSelectEvent* pev = dynamic_cast<PlayerSelectEvent*>(ev);
				/*if(selectedPlayer && !players.empty()) {
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
				}*/
				if(selectedPlayer->player1) {
					selectedPlayer = player2;
				}
				else {
					selectedPlayer = player1;
				}
				SetUnit();
			}
			else if (ev->type == PLAYER_ADD_EVENT) {
				/*Player* newP = new Player();
				players.push_back(newP);
				newP->units.push_back(new  Infantry(newP));
				newP->units.push_back(new Cavalry(newP));
				newP->units.push_back(new MonsterUnit(newP));
				if(std::find(players.begin(), players.end(), selectedPlayer) == players.end()) {
					SetPlayer();
					SetUnit();
				}*/
			}
			else if (ev->type == PLAYER_DELETE_EVENT) {
				/*if(selectedPlayer) {
					players.erase(std::find(players.begin(), players.end(), selectedPlayer));
					SetPlayer();
					SetUnit();
					if(players.empty()) selectedPlayer = NULL;
				}*/
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
			else if (ev->type == KILL_EVENT) {
				KillEvent* kev = dynamic_cast<KillEvent*>(ev);
				Soldier* soldier = kev->soldier;
				if(soldier->alive) {
					Unit* unit = soldier->unit;
					soldier->alive = false;
					unit->nLiveSoldiers--;
					if(!soldier->arrived && soldier->currentOrder == 0)
						unit->nSoldiersOnFirstOrder--;
					if(soldier->arrived && soldier->currentOrder == unit->currentOrder)
						unit->nSoldiersArrived--;
					std::erase(soldier->unit->liveSoldiers, soldier);
				}
			}
			else if (ev->type == PROJECTILE_SPAWN_EVENT) {
				//std::cout << "dakka!\n";
				projectiles.push_back(dynamic_cast<ProjectileSpawnEvent*>(ev)->p);
			}
			else if (ev->type == TICK_EVENT) {
				debug("TickEvent: map - begin");
				//mapping soldiers to grid
				for(auto player : players) {
					for(auto unit : player->units) {
						if(unit->placed) {
							if(!unit->nSoldiersOnFirstOrder && unit->nLiveSoldiers) DeleteObsoleteOrder(unit);
							CollisionScrying(map, unit);
						}
					}
				}
				//resolving collisions between soldiers and creating enemy neighbourlists
				CollisionResolution(map);
				//resolving collisions with map objects
				MapObjectCollisionHandling(map);
				ProjectileCollisionScrying(map, projectiles);
				ProjectileCollisionHandling(map);

				for(auto player : players) {
					for(auto unit : player->units) {
						//placing unit
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
					}
				}// new loop required for consistency
				for(auto player : players) {
					for(auto unit : player->units) {
						if(unit->placed) {
							//moving unit target if combat has already started every so often to keep up with moving units
							if(unit->enemyContact && unit->orders.at(unit->currentOrder)->type == ORDER_ATTACK) {
								unit->targetUpdateTimer.decrement();
								//unit->targetUpdateCounter--;
								if(unit->targetUpdateTimer.done()) {
								//if(!unit->targetUpdateCounter) {
									unit->posTarget = unit->orders.at(unit->currentOrder)->target->pos;
									MoveTarget(unit);
									unit->targetUpdateTimer.reset();
									//unit->targetUpdateCounter = 60;
								}
							}
							//advancing order
							if(unit->orders.size() > (unit->currentOrder +1) && CurrentOrderCompleted(unit)) {
								bool transitionOrder = unit->orders.at(unit->currentOrder)->_transition;
								UnitNextOrder(unit);
								Order* o = unit->orders.at(unit->currentOrder);
								if(o->type == ORDER_MOVE || true) {
									ReformUnit(unit);
									MoveTarget(unit);
								}
								//telling other units that this one is moving on if they are targeting it
								if(!unit->enemyContact && !transitionOrder) {
									debug(std::to_string(unit->targetedBy.size()));
									std::vector<std::vector<Order*>> newOrders;
									std::vector<Unit*> targetedByTemp;
									for(auto attacker : unit->targetedBy) {
										//redo pathfinding
										newOrders.push_back(std::vector<Order*>());
										targetedByTemp.push_back(attacker);
										for(int i = attacker->currentOrder; i < attacker->orders.size(); i++) {
											o = attacker->orders.at(i);
											if(!o->_auto) {
												if(o->type == ORDER_ATTACK) {
													o = new AttackOrder(dynamic_cast<AttackOrder*>(o)->target);
												}
												newOrders.back().push_back(o);
											}
										}

									}
									for(int i = 0; i < newOrders.size(); i++) {
										std::erase(unit->targetedBy, targetedByTemp.at(i));
										GiveOrdersRequest gev(targetedByTemp.at(i), newOrders.at(i));
										em->Post(&gev);
									}
								}
							}
							//individual movement
							std::vector<std::vector<Soldier*>>* soldiers = &(unit->soldiers);
							std::vector<std::vector<Eigen::Vector2d>>* posInUnit = &(unit->posInUnit);
							for(int i = 0; i < unit->nrows; i++) {
								for(int j = 0; j < unit->width; j++) {
									Soldier* soldier = soldiers->at(i).at(j);
									if(soldier && soldier->alive) {
										soldier->debugFlag1 = false;
										soldier->debugFlag2 = false;
										soldier->debugFlag3 = false;
										//possibly advancing soldier order during combat
										int co = soldier->currentOrder;
										if(!soldier->charging && unit->orders.at(co)->type != ORDER_ATTACK && unit->orders.size() > co + 1 && unit->enemyContact) {
											Order* no = unit->orders.at(co + 1);
											if(unit->orders.at(co)->target && no->target) {
												Circle c1(soldier->pos, soldier->rad);
												Eigen::Vector2d nextPos = no->pos + no->rot * unit->posInUnit.at(i).at(j);
												Circle c2(nextPos, soldier->rad);
												if(!soldier->arrived && FreePath(&c1, &c2, map)) {
													//SoldierNextOrder(soldier, posInUnit->at(i).at(j));
													soldier->arrived = true;
													if(soldier->currentOrder == unit->currentOrder)
														unit->nSoldiersArrived++;
												}
											}
										}
										//check if need to do indiv pathfinding, but only do this every second or so!
										soldier->indivPathTimer.decrement();
										//soldier->indivPathCooldown--;
										if(soldier->indivPathTimer.done()) {
										//if(soldier->indivPathCooldown < 1) {
											Circle c1(soldier->pos, soldier->rad);
											Circle c2(NoIPFPosTarget(soldier), soldier->rad);
											if(soldier->indivPath.empty()) {
												if(!FreePath(&c1, &c2, map)) {
													//do indiv pathfinding
													soldier->indivPath = findPath(&c2, &c1, map);
												}
											}
											else {
												if(FreePath(&c1, &c2, map)) {
													soldier->indivPath.clear();
												}
												else {
													Circle c3(soldier->indivPath.at(0), soldier->rad);
													if(FreePath(&c1, &c3, map)) {
														if(soldier->indivPath.size() > 1) {
															Circle c4(soldier->indivPath.at(1), soldier->rad);
															if(FreePath(&c1, &c4, map))
																std::erase(soldier->indivPath, soldier->indivPath.at(0));
														}
														else {
															if((c3.pos - c1.pos).norm() < soldier->rad)
																std::erase(soldier->indivPath, soldier->indivPath.at(0));
														}
													}
													else {
														//redo indiv pathfinding
														soldier->indivPath = findPath(&c2, &c1, map);
													}
												}
											}
											soldier->indivPathTimer.reset();
											//soldier->indivPathCooldown = soldier->indivPathCDMax;
										}
										//physics step
										if(soldier->placed && soldier->alive) {
											TimeStep(soldier, *dt);
										}
										//advancing soldier order
										if(soldier->alive && soldier->arrived) {
											if(soldier->currentOrder < unit->currentOrder) {
												SoldierNextOrder(soldier, posInUnit->at(i).at(j));
											}
										}
									}
								}
							}
							UpdatePos(unit);
							UpdateVel(unit);
						}
					}
				}
				//ranged target finding
				for(auto player : players) {
					for(auto unit : player->units) {
						if(unit->placed && unit->ranged) {
							if(unit->rangedTargetUpdateTimer.decrement()) {
								unit->rangedTarget = NULL;	// may be bad flag
								std::vector<UnitDistance> inRange;
								Eigen::Matrix2d rangedCone;
								rangedCone << std::cos(0.5*M_PI*unit->rangedAngle), -std::sin(0.5*M_PI*unit->rangedAngle), 
									std::sin(0.5*M_PI*unit->rangedAngle), std::cos(0.5*M_PI*unit->rangedAngle);
								//assign value to rangedCone
								for(auto player2 : players) {
									if(player2 != player) {
										std::cout << "Selected a different player.\n";
										// go through all units and list those that are in the cone
										for(auto unit2 : player2->units) {
											//if unit in cone: inRange.push_back(unit)
											if(unit2->placed) {
												Circle circ(unit2->pos, 0);
												if(ConeCircleCollision(unit->pos, unit->rot, rangedCone, unit->range, &circ)
													&& (unit->pos - unit2->pos).norm() < unit->range) {
													inRange.push_back(UnitDistance(unit, unit2));
													// currently ignores range stat
												}
											}
											//make some kind of priority score
										}
									}
								}
								std::sort(inRange.begin(), inRange.end(), compareUnitDistance);
								if(!inRange.empty()) {
									unit->rangedTarget = inRange.at(0).unit;
									std::cout << "Targets found, horray!\n";
								}
								else {
									unit->rangedTarget = NULL;
									std::cout << "No ranged targets found.\n";
								}
								unit->rangedTargetUpdateTimer.reset();
							}
						}
					}
				}
				//combat
				for(auto player : players) {
					for(auto unit : player->units) {
						for(auto row : unit->soldiers) {
							if(unit->placed) {
								for(auto soldier : row) {
									if(soldier->alive) {
										std::vector<SoldierNeighbourContainer> targets;
										std::vector<SoldierNeighbourContainer> notInCone;
										bool newTarget = false;
										bool outOfRangeTarget = true;
										soldier->meleeSwingTarget = NULL;
										//debug("looping through enemies in melee range");
										while(!soldier->enemiesInMeleeRange.empty()) {
											SoldierNeighbourContainer enemy = soldier->enemiesInMeleeRange.top();
											Circle circ = enemy.soldier->SoldierCircle();
											if((targets.empty() || soldier->meleeAOE) && ConeCircleCollision(soldier->pos, soldier->rot, soldier->meleeCone, soldier->rad, &circ) && enemy.inTrueRange) {
												soldier->meleeTarget = enemy.soldier; // in/excluding this line could significantly change how flanking works
												outOfRangeTarget = false;
												targets.push_back(enemy);
												if(!soldier->meleeAOE)
													soldier->meleeSwingTarget = enemy.soldier;
												if(!newTarget || (soldier->meleeAOE && !soldier->meleeTarget && !soldier->meleeTarget->alive)) {		// what to do here
													soldier->meleeTarget = enemy.soldier;
													newTarget = true;
												}
												//debug("Set target!");
											}
											else
												notInCone.push_back(enemy);
											soldier->enemiesInMeleeRange.pop();
										}
										if(!newTarget && !notInCone.empty()) {
											soldier->meleeTarget = notInCone.at(0).soldier;
											newTarget = true;
											notInCone.clear();
										}
										//handling "charging" status
										//	while charging soldiers will push into the enemy position
										//  if they have no target in front of them for 1 second they will stop charging and seek out enemies close to them
										//if(unit->orders.at(soldier->currentOrder)->type == ORDER_ATTACK) {
										Order* o = unit->orders.at(soldier->currentOrder);
										if(o->target) {
											if(soldier->charging) {
												if(unit->enemyContact) {
													if(!targets.empty() && (!soldier->meleeAOE && soldier->unit->maxSoldiers == 1) && o->type == ORDER_ATTACK)	{//lone monsters stop charging after impact
														soldier->chargeTimer.reset();
														//soldier->chargeGapTicks = 30;
													}
													else {
														if(!soldier->chargeTimer.done())
														//if(soldier->chargeGapTicks > 0)
															soldier->chargeTimer.decrement();
															//soldier->chargeGapTicks--;
														else
															soldier->charging = false;
													}
												}
												else {
													if((!targets.empty() && targets.at(0).soldier->unit == o->target) || ((unit->pos - unit->posTarget).norm() < 20 && o->type == ORDER_ATTACK)) {
														unit->enemyContact = true;
													}
												}
											}
											else {
												debug("SEEK AND DESTROY!");
												//target finding
												//AttackOrder* ao = dynamic_cast<AttackOrder*>(o);
												if((!soldier->meleeTarget || !soldier->meleeTarget->alive) && !o->target->liveSoldiers.empty()) {
													Soldier* target = NULL;
													if(soldier->meleeAOE && soldier->unit->maxSoldiers == 1) {
														double maxDist = 0;
														for(int i = 0; i < 8; i++) {
															Soldier* current = o->target->liveSoldiers.at(rand()%o->target->liveSoldiers.size());
															double dist = (current->pos - soldier->pos).norm();
															if(dist > maxDist) {
																target = current;
																maxDist = dist;
															}
														}
													}
													else {
														double minDist = std::numeric_limits<double>::infinity();
														for(int i = 0; i < 10; i++) {
															Soldier* current = o->target->liveSoldiers.at(rand()%o->target->liveSoldiers.size());
															double dist = (current->pos - soldier->pos).norm();
															if(dist < minDist) {
																target = current;
																minDist = dist;
															}
														}
													}
													soldier->meleeTarget = target;
												}
											}
										}
										if(soldier->unit->orders.at(soldier->currentOrder)->target
										&& soldier->meleeTarget && !soldier->charging) {//soldier->unit->enemyContact) {
											if(!soldier->noTargetTimer.done()) {
											//if(soldier->cantSeeTargetTimer > 0) {
												soldier->noTargetTimer.decrement();
												//soldier->cantSeeTargetTimer--;
											}
											else {
												Circle c1(soldier->pos, soldier->rad);
												Circle c2(soldier->meleeTarget->pos, soldier->meleeTarget->rad);
												if(!FreePath(&c1, &c2, map)) {
													soldier->meleeTarget = NULL;
												}
												soldier->noTargetTimer.reset();
												//soldier->cantSeeTargetTimer = 60;

											}
										}
										// resolving attacks
										if(!targets.empty() && soldier->MeleeTimer.done()) {
										//if(!targets.empty() && soldier->meleeCooldownTicks == 0) {
											soldier->MeleeTimer.reset();
											//soldier->meleeCooldownTicks = 31;
											for(auto targetContainer : targets) {
												Soldier* target = targetContainer.soldier;
												double hitChance = 0.35 + 0.01*(soldier->meleeAttack - target->meleeDefense) + 0.15*(soldier->antiInfantry && target->infantry) + 0.15*(soldier->antiLarge && target->large);
												hitChance = std::min(std::max(0.08, hitChance), 0.9);
												if(std::rand()/double(RAND_MAX) < hitChance) {
													int dmg = soldier->damage(target);
													dmg = dmg + std::max(int(1 + 0.3*(soldier->antiInfantry && target->infantry)), 1) + std::max(int(0.3*(soldier->antiLarge && target->large)), 1);
													damages.push(DamageTick(target, soldier->damage(target)));
												}
											}
										}
										soldier->MeleeTimer.decrement();
										//if(soldier->meleeCooldownTicks > 0)
										//	soldier->meleeCooldownTicks--;
									}
								}
							}
						}
					}
				}
				// do shooting after melee so that people with melee target cant shoot
				// reset ranged target after every shot (so they dont have to find new target multiple times before shooting)
				for(auto player : players) {
					for(auto unit : player->units) {
						if(unit->ranged) { //&& unit->rangedTarget) {
							for(auto row : unit->soldiers) {
								for(auto soldier : row) {
									if(unit->rangedTarget && soldier->alive) {
										if(soldier->currentOrder < unit->currentOrder) {
											soldier->rangedTarget = NULL;
											//soldier->debugFlag3 = true;
										}
										else if(!soldier->rangedTarget) {
											if(unit->rangedTarget->nLiveSoldiers > 0) {
												Soldier* target = unit->rangedTarget->liveSoldiers.at(rand()%unit->rangedTarget->liveSoldiers.size());
												if(target->currentOrder == unit->rangedTarget->currentOrder) {
													soldier->rangedTarget = target;
												}
											}
										}
										if(soldier->rangedTarget && !soldier->rangedTarget->alive) {
											soldier->rangedTarget = NULL;
										}
										if(soldier->rangedTarget
											&& soldier->currentOrder == unit->currentOrder
											&& soldier->vel.norm() < soldier->maxSpeedForFiring
											&& (!soldier->meleeTarget || (soldier->rangedTarget->pos - soldier->pos).norm() > soldier->rangedMinRange)) {
											if(soldier->ReloadTimer.done()) {
												double t = projectile_flight_time(soldier->rangedTarget->pos - soldier->pos,
													soldier->rangedTarget->vel, soldier->rangedSpeed);
												//std::cout << t << "\n";
												if(t > 0) {
													Displacement dis = ShotAngle(soldier->rangedTarget->pos - soldier->pos,
														soldier->rangedTarget->vel, soldier->rangedSpeed, t, soldier->tans);
													Eigen::Vector2d vel;
													vel << 1., 0.;
													vel = dis.rot * vel * soldier->rangedSpeed;
													// create projectile spawn event
													ProjectileSpawnEvent pev = SpawnProjectile(soldier->projectileType, soldier->pos, vel, static_cast<int>(t/em->dt), em->dt);
													em->Post(&pev);
													soldier->ReloadTimer.reset();
												}
												else
													soldier->rangedTarget = NULL;
											}
											else {
												soldier->ReloadTimer.decrement();
											}
										}
										else {
											if(soldier->vel.norm() < soldier->maxSpeedForFiring) {
											//&& (!soldier->meleeTarget || (soldier->rangedTarget->pos - soldier->pos).norm() > soldier->rangedMinRange)) {
												soldier->ReloadTimer.decrement();
											}
										}
									}
									else
										soldier->rangedTarget = NULL;
								}
							}
						}
					}
				}
				//Projectile hit scanning
				for(auto projectile : projectiles) {
					if(projectile->dead) {
						for(auto soldier : projectile->targets) {
							if(soldier->rangedDefense + 20 < rand()%100)
								damages.push(DamageTick(soldier, projectile->damage(soldier)));
						}
						projectile->targets.clear();
					}
				}
				//resolving damage
				while(!damages.empty()) {
					DamageTick d = damages.front();
					d.soldier->hp -= d.dmg;
					if(d.soldier->hp <= 0) {
						KillEvent e(d.soldier);
						em->Post(&e);
					}
					damages.pop();
				}
				//damages = std::queue<DamageTick>();
				// projectile movement and obsolescence
				for(auto projectile : projectiles) {
					if(projectile->dead) {
						Projectile* tempProj = projectile;
						std::erase(projectiles, projectile);
						//delete tempProj;	///////// VERY IMPORTANT
						std::cout << "POW!\n";
					}
					else {
						projectile->advance();
					}
				}
				//cleanup
				map->Cleangrid();	// do it later and use it for target detection? yes
				debug("TickEvent: map - end");
			}
		}
};

#endif