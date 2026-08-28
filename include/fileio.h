#ifndef FILEIO
#define FILEIO

#include <map.h>
#include <model.h>
#include <view.h>
#include <information.h>
#include <simple_ai.h>

#include <json.hpp>
#include <iostream>
#include <fstream>
#include <list>
#include <algorithm>
#include <random>

using json = nlohmann::json;

json fromFile(std::string filename) {
	std::ifstream in(filename);
	json j;
	in >> j;
	return j;
}

void getMapObjects(json* j, Map* map);
void getPathInfo(json* j, Map* map);

json MapToJson(Map* map) {
	json j;
	j["width"] = map->width;
	j["height"] = map->height;
	getMapObjects(&j, map);
	getPathInfo(&j, map);
	return j;
}

void readMapObjectsFromJSON(json* j, Map* map);
void readPathInfoFromJSON(json* j, Map* map);

Map::Map(std::string filename) {
	json j = fromFile(filename);
	width = j["width"];
	height = j["height"];
	init();
	readMapObjectsFromJSON(&j, this);
	readPathInfoFromJSON(&j, this);
}

void getMapObjects(json* j, Map* map) {
	json circles = json::array();
	json triangles = json::array();
	json rectangles = json::array();
	json waypoints = json::array();
	json deployment_zones = json::array();
	for(auto obj : map->mapObjects) {
		switch(obj->type) {
		case MAP_CIRCLE: {
			Circle* circ = dynamic_cast<Circle*>(obj);
			json jcirc;
			jcirc["pos"] = {circ->pos.coeff(0), circ->pos.coeff(1)};
			jcirc["rad"] = circ->rad;
			jcirc["high"] = obj->high;
			circles.push_back(jcirc);
			break;}
		case MAP_TRIANGLE: {
			Triangle* tri = dynamic_cast<Triangle*>(obj);
			json jtri;
			jtri["a"] = tri->a;
			jtri["b"] = tri->b;
			jtri["gamma"] = tri->gamma;
			jtri["pos"] = {tri->pos.coeff(0), tri->pos.coeff(1)};
			jtri["rot"] = {tri->rot.coeff(0,0), tri->rot.coeff(0,1), tri->rot.coeff(1,0), tri->rot.coeff(1,1)};
			jtri["high"] = obj->high;
			triangles.push_back(jtri);
			break;}
		case MAP_RECTANGLE:
		case MAP_BORDER: {
			Rrectangle* rec = dynamic_cast<Rrectangle*>(obj);
			json jrec;
			jrec["hl"] = rec->hl;
			jrec["hw"] = rec->hw;
			jrec["pos"] = {rec->pos.coeff(0), rec->pos.coeff(1)};
			jrec["rot"] = {rec->rot.coeff(0,0), rec->rot.coeff(0,1), rec->rot.coeff(1,0), rec->rot.coeff(1,1)};
			if(obj->type == MAP_BORDER) jrec["b"] = 1;
			else jrec["b"] = 0;
			jrec["high"] = obj->high;
			rectangles.push_back(jrec);
			break;}
		case MAP_WAYPOINT: {
			Circle* circ = dynamic_cast<Circle*>(obj);
			json jcirc;
			jcirc["pos"] = {circ->pos.coeff(0), circ->pos.coeff(1)};
			jcirc["rad"] = circ->rad;
			if(dynamic_cast<MapWaypoint*>(obj)->_auto) jcirc["auto"] = 1;
			else jcirc["auto"] = 0;
			jcirc["high"] = obj->high;
			waypoints.push_back(jcirc);
			break;}
		}
	}
	for(auto dz : map->deploymentZones) {
		json jdz;
		jdz["hl"] = dz->hl;
		jdz["hw"] = dz->hw;
		jdz["pos"] = {dz->pos.coeff(0), dz->pos.coeff(1)};
		jdz["rot"] = {dz->rot.coeff(0,0), dz->rot.coeff(0,1), dz->rot.coeff(1,0), dz->rot.coeff(1,1)};
		jdz["id"] = dz->player_id;
		deployment_zones.push_back(jdz);
	}
	(*j)["circles"] = circles;
	(*j)["triangles"] = triangles;
	(*j)["rectangles"] = rectangles;
	(*j)["waypoints"] = waypoints;
	(*j)["deployment_zones"] = deployment_zones;
}

void getPathInfo(json* j, Map* map) {
	json wp_dist = json::array();
	for(auto row : map->wp_path_dist) {
		json jrow = json::array();
		for(auto entry : row) {
			jrow.push_back(entry);
		}
		wp_dist.push_back(jrow);
	}
	(*j)["wp_dist"] = wp_dist;
	json wp_next = json::array();
	for(auto row : map->wp_path_next) {
		json jrow = json::array();
		for(auto entry : row) {
			jrow.push_back(entry);
		}
		wp_next.push_back(jrow);
	}
	(*j)["wp_next"] = wp_next;
}

void readMapObjectsFromJSON(json* j, Map* map) {
	for(auto jcirc : (*j)["circles"]) {
		Eigen::Vector2d pos; pos << jcirc["pos"][0], jcirc["pos"][1];
		MapCircle* circ = new MapCircle(pos, jcirc["rad"]);
		if(jcirc.contains("high"))
			circ->high = jcirc["high"];
		map->AddMapObject(circ);
	}
	if(j->contains("triangles")) {
		for(auto jtri : (*j)["triangles"]) {
			Eigen::Vector2d pos; pos << jtri["pos"][0], jtri["pos"][1];
			Eigen::Matrix2d rot; rot << jtri["rot"][0], jtri["rot"][1], jtri["rot"][2], jtri["rot"][3];
			MapObject* tri = new MapTriangle(jtri["a"], jtri["b"], jtri["gamma"], pos, rot);
			if(jtri.contains("high"))
				tri->high = jtri["high"];
			map->AddMapObject(tri);
		}
	}
	for(auto jrec : (*j)["rectangles"]) {
		Eigen::Vector2d pos; pos << jrec["pos"][0], jrec["pos"][1];
		Eigen::Matrix2d rot; rot << jrec["rot"][0], jrec["rot"][1], jrec["rot"][2], jrec["rot"][3];
		MapObject* rec;
		if(jrec.contains("b")) {
			if(int(jrec["b"])) rec = new MapBorder(jrec["hl"], jrec["hw"], pos, rot);
			else rec = new MapRectangle(jrec["hl"], jrec["hw"], pos, rot);
		}
		else rec = new MapRectangle(jrec["hl"], jrec["hw"], pos, rot);
		if(jrec.contains("high"))
			rec->high = jrec["high"];
		map->AddMapObject(rec);
	}
	if((*j).contains("deployment_zones")){
		for(auto jdz : (*j)["deployment_zones"]) {
			Eigen::Vector2d pos; pos << jdz["pos"][0], jdz["pos"][1];
			Eigen::Matrix2d rot; rot << jdz["rot"][0], jdz["rot"][1], jdz["rot"][2], jdz["rot"][3];
			DeploymentZone* dz = new DeploymentZone(jdz["hl"], jdz["hw"], pos, rot, jdz["id"]);
			map->AddMapObject(dz);
		}
	}
}

void readPathInfoFromJSON(json* j, Map* map) {
	if((*j).contains("waypoints")) {
		for(auto jcirc : (*j)["waypoints"]) {
			Eigen::Vector2d pos; pos << jcirc["pos"][0], jcirc["pos"][1];
			MapWaypoint* circ = new MapWaypoint(pos, jcirc["rad"]);
			if(jcirc.contains("auto")) {
				if(int(jcirc["auto"])) circ->_auto = true;
			}
			map->AddMapObject(circ);
		}
		if((*j).contains("wp_dist") && (*j).contains("wp_next")) {
			for(auto jrow : (*j)["wp_dist"]) {
				map->wp_path_dist.push_back(std::vector<float>());
				for(auto entry : jrow) {
					map->wp_path_dist.at(map->wp_path_dist.size()-1).push_back(entry);
				}
			}
			for(auto jrow : (*j)["wp_next"]) {
				map->wp_path_next.push_back(std::vector<int>());
				for(auto entry : jrow) {
					map->wp_path_next.at(map->wp_path_next.size()-1).push_back(entry);
				}
			}
		}
	}
}


AnimationInformation::AnimationInformation(json input) {
	if(input.contains("texture")) {
		texture = input["texture"];
		textureBlue = input["texture"];
		textureRed = input["texture"];
	}
	else if(input.contains("texture_blue")) {
		texture = input["texture_blue"];
		textureBlue = input["texture_blue"];
		textureRed = input["texture_red"];
	}
	size_x = input["size_x"];
	size_y = input["size_y"];
	if(input.contains("center_x")) { center_x = input["center_x"]; }
	else { center_x = size_x / 2; }
	if(input.contains("center_y")) { center_y = input["center_y"]; }
	else { center_y = size_y / 2; }
	if(input.contains("frame_size_x")) { frame_size_x = input["frame_size_x"]; }
	else { frame_size_x = size_x; }
	if(input.contains("frame_size_y")) { frame_size_y = input["frame_size_y"]; }
	else { frame_size_y = size_y; }
	if(input.contains("frame_origin_x")) { frame_origin_x = input["frame_origin_x"]; }
	else { frame_origin_x = (size_x - frame_size_x) / 2; }
	if(input.contains("frame_origin_y")) { frame_origin_y = input["frame_origin_y"]; }
	else { frame_origin_y = (size_y - frame_size_y); }
	if(input.contains("num_frames")) { num_frames = input["num_frames"]; }
	else { num_frames = 5; }
	if(input.contains("step")) { step = input["step"]; }
	if(input.contains("ticks_per_frame")) { ticks_per_frame = input["ticks_per_frame"]; }
	else ticks_per_frame = 5;
	if(input.contains("length")) { length = input["length"]; }
};

SoldierInformation::SoldierInformation(json input) {
	tag = input["tag"];
	radius = input["general_stats"]["radius"];
	mass = input["general_stats"]["mass"];
	max_speed = input["general_stats"]["max_speed"];
	acceleration = input["general_stats"]["acceleration"];
	turn_speed = input["general_stats"]["turn_speed"];
	on_target_dampening = input["general_stats"]["on_target_dampening"];
	max_hp = input["general_stats"]["max_hp"];
	armor = input["general_stats"]["armor"];

	melee_melee = input["melee_stats"]["melee"];
	melee_range = input["melee_stats"]["range"];
	melee_angle = input["melee_stats"]["angle"];
	melee_cooldown = input["melee_stats"]["cooldown"];
	melee_aoe = input["melee_stats"]["aoe"];
	melee_attack = input["melee_stats"]["attack"];
	melee_defense = input["melee_stats"]["defense"];
	melee_armor_piercing = input["melee_stats"]["armor_piercing"];
	melee_damage = input["melee_stats"]["damage"];

	ranged_ranged = input["ranged_stats"]["ranged"];
	ranged_range = input["ranged_stats"]["range"];
	ranged_min_range = input["ranged_stats"]["min_range"];
	ranged_radius = input["ranged_stats"]["radius"];
	ranged_heavy = input["ranged_stats"]["heavy"];
	ranged_draw_timer = input["ranged_stats"]["draw_timer"];
	ranged_reload_timer = input["ranged_stats"]["reload_timer"];
	ranged_max_speed_for_firing = input["ranged_stats"]["max_speed_for_firing"];
	ranged_defense = input["ranged_stats"]["defense"];
	ranged_projectile_speed = input["ranged_stats"]["projectile_speed"];
	ranged_armor_piercing = input["ranged_stats"]["armor_piercing"];
	ranged_ally_protection_aoe = input["ranged_stats"]["ally_protection_aoe"];
	ranged_aoe = input["ranged_stats"]["aoe"];
	ranged_damage = input["ranged_stats"]["damage"];

	if(input["keywords"].contains("infantry"))
		kw_infantry = input["keywords"]["infantry"];
	else kw_infantry = false;
	if(input["keywords"].contains("large"))
		kw_large = input["keywords"]["large"];
	else kw_large = false;
	if(input["keywords"].contains("anti_infantry"))
		kw_anti_infantry = input["keywords"]["anti_infantry"];
	else kw_anti_infantry = false;
	if(input["keywords"].contains("anti_large"))
		kw_anti_large = input["keywords"]["anti_large"];
	else kw_anti_large = false;

	anime_legs_information = AnimationInformation(input["textures"]["legs"]);
	anime_melee_information = AnimationInformation(input["textures"]["arms"]);
	anime_ranged_information = AnimationInformation(input["textures"]["ranged"]);
	anime_body_information = AnimationInformation(input["textures"]["body"]);
	if(input["textures"].contains("projectile"))
		anime_projectile_information = AnimationInformation(input["textures"]["projectile"]);
}

UnitInformation::UnitInformation(json input) {
	tag = input["tag"];
	soldier_type = input["class"];

	formation_max_soldiers = input["formation"]["maxSoldiers"];
	formation_rows = input["formation"]["rows"];
	formation_columns = input["formation"]["columns"];
	formation_x_spacing = input["formation"]["xspacing"];
	formation_y_spacing = input["formation"]["yspacing"];

	ranged_ranged = input["ranged_stats"]["ranged"];
	ranged_range = input["ranged_stats"]["range"];
	ranged_angle = input["ranged_stats"]["angle"];
	primary_ranged = input["ranged_stats"]["primary_ranged"];
}

SettingsInformation::SettingsInformation(json input) {
	damageInfo = AnimationInformation(input["damage_tick"]);
	custom_background = input["custom_background"];
	if(custom_background)
		backgroundInfo = AnimationInformation(input["background_animation"]);
	show_map_object_outlines = input["show_map_object_outlines"];
	show_all_unit_orders = input["show_all_unit_orders"];
	turn_duration = input["turn_duration"];
	simulation_mode = input["simulation_mode"];
	base_melee_attack = input["base_melee_attack"];
	ranged_base_attack = input["ranged_base_attack"];
	max_hit_chance = input["max_hit_chance"];
	min_hit_chance = input["min_hit_chance"];
	anti_large_attack_bonus = input["anti_large_attack_bonus"];
	anti_large_damage_bonus = input["anti_large_damage_bonus"];
	anti_infantry_attack_bonus = input["anti_infantry_attack_bonus"];
	anti_infantry_damage_bonus = input["anti_infantry_damage_bonus"];
	auto_generate_map_grids = input["auto_generate_map_grids"];
	for(auto size: input["map_grids"])
		map_grids.push_back(size);
	set_custom_omp_num_threads = input["set_custom_omp_num_threads"];
	custom_omp_num_threads = input["custom_omp_num_threads"];
	player1_type = Player::playerTypeDict[input["player1_type"]];
	player2_type = Player::playerTypeDict[input["player2_type"]];
}

MapEditorSettingsInformation::MapEditorSettingsInformation(json input) {
	custom_background = input["custom_background"];
	if(custom_background)
		backgroundInfo = AnimationInformation(input["background_animation"]);
}
void Model::LoadSoldierTypes(std::string filename) {
	json input = fromFile(filename);
	for(auto entry : input) {
		SoldierInformation info = SoldierInformation(entry);
		SoldierTypes.emplace(info.tag, info);
	}
}

void Model::LoadUnitTypes(std::string filename) {
	json input = fromFile(filename);
	for(auto entry : input) {
		UnitInformation info = UnitInformation(entry);
		UnitTypes.emplace(info.tag, info);
	}
}

void Model::LoadArmyLists(std::string filename) {
	json input = fromFile(filename);
	int nplayer = 0;
	for(auto entry : input) {
		for(auto unitName : entry) {
			std::cout << "adding " << unitName << " to player " << nplayer << "\n";
			Unit* unit = new Unit(UnitTypes.at(unitName), players[nplayer], SoldierTypes);
			players[nplayer]->units.push_back(unit);
			units.push_back(unit);
			unit->model_index = units.size() - 1;
			unit_locks.push_back(new omp_lock_t());
			omp_init_lock(unit_locks.at(unit_locks.size() - 1));
			for(auto soldier : unit->liveSoldiers) {
				soldiers.push_back(soldier);
				soldier->model_index = soldiers.size() - 1;
				soldier_locks.push_back(new omp_lock_t());
				omp_init_lock(soldier_locks.at(soldier_locks.size() - 1));
			}
		}
		nplayer++;
	}
	std::sort(units.begin(), units.end(), UnitSorter());
	std::reverse(units.begin(), units.end());
	unsigned seed = std::chrono::system_clock::now().time_since_epoch().count();
	auto rng = std::default_random_engine(seed);
	std::ranges::shuffle(soldiers,  rng);
	for(auto ai : simpleAIs) {
		ai->InitTargets();
	}
}

void Model::LoadDamageInfo() {
	json input = fromFile("config/templates/damage_tick.json");
	damageInfo = AnimationInformation(input);
}

void Model::LoadSettings(std::string filename) {
	json input = fromFile(filename);
	settings = SettingsInformation(input);
	if(settings.simulation_mode) state = MODEL_SIMULATION;
	else state = MODEL_GAME_READY_TO_START;
	toNextState.set_max(settings.turn_duration);
}

void MapEditorModel::loadSettings(std::string filename) {
	json input = fromFile(filename);
	settings = MapEditorSettingsInformation(input);
}

std::string Map::neighbourFileStr() {
	std::string filename = "temp/";
	filename += std::to_string(width) + "_" + std::to_string(height) + "-";
	if(grids.size() > 0) {
		filename += std::to_string(grids.at(0)->tilesize);
		for(int n_grid = 1; n_grid < grids.size(); n_grid++) {
			filename += "_" + std::to_string(grids.at(n_grid)->tilesize);
		}
	}
	filename += ".nm";
	return filename;
}

bool Map::searchNeighbourFile(std::string filename) {
	return std::filesystem::exists(filename);
}

void Map::writeNeighbourFile(std::string filename) {
	std::ofstream out(filename);
	for(int ngrid = 0; ngrid < grids.size(); ngrid++) {
		for(int nrow = 0; nrow < grids.at(ngrid)->nrows; nrow++) {
			for(int ncol = 0; ncol < grids.at(ngrid)->ncols; ncol++) {
				auto tile = grids.at(ngrid)->grid.at(nrow).at(ncol);
				for(int ngrid2 = 0; ngrid2 < grids.size(); ngrid2++) {
					int nneighbours = tile->neighbours.at(ngrid2).size();
					out << nneighbours << "\n";
					for(int nn = 0; nn < nneighbours; nn++) {
						out << tile->neighbours.at(ngrid2).at(nn)->nrow * grids.at(ngrid2)->ncols 
							+ tile->neighbours.at(ngrid2).at(nn)->ncol << "\n";
					}
					int nRedNeighbours = tile->redundantNeighbours.at(ngrid2).size();
					out << nRedNeighbours << "\n";
					for(int nrn = 0; nrn < nRedNeighbours; nrn++) {
						out << tile->redundantNeighbours.at(ngrid2).at(nrn)->nrow * grids.at(ngrid2)->ncols
							+ tile->redundantNeighbours.at(ngrid2).at(nrn)->ncol << "\n";
					}
				}
			}
		}
	}
	out.close();
}

void Map::readNeighbourFile(std::string filename) {
	std::string line;
	int nentries, val;
	std::ifstream in(filename);
	for(int ngrid = 0; ngrid < grids.size(); ngrid++) {
		for(int nrow = 0; nrow < grids.at(ngrid)->nrows; nrow++) {
			for(int ncol = 0; ncol < grids.at(ngrid)->ncols; ncol++) {
				auto tile = grids.at(ngrid)->grid.at(nrow).at(ncol);
				for(int ngrid2 = 0; ngrid2 < grids.size(); ngrid2++) {
					std::getline(in, line);
					nentries = atoi(line.c_str());
					int nrow2, ncol2;
					for(int nentry = 0; nentry < nentries; nentry++) {
						std::getline(in, line);
						val = atoi(line.c_str());
						nrow2 = val / grids.at(ngrid2)->ncols;
						ncol2 = val % grids.at(ngrid2)->ncols;
						tile->neighbours.at(ngrid2).push_back(grids.at(ngrid2)->grid.at(nrow2).at(ncol2));
					}
					std::getline(in, line);
					nentries = atoi(line.c_str());
					for(int nentry = 0; nentry < nentries; nentry++) {
						std::getline(in, line);
						val = atoi(line.c_str());
						nrow2 = val / grids.at(ngrid2)->ncols;
						ncol2 = val % grids.at(ngrid2)->ncols;
						tile->redundantNeighbours.at(ngrid2).push_back(grids.at(ngrid2)->grid.at(nrow2).at(ncol2));
					}
				}
			}
		}
	}
	in.close();
}

#endif