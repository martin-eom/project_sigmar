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
#include <information.h>


#include <cstdlib>
#include <map>
#include <chrono>

void OrderPathfinding(Unit* unit, Map* map) {
	std::vector<Order*> newOrders = std::vector<Order*>();
	for(int i = unit->currentOrder + 1; i < unit->orders.size(); i++) {
		debug("Pathfinding for an order started");
		Order* mo = unit->orders.at(i);
		if((unit->orders.at(i)->type == ORDER_MOVE && unit->orders.at(i-1)->type == ORDER_MOVE) || true) {
			Order* mo = unit->orders.at(i);
			Order* pmo = unit->orders.at(i-1);
			// checking if line of sight between orders
			double rad = unit->ncols*(unit->yspacing - 1);
			MapWaypoint w1 = MapWaypoint(mo->pos, rad);
			MapWaypoint w2 = MapWaypoint(pmo->pos, rad);
			Eigen::Matrix2d Rot;
			if(!FreePath(&w1, &w2, map)) {
				std::vector<Eigen::Vector2d> positions = findPath(&w1, &w2, map);
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
		else if(mo->type == ORDER_TARGET) {
			newOrders.push_back(new MoveOrder(mo->pos, mo->rot, MOVE_FORMUP, true, true));
		}
	}
	while(unit->orders.size() > unit->currentOrder + 1) unit->orders.pop_back();
	unit->orders.insert(unit->orders.end(), newOrders.begin(), newOrders.end());
	debug("Pathfinding done");
}

struct DamageTick {
	Soldier* soldier;
	double dmg;

	DamageTick(Soldier* soldier, double dmg) {
		this->soldier = soldier;
		this->dmg = dmg;
	}
};

enum MODEL_STATES {
	MODEL_SIMULATION,
	MODEL_GAME_PAUSED,
	MODEL_GAME_RUNNING,
	MODEL_GAME_OVER
};

class Model : public Listener{
	public:
		std::map<std::string, SoldierInformation> SoldierTypes;
		std::map<std::string, UnitInformation> UnitTypes;
		SettingsInformation settings;
		AnimationInformation damageInfo;

		std::vector<Player*> players;
		Player* player1;
		Player* player2;
		std::vector<Unit*> units;
		std::vector<Soldier*> soldiers;
		std::vector<omp_lock_t*> soldier_locks;
		std::vector<omp_lock_t*> unit_locks;
		Map* map;
		double* dt;
		//Player* selectedPlayer;
		//Unit* selectedUnit;
		std::queue<DamageTick> damages;
		std::vector<Projectile*> projectiles;
		int state;
		Timer toNextState = Timer(900);
		std::string result;

		int nticks = 0;
		double time_check_game_over = 0;
		double time_placing_units = 0;
		double time_collision_scrying = 0;
		double time_collision_resolution = 0;
		double time_map_object_collision_handling = 0;
		double time_projectile_collision_scrying = 0;
		double time_projectile_collision_resolution = 0;
		double time_total = 0;
		double time_ranged_target_finding = 0;
		double time_melee_combat = 0;
		double time_physics_step = 0;
		//omp_lock_t time_lock_physics;
		double time_hitscan = 0;
		double time_indiv_pathing = 0;
		//omp_lock_t time_lock_indiv;

		void loadSoldierTypes(std::string filename);
		void loadUnitTypes(std::string filename);
		void loadArmyLists(std::string filename);
		void loadDamageInfo();
		void loadSettings(std::string filename);
		void init();
	
		Model(EventManager* em, Map* map) : Listener(em) {
			this->map = map;
			dt = &(em->dt);
			state = MODEL_SIMULATION;
			//omp_init_lock(&time_lock_physics);
			//omp_init_lock(&time_lock_indiv);
		}
	
	private:
		void Notify(Event* ev) {
			if(ev->type == GIVE_ORDERS_REQUEST) {
				switch(state) {
				case MODEL_SIMULATION:
				case MODEL_GAME_PAUSED: {
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
							//MoveTarget(unit);
							unit->MoveTarget();
						}
					}
					break;}
				}
			}
			if(ev->type == GIVE_ALL_ORDERS_REQUEST) {
				GiveAllOrdersRequest* gaor = dynamic_cast<GiveAllOrdersRequest*>(ev);
				for(int n_unit = 0; n_unit < gaor->orderList.size(); n_unit++) {
					std::vector<Order*> orders = gaor->orderList.at(n_unit);
					if(orders.size() > 0) {
						GiveOrdersRequest gor = GiveOrdersRequest(gaor->player->units.at(n_unit), orders);
						em->Post(&gor);
					}
				}
			}
			if(ev->type == APPEND_ORDERS_REQUEST) {
				switch(state) {
				case MODEL_SIMULATION:
				case MODEL_GAME_PAUSED: {
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
					break;}
				}
			}
			else if(ev->type == UNIT_PLACE_REQUEST) {
				UnitPlaceRequest* pev = dynamic_cast<UnitPlaceRequest*>(ev);
				if(pev->unit) {
					if(!(pev->unit->placed)) {
						//Place(pev->unit, pev->pos, pev->rot);
						pev->unit->Place(pev->pos, pev->rot);
					}
					else {
						std::cout << "Unit was already placed.\n";
					}
				}
				else {
					std::cout << "No unit selected. Attempting to set unit.\n";
					//SetUnit();
				}
			}
			else if (ev->type == REFORM_EVENT) {
				switch(state) {
				case MODEL_SIMULATION:
				case MODEL_GAME_RUNNING: {
					for(auto player : players) {
						for(auto unit : player->units) {
							if(unit->placed) {
								ReformUnit(unit);
								//MoveTarget(unit);
								unit->MoveTarget();
							}
						}
					}
					break;}
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
				projectiles.push_back(dynamic_cast<ProjectileSpawnEvent*>(ev)->p);
			}
			else if(ev->type == CONTINUE_GAME_EVENT) {
				if(state == MODEL_GAME_PAUSED) {
					state = MODEL_GAME_RUNNING;
					toNextState.reset();
				}
			}
			else if (ev->type == TICK_EVENT) {
				nticks++;
				auto global_start = std::chrono::system_clock::now();
				// determining if game over
				auto start = std::chrono::system_clock::now();
				switch(state) {
				case MODEL_GAME_RUNNING:
					if(toNextState.done()) {
						int sumLife1 = 0; 
						int sumLife2 = 0;
						for(auto unit : player1->units) sumLife1 += unit->nLiveSoldiers;
						for(auto unit : player2->units) sumLife2 += unit->nLiveSoldiers;
						std::cout << sumLife1 << " " << sumLife2 << "\n";
						if(sumLife1 == 0 || sumLife2 == 0) {
							state = MODEL_GAME_OVER;
							if(sumLife1 == 0) {
								if(sumLife2 == 0) result = "Game Over: Draw";
								else result = "Game Over: Player 2 wins";
							}
							else result = "Game Over: Player 1 wins";
						}
						else {
							state = MODEL_GAME_PAUSED;
							GamePausedEvent gpe;
							em->Post(&gpe);
							toNextState.reset();
						}
					}
					else {
						toNextState.decrement();
					}
					break;
				}
				auto end = std::chrono::system_clock::now();
				time_check_game_over += std::chrono::duration<double>(end - start).count();

				//placing units
				start = std::chrono::system_clock::now();
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
					}
				}
				end = std::chrono::system_clock::now();
				time_placing_units += std::chrono::duration<double>(end - start).count();

				switch(state) {
				case MODEL_SIMULATION:
				case MODEL_GAME_RUNNING: {
					debug("TickEvent: map - begin");
					//mapping soldiers to grid

					start = std::chrono::system_clock::now();

					for(auto player : players) {
						for(auto unit : player->units) {
							if(unit->placed) {
								if(!unit->nSoldiersOnFirstOrder && unit->nLiveSoldiers) unit->DeleteObsoleteOrder();
								CollisionScrying(map, unit);
							}
						}
					}

					end = std::chrono::system_clock::now();
					time_collision_scrying += std::chrono::duration<double>(end - start).count();
					start = std::chrono::system_clock::now();
					//resolving collisions between soldiers and creating enemy neighbourlists
					CollisionResolution(map, &units, &soldiers, &soldier_locks);
					end = std::chrono::system_clock::now();
					time_collision_resolution += std::chrono::duration<double>(end - start).count();
					//resolving collisions with map objects
					start = std::chrono::system_clock::now();
					MapObjectCollisionHandling(map);
					end = std::chrono::system_clock::now();
					time_map_object_collision_handling += std::chrono::duration<double>(end - start).count();
					start = std::chrono::system_clock::now();
					ProjectileCollisionScrying(map, projectiles);
					end = std::chrono::system_clock::now();
					time_projectile_collision_scrying += std::chrono::duration<double>(end - start).count();
					start = std::chrono::system_clock::now();
					ProjectileCollisionHandling(map);

					end = std::chrono::system_clock::now();
					time_projectile_collision_resolution += std::chrono::duration<double>(end - start).count();

					start = std::chrono::system_clock::now();
					//for(auto player : players) {
					int n_units = units.size();
					#pragma omp parallel for default(shared)
					for(int n_unit = 0; n_unit < n_units; n_unit++) {
						//for(auto unit : player->units) {
						Unit* unit = units.at(n_unit);
						if(unit->placed) {
							//moving unit target if combat has already started every so often to keep up with moving units
							if(unit->enemyContact && unit->orders.at(unit->currentOrder)->type == ORDER_ATTACK) {
								unit->targetUpdateTimer.decrement();
								//unit->targetUpdateCounter--;
								if(unit->targetUpdateTimer.done()) {
								//if(!unit->targetUpdateCounter) {
									unit->posTarget = unit->orders.at(unit->currentOrder)->target->pos;
									//MoveTarget(unit);
									unit->MoveTarget();
									unit->targetUpdateTimer.reset();
									//unit->targetUpdateCounter = 60;
								}
							}
							//advancing order
							if(unit->orders.size() > (unit->currentOrder +1) && unit->CurrentOrderCompleted()) {
								bool transitionOrder = unit->orders.at(unit->currentOrder)->_transition;
								//UnitNextOrder(unit);
								unit->NextOrder();
								Order* o = unit->orders.at(unit->currentOrder);
								if(o->type == ORDER_MOVE || true) {
									ReformUnit(unit);
									//MoveTarget(unit);
									unit->MoveTarget();
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
										omp_set_lock(unit_locks.at(targetedByTemp.at(i)->model_index));
										em->Post(&gev);
										omp_unset_lock(unit_locks.at(targetedByTemp.at(i)->model_index));
									}
								}
							}
							//individual movement
							std::vector<std::vector<Soldier*>>* soldiers = &(unit->soldiers);
							std::vector<std::vector<Eigen::Vector2d>>* posInUnit = &(unit->posInUnit);
							for(int i = 0; i < unit->nrows; i++) {
								for(int j = 0; j < unit->ncols; j++) {
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

										//start = std::chrono::system_clock::now();

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

										//end = std::chrono::system_clock::now();
										//omp_set_lock(&time_lock_indiv);
										//time_indiv_pathing += std::chrono::duration<double>(end - start).count();
										//omp_unset_lock(&time_lock_indiv);

										//start = std::chrono::system_clock::now();

										//physics step
										if(soldier->placed && soldier->alive) {
											TimeStep(soldier, *dt);
										}

										//end = std::chrono::system_clock::now();
										//omp_set_lock(&time_lock_physics);
										//time_physics_step += std::chrono::duration<double>(end - start).count();
										//omp_unset_lock(&time_lock_physics);

										//advancing soldier order
										if(soldier->alive && soldier->arrived) {
											if(soldier->currentOrder < unit->currentOrder) {
												SoldierNextOrder(soldier, posInUnit->at(i).at(j));
											}
										}
									}
								}
							}
							//UpdatePos(unit);
							unit->UpdatePos();
							//UpdateVel(unit);
							unit->UpdateVel();
						}
					}

					end = std::chrono::system_clock::now();
					time_physics_step += std::chrono::duration<double>(end - start).count();

					//ranged target finding
					for(auto player : players) {
						for(auto unit : player->units) {
							if(unit->placed && unit->ranged) {
								if(unit->rangedTargetUpdateTimer.decrement()) {
									unit->rangedTarget = NULL;	// may be bad flag
									Order* current = unit->orders.at(unit->currentOrder);
									// need to detect line of sight issues for unit targets
									if(current->type == ORDER_TARGET && current->target->nLiveSoldiers > 0 && (current->target->pos - unit->pos).norm() < unit->range) {
										unit->rangedTarget = current->target;
									}
									else {
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
											debug("Targets found, horray!");
										}
										else {
											unit->rangedTarget = NULL;
											debug("No ranged targets found.");
										}
									}
									unit->rangedTargetUpdateTimer.reset();
								}
							}
						}
					}
					//combat

					start = std::chrono::system_clock::now();

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
											if(soldier->melee) {
												while(!soldier->enemiesInMeleeRange.empty()) {
													SoldierNeighbourContainer enemy = soldier->enemiesInMeleeRange.top();
													Circle circ = *(enemy.soldier); //enemy.soldier->SoldierCircle();
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
														double hitChance = settings.base_melee_attack + 0.01*(soldier->meleeAttack - target->meleeDefense) 
															+ settings.anti_infantry_attack_bonus*(soldier->antiInfantry && target->infantry) 
															+ settings.anti_large_attack_bonus*(soldier->antiLarge && target->large);
														hitChance = std::min(std::max(settings.min_hit_chance, hitChance), settings.max_hit_chance);
														if(std::rand()/double(RAND_MAX) < hitChance) {
															double dmg = soldier->meleeDamage;
															// melee damage function
															dmg = dmg * (1 + 0.3*(soldier->antiInfantry && target->infantry) + 0.3*(soldier->antiLarge && target->large));
															dmg = dmg * (0.01 * soldier->armorPiercing + (1 - 0.01*soldier->armorPiercing) * (1 - 0.01*target->armor));
															damages.push(DamageTick(target, dmg));
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
					}

					end = std::chrono::system_clock::now();
					time_melee_combat += std::chrono::duration<double>(end - start).count();

					// do shooting after melee so that people with melee target cant shoot
					// reset ranged target after every shot (so they dont have to find new target multiple times before shooting)
					
					start = std::chrono::system_clock::now();
					
					for(auto player : players) {
						for(auto unit : player->units) {
							if(unit->ranged) { //&& unit->rangedTarget) {
								for(auto row : unit->soldiers) {
									for(auto soldier : row) {
										if(unit->rangedTarget && soldier->alive) {
											bool swinging = !soldier->MeleeTimer.done() && soldier->melee;
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
											/*std::cout << static_cast<bool>(soldier->rangedTarget) 
												<< (soldier->currentOrder == unit->currentOrder) 
												<< (soldier->vel.norm() < soldier->maxSpeedForFiring) 
												<< static_cast<bool>(soldier->meleeTarget)
												<< "\n";*/
											if(soldier->rangedTarget
												&& soldier->currentOrder == unit->currentOrder
												&& soldier->vel.norm() < soldier->maxSpeedForFiring
												&& (!soldier->meleeTarget || (soldier->rangedTarget->pos - soldier->pos).norm() > soldier->rangedMinRange)) {
												bool canFire = true;
												if(soldier->rangedHeavy || true) {
													Eigen::Vector2d dist = soldier->rangedTarget->pos - soldier->pos;
													canFire = (soldier->rot.transpose() * dist).x() / dist.norm() > 0.7;
												}
												if(soldier->ReloadTimer.done() && canFire) {
													double t = projectile_flight_time(soldier->rangedTarget->pos - soldier->pos,
														soldier->rangedTarget->vel, soldier->rangedSpeed);
													//std::cout << t << "\n";
													if(t > 0) {
														Displacement dis = ShotAngle(soldier->rangedTarget->pos - soldier->pos,
															soldier->rangedTarget->vel, soldier->rangedSpeed, t, soldier->tans);
														Eigen::Vector2d vel;
														vel << 1., 0.;
														vel = dis.rot * vel * soldier->rangedSpeed;
														if(soldier->rangedTarget->meleeTarget && !soldier->rangedTarget->meleeTarget->large) {
															Soldier* mtarget = soldier->rangedTarget->meleeTarget;
															Eigen::Vector2d targetPos = soldier->pos + vel * t;
															Eigen::Vector2d allyPos = mtarget->pos + mtarget->vel * t;
															double rmin = soldier->tans * (soldier->pos - targetPos).norm() + soldier->rangedAOE;
															if((targetPos - allyPos).norm() - mtarget->rad < rmin)
																canFire = false;
														}
														if(swinging) {
															canFire = false;
														}
														// create projectile spawn event
														if(canFire) {
															ProjectileSpawnEvent pev = SpawnProjectile(soldier->tag, soldier->pos, vel, static_cast<int>(t/em->dt), em->dt, soldier->rangedDamage, soldier->rangedArmorPiercing, soldier->projectileAOE);
															em->Post(&pev);
															soldier->ReloadTimer.reset();
														}
													}
													else
														soldier->rangedTarget = NULL;
												}
												else if(!swinging){
													soldier->ReloadTimer.decrement();
												}
											}
											else {
												if(soldier->vel.norm() < soldier->maxSpeedForFiring && !swinging) {
												//&& (!soldier->meleeTarget || (soldier->rangedTarget->pos - soldier->pos).norm() > soldier->rangedMinRange)) {
													soldier->ReloadTimer.decrement();
												}
											}
										}
										else
											soldier->rangedTarget = NULL;
											if(!soldier->ReloadTimer.done() && soldier->speed < soldier->maxSpeedForFiring)
												soldier->ReloadTimer.decrement();
									}
								}
							}
						}
					}

					end = std::chrono::system_clock::now();
					time_ranged_target_finding += std::chrono::duration<double>(end - start).count();

					start = std::chrono::system_clock::now();

					//Projectile hit scanning
					for(auto projectile : projectiles) {
						if(projectile->dead && !projectile->longDead) {
							for(auto soldier : projectile->targets) {
								double hitChance = settings.ranged_base_attack - 0.01*soldier->rangedDefense;
								hitChance = std::min(std::max(settings.min_hit_chance, hitChance), settings.max_hit_chance);
								if(std::rand()/double(RAND_MAX) < hitChance) {
									double dmg = projectile->damage;
									dmg = dmg * (0.01*projectile->armorPiercing + (1 - 0.01*projectile->armorPiercing) * (1 - 0.01*soldier->armor));
									damages.push(DamageTick(soldier, dmg));
								}
							}
							projectile->targets.clear();
						}
					}

					end = std::chrono::system_clock::now();
					time_hitscan += std::chrono::duration<double>(end - start).count();

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
						if(projectile->longDead) {
							Projectile* tempProj = projectile;
							std::erase(projectiles, projectile);
							//delete tempProj;	///////// VERY IMPORTANT
						}
						else if(projectile->dead) {
							projectile->longDead = true;
						}
						else {
							projectile->advance();
						}
					}
					//cleanup
					map->Cleangrid();	// do it later and use it for target detection? yes
					debug("TickEvent: map - end");
					break;}
				}
				auto global_end = std::chrono::system_clock::now();
				time_total += std::chrono::duration<double>(global_end - global_start).count();

				if(nticks == 3600) {
					time_total = 0;
					time_collision_scrying = 0;
					time_collision_resolution = 0;
					time_map_object_collision_handling = 0;
					time_projectile_collision_scrying = 0;
					time_projectile_collision_resolution = 0;
					time_check_game_over = 0;
					time_placing_units = 0;
					time_ranged_target_finding = 0;
					time_melee_combat = 0;
					time_physics_step = 0;
					time_indiv_pathing = 0;
					em->measureTime = true;
				}
				if(nticks == 4500) {
					std::cout << "####### MODEL TIMING ##############\n";
					std::cout << "total time:              " << time_total << "\n";
					std::cout << "expected time:           " << (nticks - 3600.) / 30. << "\n";
					std::cout << "placing units:           " << time_placing_units << "\n";
					std::cout << "collision scrying:       " << time_collision_scrying << "\n";
					std::cout << "collision resolution:    " << time_collision_resolution << "\n";
					std::cout << "map object collisions:   " << time_map_object_collision_handling << "\n";
					std::cout << "proj. collision scrying: " << time_projectile_collision_scrying << "\n";
					std::cout << "proj. collision handling:" << time_projectile_collision_resolution << "\n";
					std::cout << "ranged target finding:   " << time_ranged_target_finding << "\n";
					std::cout << "melee combat:            " << time_melee_combat << "\n";
					std::cout << "physics step:            " << time_physics_step << "\n";
					std::cout << "projectile hit scanning: " << time_hitscan << "\n";
					std::cout << "individual path finding: " << time_indiv_pathing << "\n";
					std::cout << "##################################\n";
					em->showTimes = true;
				}
			}
		}
};

class MapEditorModel {
public:
	void loadSettings(std::string filename);

	MapEditorSettingsInformation settings;

	MapEditorModel() {}

	void init() {
		loadSettings("config/map_editor_settings.json");
	}
};

void Model::init() {
	loadSoldierTypes("config/templates/classes.json");
	loadUnitTypes("config/templates/units.json");
	//loadDamageInfo();
	loadSettings("config/game_settings.json");
}


#endif