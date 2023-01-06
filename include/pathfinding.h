#ifndef PATHFINDING
#define PATHFINDING

#include <map.h>
#include <Dense>

bool FreePath(Eigen::Vector2d p1, Eigen::Vector2d p2, Map* map) {
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
				if(!FreePath(map->waypoints.at(i)->pos, map->waypoints.at(j)->pos, map)) {
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