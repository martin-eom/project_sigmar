#ifndef PATHFINDING
#define PATHFINDING

#include <map.h>
#include <Dense>

bool FreePath(Circle* w1, Circle* w2, Map* map) {
	Eigen::Vector2d p1 = w1->pos;
	Eigen::Vector2d p2 = w2->pos;
	Point u(p1);
	Point v(p2);
	bool collision = false;
	for(auto obj : map->mapObjects) {
		switch(obj->type()) {
		case MAP_CIRCLE:
			if(LineCircleCollision(&u, &v, dynamic_cast<Circle*>(obj))) collision = true;
			break;
		case MAP_RECTANGLE:
			if(LineRectangleCollison(&u, &v, dynamic_cast<Rrectangle*>(obj))) collision = true;
			break;
		}
		if(collision) break;
	}
	Eigen::Vector2d diff = p2 - p1;
	double hw = std::min(w1->rad, w2->rad) * 0.8;
	double hl = diff.norm() / 2;
	double cos = diff.coeff(0) / diff.norm();
	double sin = diff.coeff(1) / diff.norm();
	Eigen::Matrix2d rot; rot << cos, -sin, sin, cos;
	Rrectangle rec = Rrectangle(hw, hl, p1 + 0.5*diff, rot);
	for(auto obj : map->mapObjects) {
		switch(obj->type()) {
		case MAP_CIRCLE:
			if(CircleRectangleCollision(dynamic_cast<Circle*>(obj), &rec)) collision = true;
			break;
		case MAP_RECTANGLE:
			if(RectangleRectangleCollision(dynamic_cast<Rrectangle*>(obj), &rec)) collision = true;
			break;
		}
		if(collision) break;
	}
	return !collision;
}

void FloydWarshallWithPathReconstruction(Map* map) {
	map->wp_path_dist.clear();
	map->wp_path_next.clear();
	for(int i = 0; i < map->waypoints.size(); i++) {
		map->wp_path_dist.push_back(std::vector<float>());
		map->wp_path_next.push_back(std::vector<int>());
		for(int j = 0; j < map->waypoints.size(); j++) {
			if(i == j) {
				map->wp_path_dist.at(i).push_back(0);
				map->wp_path_next.at(i).push_back(i);
			}
			else {
				if(!FreePath(map->waypoints.at(i), map->waypoints.at(j), map)) {
					map->wp_path_dist.at(i).push_back(std::numeric_limits<float>::infinity());
					map->wp_path_next.at(i).push_back(NULL);
				}
				else {
					map->wp_path_dist.at(i).push_back((map->waypoints.at(i)->pos - map->waypoints.at(j)->pos).norm());
					map->wp_path_next.at(i).push_back(j);
				}
			}
		}
	}// that was the initialization
	debug("Beginning the Floyd Warshall algorithm...");
	for(int k = 0; k < map->waypoints.size(); k++) {
		for(int i = 0; i < map->waypoints.size(); i++) {
			for(int j = 0; j < map->waypoints.size(); j++) {
				float dist_k = map->wp_path_dist.at(i).at(k) + map->wp_path_dist.at(k).at(j);
				if(map->wp_path_dist.at(i).at(j) > dist_k) {
					map->wp_path_dist.at(i).at(j) = dist_k;
					map->wp_path_next.at(i).at(j) = map->wp_path_next.at(i).at(k);
				}
			}
		}
	}
	debug("Finished the Floyd Warshall algorithm!");
}

#endif