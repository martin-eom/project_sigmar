#ifndef FILEIO
#define FILEIO

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
			rectangles.push_back(jrec);
			break;}
		}
	}
	j["circles"] = circles;
	j["rectangles"] = rectangles;
	std::cout << j;
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
		MapRectangle* rec = new MapRectangle(jrec["hl"], jrec["hw"], pos, rot);
		AddMapObject(rec);
	}
}

#endif