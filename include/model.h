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
		double time_physics_path = 0;
		double time_physics_order = 0;
		double time_physics_movement = 0;
		double time_physics_move_freepath = 0;
		double time_physics_move_indiv = 0;
		double time_physics_indiv_freepath = 0;
		double time_physics_indiv_findpath = 0;
		double time_physics_indiv_findpath_freepath = 0;
		double time_physics_move_step = 0;
		double time_physics_move_nextorder = 0;
		double time_hitscan = 0;
		double time_indiv_pathing = 0;
		bool displayedTime = false;

		//TileWalker displayWalker;
		std::vector<gridpiece*> displayWalker;
		gridpiece* walkerStart;
		gridpiece* walkerEnd;
		Rrectangle walkerRec;
		Timer walkerTimer = Timer(1500);
		bool hasWalker = false;

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
		void TickResponse();

		Model(EventManager* em, Map* map) : Listener(em) {
			this->map = map;
			dt = &(em->dt);
			state = MODEL_SIMULATION;
		}
	
	private:
		virtual void Notify(Event* ev);
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
	std::cout << "Running on " << omp_get_max_threads() << " threads.\n";

}

void Model::Notify(Event* ev) {
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
		TickResponse();
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
			std::cout << "update target path:      " << time_physics_path << "\n";
			std::cout << "order advancement:       " << time_physics_order << "\n";
			std::cout << "soldier movement:        " << time_physics_movement << "\n";
			std::cout << "movement free path:      " << time_physics_move_freepath << "\n";
			std::cout << "movement indiv path:     " << time_physics_move_indiv << "\n";
			std::cout << "movement indiv freepath: " << time_physics_indiv_freepath << "\n";
			std::cout << "movement indiv findpath: " << time_physics_indiv_findpath << "\n";
			std::cout << "movement findpath fp:    " << time_physics_indiv_findpath_freepath << "\n";
			std::cout << "movement step:           " << time_physics_move_step << "\n";
			std::cout << "movement next order:     " << time_physics_move_nextorder << "\n";
			std::cout << "projectile hit scanning: " << time_hitscan << "\n";
			std::cout << "individual path finding: " << time_indiv_pathing << "\n";
			std::cout << "##################################\n";
			em->showTimes = true;
			displayedTime = true;
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
				unit->Reform();
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
					unit->Reform();
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

void Model::TickResponse() {
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
				auto sub_start = std::chrono::system_clock::now();
				if(unit->orders.at(unit->currentOrder)->_combat) {
					unit->UpdateTargetPath(map, em);
				}
				auto sub_end = std::chrono::system_clock::now();
				if(state != MODEL_GAME_PAUSED)
					time_physics_path += std::chrono::duration<double>(sub_end - sub_start).count();
				//advancing order
				sub_start = std::chrono::system_clock::now();
				if(unit->CurrentOrderCompleted()) {
					unit->AdvanceOrder(map);
				}
				sub_end = std::chrono::system_clock::now();
				if(state != MODEL_GAME_PAUSED)
					time_physics_order += std::chrono::duration<double>(sub_end - sub_start).count();
				//individual movement
				sub_start = std::chrono::system_clock::now();
				unit->SoldierMovement(map, dt, &time_physics_move_freepath, &time_physics_move_indiv, 
					&time_physics_move_step, &time_physics_move_nextorder, &time_physics_indiv_freepath,
					&time_physics_indiv_findpath, &time_physics_indiv_findpath_freepath);
				unit->UpdatePos();
				unit->UpdateVel();
				sub_end = std::chrono::system_clock::now();
				if(state != MODEL_GAME_PAUSED)
					time_physics_movement += std::chrono::duration<double>(sub_end - sub_start).count();
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

	if(hasWalker) {
		if(walkerTimer.decrement()) {
			hasWalker = false;
			walkerTimer.reset();
		}
	}
}

#endif