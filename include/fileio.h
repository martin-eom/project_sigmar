#ifndef OLDFILEIO
#define OLDFILEIO

#include <map.h>

#include <json.hpp>
#include <iostream>
#include <fstream>
#include <list>

using json = nlohmann::json;

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
	std::ifstream in(filename);
	json j;
	in >> j;
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

#endif