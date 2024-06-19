#ifndef FILEIO
#define FILEIO

#include <map.h>
#include <model.h>
#include <view.h>
#include <information.h>

#include <json.hpp>
#include <iostream>
#include <fstream>
#include <list>

using json = nlohmann::json;

json fromFile(std::string filename) {
	std::ifstream in(filename);
	json j;
	in >> j;
	return j;
}

json MapToJson(Map* map) {
	json j;
	j["width"] = map->width;
	j["height"] = map->height;
	j["tilesize"] = map->tilesize;
	json circles = json::array();
	json rectangles = json::array();
	json waypoints = json::array();
	for(auto obj : map->mapObjects) {
		switch(obj->type()) {
		case MAP_CIRCLE: {
			Circle* circ = dynamic_cast<Circle*>(obj);
			json jcirc;
			jcirc["pos"] = {circ->pos.coeff(0), circ->pos.coeff(1)};
			jcirc["rad"] = circ->rad;
			circles.push_back(jcirc);
			break;}
		case MAP_RECTANGLE:
		case MAP_BORDER: {
			Rrectangle* rec = dynamic_cast<Rrectangle*>(obj);
			json jrec;
			jrec["hl"] = rec->hl;
			jrec["hw"] = rec->hw;
			jrec["pos"] = {rec->pos.coeff(0), rec->pos.coeff(1)};
			jrec["rot"] = {rec->rot.coeff(0,0), rec->rot.coeff(0,1), rec->rot.coeff(1,0), rec->rot.coeff(1,1)};
			if(obj->type() == MAP_BORDER) jrec["b"] = 1;
			else jrec["b"] = 0;
			rectangles.push_back(jrec);
			break;}
		case MAP_WAYPOINT: {
			Circle* circ = dynamic_cast<Circle*>(obj);
			json jcirc;
			jcirc["pos"] = {circ->pos.coeff(0), circ->pos.coeff(1)};
			jcirc["rad"] = circ->rad;
			if(dynamic_cast<MapWaypoint*>(obj)->_auto) jcirc["auto"] = 1;
			else jcirc["auto"] = 0;
			waypoints.push_back(jcirc);
			break;}
		}
	}
	j["circles"] = circles;
	j["rectangles"] = rectangles;
	j["waypoints"] = waypoints;
	json wp_dist = json::array();
	for(auto row : map->wp_path_dist) {
		json jrow = json::array();
		for(auto entry : row) {
			jrow.push_back(entry);
		}
		wp_dist.push_back(jrow);
	}
	j["wp_dist"] = wp_dist;
	json wp_next = json::array();
	for(auto row : map->wp_path_next) {
		json jrow = json::array();
		for(auto entry : row) {
			jrow.push_back(entry);
		}
		wp_next.push_back(jrow);
	}
	j["wp_next"] = wp_next;
	return j;
}

Map::Map(std::string filename) {
	json j = fromFile(filename);
	width = j["width"];
	height = j["height"];
	tilesize = j["tilesize"];
	init();
	for(auto jcirc : j["circles"]) {
		Eigen::Vector2d pos; pos << jcirc["pos"][0], jcirc["pos"][1];
		MapCircle* circ = new MapCircle(pos, jcirc["rad"]);
		AddMapObject(circ);
	}
	for(auto jrec : j["rectangles"]) {
		Eigen::Vector2d pos; pos << jrec["pos"][0], jrec["pos"][1];
		Eigen::Matrix2d rot; rot << jrec["rot"][0], jrec["rot"][1], jrec["rot"][2], jrec["rot"][3];
		MapObject* rec;
		if(jrec.contains("b")) {
			if(int(jrec["b"])) rec = new MapBorder(jrec["hl"], jrec["hw"], pos, rot);
			else rec = new MapRectangle(jrec["hl"], jrec["hw"], pos, rot);
		}
		else rec = new MapRectangle(jrec["hl"], jrec["hw"], pos, rot);
		AddMapObject(rec);
	}
	debug("Non-pathfinding map elements loaded successfully.");
	if(j.contains("waypoints")) {
		for(auto jcirc : j["waypoints"]) {
			debug("Loading waypoint.");
			Eigen::Vector2d pos; pos << jcirc["pos"][0], jcirc["pos"][1];
			MapWaypoint* circ = new MapWaypoint(pos, jcirc["rad"]);
			if(jcirc.contains("auto")) {
				if(int(jcirc["auto"])) circ->_auto = true;
			}
			AddMapObject(circ);
		}
		if(j.contains("wp_dist") && j.contains("wp_next")) {
			for(auto jrow : j["wp_dist"]) {
				wp_path_dist.push_back(std::vector<float>());
				for(auto entry : jrow) {
					wp_path_dist.at(wp_path_dist.size()-1).push_back(entry);
				}
			}
			for(auto jrow : j["wp_next"]) {
				wp_path_next.push_back(std::vector<int>());
				for(auto entry : jrow) {
					wp_path_next.at(wp_path_next.size()-1).push_back(entry);
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
}

SettingsInformation::SettingsInformation(json input) {
	damageInfo = AnimationInformation(input["damage_tick"]);
	custom_background = input["custom_background"];
	if(custom_background)
		backgroundInfo = AnimationInformation(input["background_animation"]);
	show_map_object_outlines = input["show_map_object_outlines"];
}

MapEditorSettingsInformation::MapEditorSettingsInformation(json input) {
	custom_background = input["custom_background"];
	if(custom_background)
		backgroundInfo = AnimationInformation(input["background_animation"]);
}
void Model::loadSoldierTypes(std::string filename) {
	json input = fromFile(filename);
	for(auto entry : input) {
		SoldierInformation info = SoldierInformation(entry);
		SoldierTypes.emplace(info.tag, info);
	}
	for(std::map<std::string, SoldierInformation>::iterator it = SoldierTypes.begin(); it != SoldierTypes.end(); ++it) {
		std::cout << "SoldierType key: " << it->first << "\n";
	}
}

void Model::loadUnitTypes(std::string filename) {
	json input = fromFile(filename);
	for(auto entry : input) {
		UnitInformation info = UnitInformation(entry);
		UnitTypes.emplace(info.tag, info);
	}
	for(std::map<std::string, UnitInformation>::iterator it = UnitTypes.begin(); it != UnitTypes.end(); ++it) {
		std::cout << "UnitType key: " << it->first << "\n";
	}
}

void Model::loadArmyLists(std::string filename) {
	json input = fromFile(filename);
	int nplayer = 0;
	for(auto entry : input) {
		for(auto unitName : entry) {
			std::cout << "adding " << unitName << " to player " << nplayer << "\n";
			players[nplayer]->units.push_back(new Unit(UnitTypes.at(unitName), players[nplayer], SoldierTypes));
		}
		nplayer++;
	}
}

void Model::loadDamageInfo() {
	json input = fromFile("config/templates/damage_tick.json");
	damageInfo = AnimationInformation(input);
}

void Model::loadSettings(std::string filename) {
	json input = fromFile(filename);
	settings = SettingsInformation(input);
}

void MapEditorModel::loadSettings(std::string filename) {
	json input = fromFile(filename);
	settings = MapEditorSettingsInformation(input);
}

#endif