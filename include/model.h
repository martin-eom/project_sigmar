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
#include <timing.h>


#include <cstdlib>
#include <map>
#include <chrono>


void OrderPathfinding(Unit* unit, Map* map, std::vector<Order*> nos = std::vector<Order*>(), int start_order = 0) {
	std::vector<Order*> newOrders = std::vector<Order*>();
	if(start_order == 0)
		start_order = unit->currentOrder + 1;
		nos = unit->orders;
	for(int i = start_order; i < nos.size(); i++) {
		debug("Pathfinding for an order started");
		Order* mo = nos.at(i);
		if((nos.at(i)->type == ORDER_MOVE && nos.at(i-1)->type == ORDER_MOVE) || true) {
			Order* mo = nos.at(i);
			Order* pmo = nos.at(i-1);
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
						mo->rot = Rot;
					}
					else if(mo->type == ORDER_ATTACK)
						newOrders.push_back(new MoveOrder(positions.at(k-1), Rot, MOVE_PASSINGTHROUGH, true, false, mo->target));
					else
						newOrders.push_back(new MoveOrder(positions.at(k-1), Rot, MOVE_PASSINGTHROUGH, true, false));
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
	while(nos.size() > start_order) unit->orders.pop_back(); // I cant use nos here, damnit
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
		double time_hitscan = 0;
		double time_indiv_pathing = 0;
		bool displayedTime = false;

		void loadSoldierTypes(std::string filename);
		void loadUnitTypes(std::string filename);
		void loadArmyLists(std::string filename);
		void loadDamageInfo();
		void loadSettings(std::string filename);
		void init();

		void GameStateCheck();
		void PlaceUnits();
		void MapSoldiersToGrid();
		void RangedTargetFinding();
		void MeleeCombat();
		void Shooting();
		void ProjectileHitResolution();
		void DamageResolution();
		void ProjectileCleanup();
	
		void GiveOrdersResponse(Event* ev);
		void GiveAllOrdersResponse(Event* ev);
		void AppendOrdersResponse(Event* ev);
		void ReformResponse(Event* ev);
		void KillResponse(Event* ev);
		void TickResponse(Event* ev);

		Model(EventManager* em, Map* map) : Listener(em) {
			this->map = map;
			dt = &(em->dt);
			state = MODEL_SIMULATION;
		}
	
	private:
		void Notify(Event* ev) {
			if(ev->type == GIVE_ORDERS_REQUEST) {
				GiveOrdersResponse(ev);
			}
			else if(ev->type == GIVE_ALL_ORDERS_REQUEST) {
				GiveAllOrdersResponse(ev);
			}
			else if(ev->type == APPEND_ORDERS_REQUEST) {
				AppendOrdersResponse(ev);
			}
			else if(ev->type == UNIT_PLACE_REQUEST) {
				UnitPlaceRequest* pev = dynamic_cast<UnitPlaceRequest*>(ev);
				if(pev->unit) {
					if(!(pev->unit->placed)) {
						pev->unit->Place(pev->pos, pev->rot);
					}
				}
			}
			else if (ev->type == REFORM_EVENT) {
				ReformResponse(ev);
			}
			else if (ev->type == KILL_EVENT) {
				KillResponse(ev);
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
				auto global_start = std::chrono::system_clock::now();
				TickResponse(ev);

				switch(state) {
				case MODEL_SIMULATION:
				case MODEL_GAME_RUNNING: {
					debug("TickEvent: map - begin");

					/*
					//combat
					
					auto start = std::chrono::system_clock::now();

					for(auto player : players) {
						for(auto unit : player->units) {
							for(auto row : unit->soldiers) {
								if(unit->placed) {
									for(auto soldier : row) {
										if(soldier->alive) {
											std::vector<SoldierNeighbourContainer> targets;
											std::vector<SoldierNeighbourContainer> notInCone;
											bool newTarget = false;
											//bool outOfRangeTarget = true;
											soldier->meleeSwingTarget = NULL;
											//debug("looping through enemies in melee range");
											if(soldier->melee) {
												while(!soldier->enemiesInMeleeRange.empty()) {
													SoldierNeighbourContainer enemy = soldier->enemiesInMeleeRange.top();
													Circle circ = *(enemy.soldier); //enemy.soldier->SoldierCircle();
													if((targets.empty() || soldier->meleeAOE) && ConeCircleCollision(soldier->pos, soldier->rot, soldier->meleeCone, soldier->rad, &circ) && enemy.inTrueRange) {
														soldier->meleeTarget = enemy.soldier; // in/excluding this line could significantly change how flanking works
														//outOfRangeTarget = false;
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
												Order* o = unit->orders.at(soldier->currentOrder);
												if(o->target) {
													if(soldier->charging) {
														if(unit->enemyContact) {
															if(!targets.empty() && (!soldier->meleeAOE && soldier->unit->maxSoldiers == 1) && o->type == ORDER_ATTACK)	{//lone monsters stop charging after impact
																soldier->chargeTimer.reset();
															}
															else {
																if(!soldier->chargeTimer.done())
																	soldier->chargeTimer.decrement();
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
														soldier->noTargetTimer.decrement();
													}
													else {
														Circle c1(soldier->pos, soldier->rad);
														Circle c2(soldier->meleeTarget->pos, soldier->meleeTarget->rad);
														if(!FreePath(&c1, &c2, map)) {
															soldier->meleeTarget = NULL;
														}
														soldier->noTargetTimer.reset();
													}
												}
												// resolving attacks
												if(!targets.empty() && soldier->MeleeTimer.done()) {
													soldier->MeleeTimer.reset();
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
											}
										}
									}
								}
							}
						}
					}

					auto end = std::chrono::system_clock::now();
					if(state != MODEL_GAME_PAUSED)
						time_melee_combat += std::chrono::duration<double>(end - start).count();*/

					// do shooting after melee so that people with melee target cant shoot
					// reset ranged target after every shot (so they dont have to find new target multiple times before shooting)
					/*
					auto start = std::chrono::system_clock::now();
					
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
											else if(soldier->rangedTarget && (soldier->rangedTarget->unit != unit->rangedTarget)) {
												soldier->rangedTarget = NULL;
											}
											else if(!soldier->rangedTarget) {
												if(unit->rangedTarget->nLiveSoldiers > 0) {
													Soldier* target = unit->rangedTarget->liveSoldiers.at(rand()%unit->rangedTarget->liveSoldiers.size());
													//here goes the hit detection
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
												bool canFire = true;
												if(soldier->rangedHeavy || true) {
													Eigen::Vector2d dist = soldier->rangedTarget->pos - soldier->pos;
													canFire = (soldier->rot.transpose() * dist).x() / dist.norm() > 0.7;
												}
												if(soldier->ReloadTimer.done() && canFire) {
													double t = projectile_flight_time(soldier->rangedTarget->pos - soldier->pos,
														soldier->rangedTarget->vel, soldier->rangedSpeed);
													if(t > 0 && soldier->projectileSpeed * t < soldier->rangedRange) {
														Displacement dis = ShotAngle(soldier->rangedTarget->pos - soldier->pos,
															soldier->rangedTarget->vel, soldier->rangedSpeed, t, soldier->tans);
														Eigen::Vector2d vel;
														vel << 1., 0.;
														vel = dis.rot * vel * soldier->rangedSpeed;
														//los check
														Eigen::Vector2d pointOfImpact = soldier->pos + vel * t;
														Circle c1 = Circle(soldier->pos, soldier->rad);
														Circle c2 = Circle(pointOfImpact, soldier->rad);//soldier->rad);
														if(!FreePath(&c1, &c2, map, true)) {
															canFire = false;
														}
														if(canFire && soldier->rangedTarget->meleeTarget && !soldier->rangedTarget->meleeTarget->large) {
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
															ProjectileSpawnEvent pev = SpawnProjectile(soldier->tag, soldier->pos, vel, static_cast<int>(t/em->dt), em->dt, soldier->rangedDamage, soldier->rangedArmorPiercing, soldier->projectileAOE, soldier->projectileTilesize);
															em->Post(&pev);
															soldier->ReloadTimer.reset();
														}
														else
															soldier->rangedTarget = NULL;
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

					auto end = std::chrono::system_clock::now();
					if(state != MODEL_GAME_PAUSED)
						time_ranged_target_finding += std::chrono::duration<double>(end - start).count();
						*/
					/*
					auto start = std::chrono::system_clock::now();

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

					auto end = std::chrono::system_clock::now();
					if(state != MODEL_GAME_PAUSED)
						time_hitscan += std::chrono::duration<double>(end - start).count();*/

					//resolving damage
					/*while(!damages.empty()) {
						DamageTick d = damages.front();
						d.soldier->hp -= d.dmg;
						if(d.soldier->hp <= 0) {
							KillEvent e(d.soldier);
							em->Post(&e);
						}
						damages.pop();
					}*/
					// projectile movement and obsolescence
					/*for(auto projectile : projectiles) {
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
					}*/

					//cleanup
					//map->Cleangrid();	// do it later and use it for target detection? yes
					debug("TickEvent: map - end");
					break;}
				}
				auto global_end = std::chrono::system_clock::now();
				if(state != MODEL_GAME_PAUSED)
					time_total += std::chrono::duration<double>(global_end - global_start).count();

				if(state == MODEL_GAME_OVER && !displayedTime) {
					std::cout << "####### MODEL TIMING ##############\n";
					std::cout << "total time:              " << time_total << "\n";
					//std::cout << "expected time:           " << (nticks - 3600.) / 30. << "\n";
					std::cout << "expected time:           " << (nticks/1.) / 30. << "\n";
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
					displayedTime = true;
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
	if(settings.auto_generate_map_grids) {
		settings.map_grids.clear();
		for(auto it: SoldierTypes) {
			auto info = it.second;
			int size = std::max(int(std::ceil(info.radius * 2)), 5);
			if(std::find(settings.map_grids.begin(), settings.map_grids.end(), size) == settings.map_grids.end())
				settings.map_grids.push_back(size);
		}
		for(auto it: SoldierTypes) {
			auto info = it.second;
			if(info.ranged_ranged) {
				int size = std::max(int(std::ceil(info.ranged_aoe * 2)), *std::min_element(settings.map_grids.begin(), settings.map_grids.end()));
				if(std::find(settings.map_grids.begin(), settings.map_grids.end(), size) == settings.map_grids.end())
					settings.map_grids.push_back(size);
			}
		}
	}
	std::sort(settings.map_grids.begin(), settings.map_grids.end());
	if(settings.map_grids.empty())
		settings.map_grids.push_back(map->optimalTileSize);
	for(auto it: SoldierTypes) {
		auto key = it.first;
		//auto info = &(it.second);
		SoldierTypes[key].tilesize = settings.map_grids.at(0);
		for(int size: settings.map_grids) {
			if(size >= SoldierTypes[key].tilesize) {
				SoldierTypes[key].tilesize = size;
				if(size >= std::ceil(SoldierTypes[key].radius * 2))
					break;
			}
		}
		std::cout << SoldierTypes[key].tag << " " << SoldierTypes[key].tilesize << "\n";
		SoldierTypes[key].projectile_tilesize = settings.map_grids.at(0);
		for(int size: settings.map_grids) {
			if(size >= SoldierTypes[key].projectile_tilesize) {
				SoldierTypes[key].projectile_tilesize = size;
				if(size >= std::ceil(SoldierTypes[key].ranged_aoe * 2))
					break;
			}
		}
	}
	std::cout << "map grid sizes:\n";
	for(auto entry: settings.map_grids) {
		std::cout << entry << " ";
	}
	std::cout << "\n--------------\n";
	map->initGrids(settings.map_grids);
	std::cout << "map grid sizes:\n";
	for(auto container: map->grids) {
		std::cout << container->tilesize << " " << container->grid.size() << " " << container->grid.at(0).size() << "\n";
	}
	std::cout << "\n--------------\n";
	map->UpdateGridsWithMapObjects();
	std::string neighbourFileString = map->neighbourFileStr();
	if(map->searchNeighbourFile(neighbourFileString)) {
		std::cout << "Found neighbour-mapping file for this configuration of map size and grid sizes.\n";
		std::cout << "Reading neighbour map from " << neighbourFileString << "\n";
		map->readNeighbourFile(neighbourFileString);
		std::cout << "Done.\n";
	}
	else {
		std::cout << "No neighbour-mapping file found for this configuration of map size and grid sizes.\n";
		std::cout << "Creating neighbour map...\n";
		map->setAllNeighbours();
		std::cout << "Writing neighbour map to file " << neighbourFileString << "\n";
		map->writeNeighbourFile(neighbourFileString);
		std::cout << "Done.\n";
	}
	if(settings.set_custom_omp_num_threads)
		omp_set_num_threads(std::min(settings.custom_omp_num_threads, omp_get_max_threads() - 1));
	else
		omp_set_num_threads(std::max(1, omp_get_max_threads() - 1));

}

void Model::GameStateCheck() {
	switch(state) {
	case MODEL_GAME_RUNNING:
		if(toNextState.done()) {
			int sumLife1 = 0; 
			int sumLife2 = 0;
			for(auto unit : player1->units) sumLife1 += unit->nLiveSoldiers;
			for(auto unit : player2->units) sumLife2 += unit->nLiveSoldiers;
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
}

void Model::PlaceUnits() {
	for(auto player : players) {
		for(auto unit : player->units) {
			if(!unit->placed) {
				if(!unit->orders.empty()) {
					Order* o = unit->orders.at(0);
					if(o->type == ORDER_MOVE) {	// this is only triggered in simulation mode, if you try to place a unit with an attack order
						UnitPlaceRequest* pev = new UnitPlaceRequest(unit, o->pos, o->rot);
						em->Post(pev);
					}
				}
			}
		}
	}
}

void Model::MapSoldiersToGrid() {
	for(auto unit : units) {
		if(unit->placed) {
			CollisionScrying(map, unit);
		}
	}
}

void Model::GiveOrdersResponse(Event* ev) {
	switch(state) {
	case MODEL_SIMULATION:
	case MODEL_GAME_PAUSED: {
		GiveOrdersRequest* oev = dynamic_cast<GiveOrdersRequest*>(ev);
		Unit* unit = oev->unit;
		if(unit) {

			if(unit->placed) {
				// special case for units on combat order
				if(unit->orders.at(unit->currentOrder)->type == ORDER_ATTACK) {
					unit->ResetCharging();
				}
				// deleting all future orders as well as the current one
				while(unit->orders.size() > unit->currentOrder) unit->orders.pop_back();
				// setting a new current order to the current unit position as starting point for the pathfinding calculation
				if(!oev->orders.empty() && oev->orders.at(0)->type == ORDER_ATTACK)
					unit->orders.push_back(new MoveOrder(unit->pos, unit->rot, MOVE_PASSINGTHROUGH, true, false, oev->orders.at(0)->target));
				else
					unit->orders.push_back(new MoveOrder(unit->pos, unit->rot, MOVE_PASSINGTHROUGH, true, false));
			}
			// appending the new orders
			for(auto order : oev->orders) {
				if(order->type == ORDER_ATTACK) {
					order->setCombat();
					Unit* target = dynamic_cast<AttackOrder*>(order)->target;
					order->pos = target->pos;
				}
				unit->orders.push_back(order);
			}

			// preparing unplaced units
			if(!unit->placed) unit->currentOrder = 0;
			Order* o = unit->orders.at(unit->currentOrder);
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
			// reforming so that the new position targets are set
			if(unit->placed) {
				ReformUnit(unit);
				unit->MoveTarget();
			}
		}
		break;}
	}
}

void Model::GiveAllOrdersResponse(Event* ev) {
	GiveAllOrdersRequest* gaor = dynamic_cast<GiveAllOrdersRequest*>(ev);
	for(int n_unit = 0; n_unit < gaor->orderList.size(); n_unit++) {
		std::vector<Order*> orders = gaor->orderList.at(n_unit);
		if(orders.size() > 0) {
			GiveOrdersRequest gor = GiveOrdersRequest(gaor->player->units.at(n_unit), orders);
			em->Post(&gor);
		}
	}
}

void Model::AppendOrdersResponse(Event* ev) {
	switch(state) {
	case MODEL_SIMULATION:
	case MODEL_GAME_PAUSED: {
		AppendOrdersRequest* oev = dynamic_cast<AppendOrdersRequest*>(ev);
		Unit* unit = oev->unit;
		if(unit) {
			for(auto order : oev->orders) {
				if(order->type == ORDER_ATTACK) {
					Unit* target = dynamic_cast<AttackOrder*>(order)->target;
					order->pos = target->pos;
				}
				unit->orders.push_back(order);
			}
		}
		break;}
	}
}

void Model::ReformResponse(Event* ev) {
	switch(state) {
	case MODEL_SIMULATION:
	case MODEL_GAME_RUNNING: {
		for(auto player : players) {
			for(auto unit : player->units) {
				if(unit->placed) {
					ReformUnit(unit);
					unit->MoveTarget();
				}
			}
		}
		break;}
	}
}

void Model::KillResponse(Event* ev) {
	KillEvent* kev = dynamic_cast<KillEvent*>(ev);
	Soldier* soldier = kev->soldier;
	if(soldier->alive) {
		Unit* unit = soldier->unit;
		soldier->alive = false;
		unit->nLiveSoldiers--;
		if(soldier->currentOrder == 0)
			unit->nSoldiersOnFirstOrder--;
		if(soldier->arrived && soldier->currentOrder == unit->currentOrder)
			unit->nSoldiersArrived--;
		std::erase(soldier->unit->liveSoldiers, soldier);
	}
}

void Model::TickResponse(Event* ev) {
	if(state != MODEL_GAME_PAUSED)
		nticks++;

	// determining if game over
	auto time = TimeFunction(std::bind(&Model::GameStateCheck, this));
	//auto time = TimeFunction([&]() {GameStateCheck();});
	if(state != MODEL_GAME_PAUSED)
		time_check_game_over += time;


	switch(state) {
	case MODEL_SIMULATION:
	case MODEL_GAME_RUNNING: {

		//placing units
		time = TimeFunction(std::bind(&Model::PlaceUnits, this));
		if(state != MODEL_GAME_PAUSED)
			time_placing_units += time;

		//deleting obsolete orders
		for(auto unit: units) {
			if(!unit->nSoldiersOnFirstOrder && unit->nLiveSoldiers) unit->DeleteObsoleteOrder();
		}

		//mapping soldiers to grid
		time = TimeFunction(std::bind(&Model::MapSoldiersToGrid, this));
		if(state != MODEL_GAME_PAUSED)
			time_collision_scrying += time;

		//resolving collisions between soldiers and creating enemy neighbourlists
		auto start = std::chrono::system_clock::now();
		CollisionResolution(map, &units, &soldiers, &soldier_locks);
		auto end = std::chrono::system_clock::now();
		if(state != MODEL_GAME_PAUSED)
			time_collision_resolution += std::chrono::duration<double>(end - start).count();

		//resolving collisions with map objects
		start = std::chrono::system_clock::now();
		MapObjectCollisionHandling(map);
		end = std::chrono::system_clock::now();
		if(state != MODEL_GAME_PAUSED)
			time_map_object_collision_handling += std::chrono::duration<double>(end - start).count();

		//mapping projectiles to grid
		start = std::chrono::system_clock::now();
		ProjectileCollisionScrying(map, projectiles);
		end = std::chrono::system_clock::now();
		if(state != MODEL_GAME_PAUSED)
			time_projectile_collision_scrying += std::chrono::duration<double>(end - start).count();

		//resolving projectile collisions
		start = std::chrono::system_clock::now();
		ProjectileCollisionHandling(map);
		end = std::chrono::system_clock::now();
		if(state != MODEL_GAME_PAUSED)
			time_projectile_collision_resolution += std::chrono::duration<double>(end - start).count();

		//physics step
		start = std::chrono::system_clock::now();
		int n_units = units.size();
		#pragma omp parallel for default(shared)
		for(int n_unit = 0; n_unit < n_units; n_unit++) {
			Unit* unit = units.at(n_unit);
			if(unit->placed) {
				//moving unit target if combat has already started every so often to keep up with moving units
				if(unit->orders.at(unit->currentOrder)->_combat) {
					unit->UpdateTargetPath(map, em);
				}
				//advancing order
				if(unit->CurrentOrderCompleted()) {
					unit->AdvanceOrder(map);
				}
				//individual movement
				unit->SoldierMovement(map, dt);
				unit->UpdatePos();
				unit->UpdateVel();
			}
		}
		end = std::chrono::system_clock::now();
		if(state != MODEL_GAME_PAUSED)
			time_physics_step += std::chrono::duration<double>(end - start).count();

		//ranged target finding
		RangedTargetFinding();

		time = TimeFunction(std::bind(&Model::MeleeCombat, this));
		if(state != MODEL_GAME_PAUSED)
			time_melee_combat += time;

		time = TimeFunction(std::bind(&Model::Shooting, this));
		if(state != MODEL_GAME_PAUSED)
			time_ranged_target_finding += time;

		time = TimeFunction(std::bind(&Model::ProjectileHitResolution, this));
		if(state != MODEL_GAME_PAUSED)
			time_hitscan += time;

		DamageResolution();
		ProjectileCleanup();
		map->Cleangrid();	// do it later and use it for target detection? yes
		break;}
	}
}

void Model::RangedTargetFinding() {
	for(auto unit: units) {
		if(unit->placed && unit->ranged && unit->rangedTargetUpdateTimer.decrement()) {
			unit->rangedTarget = NULL;
			if(!unit->UseOrderTarget(map)) {
				unit->FindRangedTarget(players);
			}
			unit->rangedTargetUpdateTimer.reset();
		}
	}
}

void Model::MeleeCombat() {
	for(auto unit : units) {
		if(unit->placed) {
			for(auto row : unit->soldiers) {
				for(auto soldier : row) {
					if(soldier->alive) {
						std::vector<SoldierNeighbourContainer> targets;
						std::vector<SoldierNeighbourContainer> notInCone;
						soldier->meleeSwingTarget = NULL;
						if(soldier->melee) {
							soldier->ChooseMeleeTargetsByRangeAndCone(&targets, &notInCone);
							soldier->HandleCharging(&targets);
							soldier->CheckIfPathToTarget(map);
							soldier->ResolveAttacks(this, &targets);
						}
					}
				}
			}
		}
	}
}


void Model::Shooting() {
	for(auto unit : units) {
		if(unit->ranged) {
			for(auto soldier : unit->liveSoldiers) {
				if(unit->rangedTarget) {
					soldier->GetValidTargetRangedTarget();
					soldier->FireOrReloadIfPossible(map, em);
				}
				else
					soldier->rangedTarget = NULL;
					if(!soldier->ReloadTimer.done() && soldier->speed < soldier->maxSpeedForFiring
						&& (soldier->MeleeTimer.done() || !soldier->melee))
						soldier->ReloadTimer.decrement();
			}
		}
	}
}

void Model::ProjectileHitResolution() {
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
}

void Model::DamageResolution() {
	while(!damages.empty()) {
		DamageTick d = damages.front();
		d.soldier->hp -= d.dmg;
		if(d.soldier->hp <= 0) {
			KillEvent e(d.soldier);
			em->Post(&e);
		}
		damages.pop();
	}
}

void Model::ProjectileCleanup() {
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
}

#endif