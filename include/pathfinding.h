#ifndef PATHFINDING
#define PATHFINDING

#include <map.h>
#include <model.h>
#include <Eigen/Dense>


enum TILEWALKER_DIRECTION {
	TW_WIDE_UP,
	TW_WIDE_DOWN,
	TW_TALL_UP,
	TW_TALL_DOWN
};

class TileWalker {
public:
	Rrectangle rec;
	grid_container* grid;
	gridpiece* start = NULL;
	gridpiece* end = NULL;
	gridpiece* current = NULL;
	std::vector<gridpiece*> walkedTiles;
	Straight line1;
	Straight line2;
	int direction;
	int path; //current row (tall) or column (wide) index
	int lower_bound; // first column (tall) or row (wide) index for current path
	int upper_bound; // last bound column (tall) or row (wide) index for current path
	int current_ind; // current column (tall) or row (wide) index
	int start_path;
	int end_path;
	int end_ind;
	int cnt = 0;
	bool _dontShow;

	gridpiece* Next();
	bool End();

	void CreateRectangle(Circle* w1, Circle* w2);
	void SetLines();
	void SetDirection();
	void SetStartPoint();
	void SetEndPoint();

	TileWalker() {}
	TileWalker(Circle* w1, Circle* w2, Map* map, bool dontShow = true) {
		_dontShow = dontShow;
		CreateRectangle(w1, w2);
		grid = map->grids.back();
		SetLines();
		SetStartPoint();
		SetEndPoint();
		if(!_dontShow)
			std::cout << "end_path: " << end_path << ", end_ind: " << end_ind << ", nrows: " << grid->nrows << ", ncols: " << grid->ncols << " " << rec.corners.at(1).x() << "\n";
	}

};

bool TileWalker::End() {
	return (current == end);
}

void TileWalker::CreateRectangle(Circle* w1, Circle* w2) {
		Circle* c1;
		Circle* c2;
		if(w1->pos.x() < w2->pos.x()) {
			c1 = w1;
			c2 = w2;
		}
		else if(w1->pos.x() > w2->pos.x()) {
			c1 = w2;
			c2 = w1;
		}
		else if(w1->pos.y() < w2->pos.y()) {
			c1 = w1;
			c2 = w2;
		}
		else {
			c1 = w2;
			c2 = w1;
		}
		Eigen::Vector2d diff = c2->pos - c1->pos;
		double hw = std::min(c1->rad, c2->rad) * 0.8;
		double hl = diff.norm() / 2;
		double cos = diff.coeff(0) / diff.norm();
		double sin = diff.coeff(1) / diff.norm();
		Eigen::Matrix2d rot; rot << cos, -sin, sin, cos;
		rec = Rrectangle(hw, hl, c1->pos + 0.5*diff, rot);
		//std::cout << "Rectangle corenrs:\n";
		//std::cout << rec.corners.at(0).pos << "\n";
		//std::cout << rec.corners.at(1).pos << "\n";
		//std::cout << rec.corners.at(2).pos << "\n";
		//std::cout << rec.corners.at(3).pos << "\n";
}

void TileWalker::SetLines() {
	line1 = Straight(&rec.corners.at(0), &rec.corners.at(1));
	line2 = Straight(&rec.corners.at(3), &rec.corners.at(2));
	SetDirection();
}

void TileWalker::SetDirection() {
	if(line1.vertical) {
		direction = TW_TALL_UP;
	}
	else if(abs(line1.a) > 1) {
		if(line1.a < 0)
			direction = TW_TALL_DOWN;
		else
			direction = TW_TALL_UP;
	}
	else if(line1.a < 0)
		direction = TW_WIDE_DOWN;
	else
		direction = TW_WIDE_UP;
}

void TileWalker::SetEndPoint() {
	Eigen::Vector2d end_point = rec.corners.at(2).pos;
	int end_row = (int) (end_point.y() / grid->tilesize);
	end_row = legalizeIndex(end_row, grid->nrows);
	int end_col = (int) (end_point.x() / grid->tilesize);
	end_col = legalizeIndex(end_col, grid->ncols);
	end = grid->grid.at(end_row).at(end_col);

	switch(direction) {
	case TW_WIDE_UP:
		end_row = (int) (line2.y(end->rec->pos.x() - 0.5*grid->tilesize) / grid->tilesize);
		end_row = legalizeIndex(end_row, grid->nrows);
		end_path = end_col;
		end_ind = end_row;
		break;
	case TW_WIDE_DOWN:
		end_row = (int) (line2.y(rec.corners.at(1).pos.x()) / grid->tilesize);
		//end_row = (int) (line2.y((end_row+1)*grid->tilesize) / grid->tilesize);
		//end_row = legalizeIndex(end_row, grid->nrows);
		end_col = (int) (rec.corners.at(1).x() / grid->tilesize);
		end_col = legalizeIndex(end_col, grid->ncols);
		end_row = (int) (line2.y((end_col+1)*grid->tilesize) / grid->tilesize);
		end_row = legalizeIndex(end_row, grid->nrows);
		end_path = end_col;
		end_ind = end_row;
		break;
	case TW_TALL_UP:
		end_row = (int) (rec.corners.at(1).pos.y() / grid->tilesize);
		end_row = legalizeIndex(end_row, grid->nrows);
		//end_col = (int) (line2.x(rec.corners.at(1).pos.y()) / grid->tilesize);
		end_col = (int) (line2.x((end_row+1)*grid->tilesize) / grid->tilesize);
		end_col = legalizeIndex(end_col, grid->ncols);
		end_path = end_row;
		end_ind = end_col;
		break;
	case TW_TALL_DOWN:
		end_col = (int) (line2.x(end->rec->pos.y() + 0.5*grid->tilesize) / grid->tilesize);
		end_col = legalizeIndex(end_col, grid->ncols);
		end_path = end_row;
		end_ind = end_col;
		break;
	}
	end = grid->grid.at(end_row).at(end_col);
}

void TileWalker::SetStartPoint() {
	Eigen::Vector2d start_point = rec.corners.at(0).pos;
	int start_row = (int) (start_point.y() / grid->tilesize);
	start_row = legalizeIndex(start_row, grid->nrows);
	//std::cout << "initial starting row: " << start_row << "\n";
	int start_col = (int) (start_point.x() / grid->tilesize);
	start_col = legalizeIndex(start_col, grid->ncols);
	current = grid->grid.at(start_row).at(start_col);

	switch(direction) {
	case TW_WIDE_UP:
		start_row = (int) (line1.y(current->rec->pos.x() + 0.5*grid->tilesize) / grid->tilesize);
		start_row = legalizeIndex(start_row, grid->nrows);
		path = start_col;
		lower_bound = start_row;
		current_ind = lower_bound;
		upper_bound = (int) (line2.y(start_col*grid->tilesize) / grid->tilesize);
		upper_bound = legalizeIndex(upper_bound, grid->nrows);
		break;
	case TW_WIDE_DOWN:
		//start_row = (int) (line1.y(rec.corners.at(3).pos.x()) / grid->tilesize);
		//start_row = legalizeIndex(start_row, grid->nrows);
		start_col = (int) (rec.corners.at(3).x() / grid->tilesize);
		start_col = legalizeIndex(start_col, grid->ncols);
		start_row = (int) (line1.y(start_col*grid->tilesize) / grid->tilesize);
		start_row = legalizeIndex(start_row, grid->nrows);
		path = start_col;
		lower_bound = start_row;
		upper_bound = (int) (line2.y((start_col+1)*grid->tilesize) / grid->tilesize);
		upper_bound = legalizeIndex(upper_bound, grid->nrows);
		break;
	case TW_TALL_UP:
		start_row = (int) (rec.corners.at(3).pos.y() / grid->tilesize);
		start_row = legalizeIndex(start_row, grid->nrows);
		//start_col = (int) (line1.x(rec.corners.at(3).pos.y()) / grid->tilesize);
		start_col = (int) (line1.x(start_row*grid->tilesize) / grid->tilesize);
		start_col = legalizeIndex(start_col, grid->ncols);
		path = start_row;
		lower_bound = start_col;
		upper_bound = (int) (line2.x((start_row+1)*grid->tilesize) / grid->tilesize);
		upper_bound = legalizeIndex(upper_bound, grid->ncols);
		break;
	case TW_TALL_DOWN:
		start_col = (int) (line1.x(current->rec->pos.y() - 0.5*grid->tilesize) / grid->tilesize);
		start_col = legalizeIndex(start_col, grid->ncols);
		path = start_row;
		lower_bound = start_col;
		upper_bound = (int) (line2.x(current->rec->pos.y() + 0.5*grid->tilesize) / grid->tilesize);
		upper_bound = legalizeIndex(upper_bound, grid->ncols);
		break;
	}
	//std::cout << "final start row: " << start_row << "\n";
	current = grid->grid.at(start_row).at(start_col);
	start = grid->grid.at(start_row).at(start_col);
	current_ind = lower_bound;
	start_path = path;
}

gridpiece* TileWalker::Next() {
	//std::cout << "direction: " << direction << ", step: " << cnt << ", lbound: " << lower_bound << ", ubound:" << upper_bound << ", ind: " << current_ind << ", path: " << path << "\n";
	if(!_dontShow) {
		walkedTiles.push_back(current);
		std::cout << direction << " " << path << " " << current_ind << " " << end_path << " " << end_ind << "\n";
	}
	//std::cout << "if nothing disrupts the previous message we are stuck!\n";
	if(current_ind == end_ind && path == end_path) {
		current = NULL;
		//std::cout << "Tilewalker walked " << cnt << " steps!\n";
	}
	else {
		cnt++;
		switch(direction) {
		case TW_WIDE_UP:
			if(current_ind > upper_bound)
				current_ind--;
			else {
				path++;
				lower_bound = (int) (line1.y((path+1)*grid->tilesize) / grid->tilesize);
				lower_bound = legalizeIndex(lower_bound, grid->nrows);
				upper_bound = (int) (line2.y(path*grid->tilesize) / grid->tilesize);
				upper_bound = legalizeIndex(upper_bound, grid->nrows);
				current_ind = lower_bound;
			}
			break;
		case TW_WIDE_DOWN:
			if(current_ind > upper_bound)
				current_ind--;
			else {
				path++;
				lower_bound = (int) (line1.y(path*grid->tilesize) / grid->tilesize);
				lower_bound = legalizeIndex(lower_bound, grid->nrows);
				upper_bound = (int) (line2.y((path+1)*grid->tilesize) / grid->tilesize);
				//std::cout << (path+1)*grid->tilesize << "\n";
				upper_bound = legalizeIndex(upper_bound, grid->nrows);
				current_ind = lower_bound;
			}
			break;
		case TW_TALL_UP:
			if(current_ind < upper_bound)
				current_ind++;
			else {
				path++;
				lower_bound = (int) (line1.x(path*grid->tilesize) / grid->tilesize);
				lower_bound = legalizeIndex(lower_bound, grid->ncols);
				upper_bound = (int) (line2.x((path+1)*grid->tilesize) / grid->tilesize);
				upper_bound = legalizeIndex(upper_bound, grid->ncols);
				current_ind = lower_bound;
			}
			break;
		case TW_TALL_DOWN:
			if(current_ind > upper_bound)
				current_ind--;
			else {
				path--;
				//std::cout << "path: " << path << ", endpath: " << end_path << ", start_path: " << start_path << "\n";
				lower_bound = (int) (line1.x(path*grid->tilesize) / grid->tilesize);
				lower_bound = legalizeIndex(lower_bound, grid->ncols);
				upper_bound = (int) (line2.x((path+1)*grid->tilesize) / grid->tilesize);
				upper_bound = legalizeIndex(upper_bound, grid->ncols);
				current_ind = lower_bound;
			}
			break;
		}
		//std::cout << "We all know... " << path << " " << current_ind << "\n";
		switch(direction) {
		case TW_WIDE_UP:
		case TW_WIDE_DOWN:
			if(current_ind < 0 || current_ind >= grid->nrows || path < 0 || path >= grid->ncols)
				current = NULL;
			else
				current = grid->grid.at(current_ind).at(path);
			break;
		case TW_TALL_UP:
		case TW_TALL_DOWN:
			if(path < 0 || path >= grid->nrows || current_ind < 0 || current_ind >= grid->ncols)
				current = NULL;
			else
				current = grid->grid.at(path).at(current_ind);
			break;
		}
		//std::cout << "...where the problem lies.\n";
	}
	return current;
}

bool OldFreePath(Circle* w1, Circle* w2, Map* map, bool hightSensitive = false, bool dontShow = true, Model* model = NULL) {
	Eigen::Vector2d p1 = w1->pos;
	Eigen::Vector2d p2 = w2->pos;
	Point u(p1);
	Point v(p2);
	bool collision = false;
	Eigen::Vector2d diff = p2 - p1;
	double hw = std::min(w1->rad, w2->rad) * 0.8;
	double hl = diff.norm() / 2;
	double cos = diff.coeff(0) / diff.norm();
	double sin = diff.coeff(1) / diff.norm();
	Eigen::Matrix2d rot; rot << cos, -sin, sin, cos;
	Rrectangle rec = Rrectangle(hw, hl, p1 + 0.5*diff, rot);
	for(auto obj : map->mapObjects) {
		if(!hightSensitive || obj->high) {
			switch(obj->type) {
			case MAP_CIRCLE:
				if(CircleRectangleCollision(dynamic_cast<Circle*>(obj), &rec)) collision = true;
				break;
			case MAP_RECTANGLE:
			case MAP_TRIANGLE:
				if(PolygonPolygonCollision(dynamic_cast<Ppolygon*>(obj), &rec)) collision = true;
				break;
			}
		}
		if(collision) break;
	}
	return !collision;
}

bool FreePath(Circle* w1, Circle* w2, Map* map, bool hightSensitive = false, bool dontShow = true, Model* model = NULL) {
	bool collision = false;
	//std::cout << "Initializing tilewalker with positions " << w1->pos << " " << w2->pos << "\n";
	TileWalker walker = TileWalker(w1, w2, map, dontShow);
	//std::cout << "walker initialized!\n";
	std::vector<int> checkedObjects;
	gridpiece* tile = walker.current;
	//std::cout << "Entering loop...\n";
	if(model) {
		for(auto obj: map->mapObjects) {
			std::cout << obj->ID << " ";
		}
		std::cout << "\n";
	}
	while(tile != NULL) {
		//std::cout << "isolating crash...\n";
		if(model) {
			model->displayWalker = walker.walkedTiles;
			model->walkerStart = walker.start;
			model->walkerEnd = walker.end;
			model->walkerRec = walker.rec;
			model->hasWalker = true;
		}
		for(auto obj: tile->mapObjects) {
			if(!(obj->type == MAP_WAYPOINT) && std::find(checkedObjects.begin(), checkedObjects.end(), obj->ID) == checkedObjects.end()) {
				if(model)
					std::cout << "I swear we're checking some objects!\n";
				if(!hightSensitive || obj->high) {
					switch(obj->type) {
					case MAP_CIRCLE:
						if(model)
							std::cout << "It's a circle!\n";
						if(CircleRectangleCollision(dynamic_cast<Circle*>(obj), &walker.rec)) collision = true;
						break;
					case MAP_RECTANGLE:
					case MAP_TRIANGLE:
						if(model)
							std::cout << "It's a polygon!\n";
						//std::cout << "You can see this is a rectangle, right?\n";
						if(PolygonPolygonCollision(dynamic_cast<Ppolygon*>(obj), &walker.rec)) collision = true;
						break;
					}
				}
				if(model)
					std::cout << "It did " << collision << " collide at tile " << walker.current->nrow << " " << walker.current->ncol << "!\n";
				if(collision) break;
				checkedObjects.push_back(obj->ID);
			}
		}
		if(collision) break;
		//std::cout << "We all know...\n";
		tile = walker.Next();
		//std::cout << "...wherer the problem lies.\n";
	}
	//std::cout << "Finished loop!\n";
	return !collision;
}

gridpiece* NextTile(gridpiece* tile, gridpiece* start, gridpiece* end) {
	// geometric problem yet to be solved
	return tile;
};

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

std::vector<Eigen::Vector2d> findExplicitPath(int start, int end, Map* map) {
	std::vector<Eigen::Vector2d> positions;
	positions.push_back(map->waypoints.at(start)->pos);
	int next = map->wp_path_next.at(start).at(end);
	while(next != end) {
		positions.push_back(map->waypoints.at(next)->pos);
		next = map->wp_path_next.at(next).at(end);
	}
	positions.push_back(map->waypoints.at(end)->pos);
	return positions;
}

std::vector<Eigen::Vector2d> findPath(Circle* w1, Circle* w2, Map* map, Model* model = NULL) {
	//auto startTime = std::chrono::system_clock::now();
	//auto endTime = std::chrono::system_clock::now();
	// finding waypoints with line of sight to start and goal
	std::vector<int> visibleStart = std::vector<int>();
	std::vector<int> visibleEnd = std::vector<int>();
	//std::cout << "there are " << map->waypoints.size() << " wps to check.\n";
	for(int j = 0; j < map->waypoints.size(); j++) {
		//startTime = std::chrono::system_clock::now();
		if(FreePath(w2, map->waypoints.at(j), map)) {visibleStart.push_back(j);}
		if(FreePath(w1, map->waypoints.at(j), map)) {visibleEnd.push_back(j);}
		//endTime = std::chrono::system_clock::now();
		//if(time)
		//	*time += std::chrono::duration<double>(endTime - startTime).count();
		//std::cout << "checked wp " << j << "\n";
	}
	// finding shortest-total-path combination
	int start, end;
	bool foundPath = false;
	float mindist = std::numeric_limits<float>::infinity();
	for(auto i1 : visibleStart) {
		for(auto i2 : visibleEnd) {
			float dist = (w2->pos - map->waypoints.at(i1)->pos).norm()
						+ (w1->pos - map->waypoints.at(i2)->pos).norm()
						+ map->wp_path_dist.at(i1).at(i2);
			if(dist < mindist) {
				mindist = dist;
				start = i1;
				end = i2;
				foundPath = true;
			}
		}
	}
	//determining positions
	/*std::vector<Eigen::Vector2d> positions;
	if(mindist != std::numeric_limits<float>::infinity()) {
		positions.push_back(map->waypoints.at(start)->pos);
		int next = map->wp_path_next.at(start).at(end);
		while(next != end) {
			positions.push_back(map->waypoints.at(next)->pos);
			next = map->wp_path_next.at(next).at(end);
		}
		positions.push_back(map->waypoints.at(end)->pos);
		positions.push_back(w1->pos);	
	}*/
	std::vector<Eigen::Vector2d> positions;
	if(foundPath) {
		positions = findExplicitPath(start, end, map);
		positions.push_back(w1->pos);
	}
	return positions;
}

#endif