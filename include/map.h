#ifndef MAP
#define MAP

//#include <base.h>
#include <soldiers.h>
#include <units.h>
#include <extra_math.h>
#include <debug.h>
#include <projectiles.h>

#include <iostream>
#include <vector>
#include <algorithm>
#include <Dense>

enum MAP_OBJECT_TYPES {
	MAP_NONE,
	MAP_TRIANGLE,
	MAP_RECTANGLE,
	MAP_BORDER,
	MAP_DEPLOYMENT_ZONE,
	MAP_CIRCLE,
	MAP_WAYPOINT
};

class Map;

class MapObject {
public:
	int type;
	bool high = true;
	virtual void AutoWaypoints(double rad, Map* map) {}
	void toggle_high() {
		high = !high;
	};
	MapObject() {type = MAP_NONE;}
};

class MapCircle : public MapObject, public Circle {
public:
	MapCircle(Eigen::Vector2d pos, double rad) : MapObject(), Circle(pos, rad) {type = MAP_CIRCLE;}
	void AutoWaypoints(double rad, Map* map);
};

class MapWaypoint : public MapCircle {
public:
	bool _auto;
	MapWaypoint(Eigen::Vector2d pos, double rad) : MapCircle(pos, rad) {_auto = false; type = MAP_WAYPOINT;}
};

class MapTriangle : public MapObject, public Triangle {
public:
	MapTriangle(double a, double b, double gamma, Eigen::Vector2d pos, Eigen::Matrix2d rot) : MapObject(), Triangle(a, b, gamma, pos, rot) {type = MAP_TRIANGLE;}
	void AutoWaypoints(double rad, Map* map);
};

class MapRectangle : public MapObject, public Rrectangle {
public:
	MapRectangle(double hl, double hw, Eigen::Vector2d pos, Eigen::Matrix2d rot) : MapObject(), Rrectangle(hl, hw, pos, rot) {type = MAP_RECTANGLE;}
	void AutoWaypoints(double rad, Map* map);
};

class MapBorder : public MapRectangle {
public:
	MapBorder(double hl, double hw, Eigen::Vector2d pos, Eigen::Matrix2d rot) : MapRectangle(hl, hw, pos, rot) {type = MAP_BORDER;}
};

class DeploymentZone : public MapRectangle {
public:
	int player_id = 0;
	DeploymentZone(double hl, double hw, Eigen::Vector2d pos, Eigen::Matrix2d rot) : MapRectangle(hl, hw, pos, rot) {type = MAP_DEPLOYMENT_ZONE;}
	DeploymentZone(double hl, double hw, Eigen::Vector2d pos, Eigen::Matrix2d rot, int player_id) : DeploymentZone(hl, hw, pos, rot) {
		this->player_id = player_id;
	}
};

class gridpiece{
public:
	std::vector<Soldier*> soldiers;
	std::vector<Soldier*> p1Soldiers;
	std::vector<Soldier*> p2Soldiers;
	std::vector<MapObject*> mapObjects;
	std::vector<Projectile*> projectiles;
	std::vector<std::vector<gridpiece*>> neighbours;
	std::vector<std::vector<gridpiece*>> redundantNeighbours;
	Rrectangle* rec;
	int nrow;
	int ncol;
};

class grid_container{
public:
	int tilesize;
	int nrows;
	int ncols;
	std::vector<std::vector<gridpiece*>> grid;
};

class Map {
public:
	const static int optimalTileSize = 31;
	int width;
	int height;
	std::vector<grid_container*> grids;
	std::vector<MapObject*> mapObjects;
	std::vector<MapWaypoint*> waypoints;
	std::vector<MapBorder*> borders;
	std::vector<DeploymentZone*> deploymentZones;
	std::vector<std::vector<float>> wp_path_dist;
	std::vector<std::vector<int>> wp_path_next;
		
	void Cleangrid();
	void Assign(Soldier* soldier, int i, int j, grid_container* grid);
	void ProjectileAssign(Projectile* projectile, int i, int j, grid_container* grid);
	void AddMapObject(MapObject* obj, bool update = false);
	void UpdateGridsWithMapObjects();
	void RemoveMapObject(MapObject* obj);
	bool Borders();
	void toggelBorders();
	void initGrids(std::vector<int> tilesizes);
	std::string neighbourFileStr();
	bool searchNeighbourFile(std::string filename);
	void readNeighbourFile(std::string filename);
	void writeNeighbourFile(std::string filename);
	void setAllNeighbours();
	grid_container* getGrid(int tilesize);

private:
	void setNeighbours(grid_container* grid, int n_grid);
	void setRedundantNeighbours(grid_container* grid, int n_grid);
	void createBorders();
	void init();
		
public:
	Map(int width, int height, int tilesize) {
		this->width = width;
		this->height = height;
		init();
	}

	Map(int width, int height) : Map(width, height, optimalTileSize) {}
	Map(std::string filename);	//defined in fileio.h
};

void Map::init() {
	//creating map borders, but not adding them to objects yet
	createBorders();
}

void Map::initGrids(std::vector<int> tilesizes) {
	for(int size: tilesizes) {
		grid_container* container = new grid_container();
		container->tilesize = size;
		container->nrows = height / size + bool(height % size);
		container->ncols = width / size + bool(width % size);
		for(int i = 0; i < container->nrows; i++) {
			container->grid.push_back(std::vector<gridpiece*>());
			for(int j = 0; j < container->ncols; j++) {
				container->grid.at(i).push_back(new gridpiece());
				container->grid.at(i).at(j)->nrow = i;
				container->grid.at(i).at(j)->ncol = j;
				Eigen::Vector2d center;
				center << size/2. + j*size, size/2. + i*size;
				Eigen::Matrix2d rot;
				rot << 1., 0., 0., 1.;
				container->grid.at(i).at(j)->rec = new Rrectangle(size / 2., size / 2., center, rot);
				for(auto size: tilesizes) {
					container->grid.at(i).at(j)->neighbours.push_back(std::vector<gridpiece*>());
					container->grid.at(i).at(j)->redundantNeighbours.push_back(std::vector<gridpiece*>());
				}
			}
		}
		grids.push_back(container);
	}
}

grid_container* Map::getGrid(int tilesize) {
	for(auto container: grids) {
		if(tilesize == container->tilesize) {
			return container;
		}
	}
}

void Map::setNeighbours(grid_container* grid, int n_grid) {
	for(int i = 0; i < grid->nrows - 1; i++) {
		for(int j = 0; j < grid->ncols - 1; j++) {
			grid->grid.at(i).at(j)->neighbours[n_grid].push_back(grid->grid.at(i+1).at(j));
			grid->grid.at(i).at(j)->neighbours[n_grid].push_back(grid->grid.at(i).at(j+1));
			grid->grid.at(i).at(j)->neighbours[n_grid].push_back(grid->grid.at(i+1).at(j+1));
			if(i == 0) {
				grid->grid.at(grid->nrows-1).at(j)->neighbours[n_grid].push_back(grid->grid.at(grid->nrows-1).at(j+1));
			}
			if(j > 0) {
				grid->grid.at(i).at(j)->neighbours[n_grid].push_back(grid->grid.at(i+1).at(j-1));
			}
		}
		grid->grid.at(i).at(grid->ncols-1)->neighbours[n_grid].push_back(grid->grid.at(i+1).at(grid->ncols-1));
	}	
}

void Map::setRedundantNeighbours(grid_container* grid, int n_grid) {
	for(int i = 1; i < grid->nrows; i++) {
		for(int j = 1; j < grid->ncols; j++) {
			if(i+1 < grid->nrows && j-1 > 0)
				grid->grid.at(i).at(j)->redundantNeighbours[n_grid].push_back(grid->grid.at(i+1).at(j-1));
			//if(j-1 > 0)
			//	grid->grid.at(i).at(j)->redundantNeighbours2[n_grid].push_back(grid->grid.at(i).at(j-1));
			if(i-1 > 0 && j-1 > 0)
				grid->grid.at(i).at(j)->redundantNeighbours[n_grid].push_back(grid->grid.at(i-1).at(j-1));
			if(i-1 > 0)
				grid->grid.at(i).at(j)->redundantNeighbours[n_grid].push_back(grid->grid.at(i-1).at(j));
			if(i-1 > 0 && j+1 < grid->ncols)
				grid->grid.at(i).at(j)->redundantNeighbours[n_grid].push_back(grid->grid.at(i-1).at(j+1));
		}
	}
}

void Map::setAllNeighbours() {
	std::cout << "mapping grids to each other (this may take a while)\n";
	std::vector<grid_container*>::iterator it;
	for(it = grids.begin(); it < grids.end(); it++) {
		std::vector<grid_container*>::iterator it2;
		for(it2 = it; it2 < grids.end(); it2++) {
			std::cout << "grid sizes: " << (*it)->tilesize << " " << (*it2)->tilesize << "\n";
			if(it == it2) {
				setNeighbours(*it, it - grids.begin());
				setRedundantNeighbours(*it, it - grids.begin());
			}
			else {
				double hw = ((*it2)->tilesize + 2*(*it)->tilesize) * 0.5;
				double exclusion_dist = pow(2, 0.5) * hw + (*it2)->tilesize / pow(2, 0.5) * 1.1;
				int cntr = 0;
				for(auto row1: (*it)->grid) {
					cntr++;
					for(auto tile: row1) {
						Rrectangle collisionBox(hw, hw, tile->rec->pos, tile->rec->rot);
						// finding surrounding tile
						int i = (int) (tile->rec->pos.coeff(1) / (*it2)->tilesize);
						if(i < 0) i = 0;
						if(i > (*it2)->nrows - 1) i = (*it2)->nrows - 1;
						int j = (int) (tile->rec->pos.coeff(0) / (*it2)->tilesize);
						if(j < 0) j = 0;
						if(j > (*it2)->ncols - 1) j = (*it2)->ncols - 1;
						// reducing number tiles to check for collision
						int up, down, left, right;
						up = down = left = right = 0;
						while(true) {
							if(i - down <= 0) break;
							if(tile->rec->pos.coeff(1) - (*it2)->grid.at(i - down).at(j)->rec->pos.coeff(1) > exclusion_dist)
								break;
							down++;
						}
						while(true) {
							if(i + up >= (*it2)->nrows - 1) break;
							if((*it2)->grid.at(i + up).at(j)->rec->pos.coeff(1) - tile->rec->pos.coeff(1) > exclusion_dist)
								break;
							up++;
						}
						while(true) {
							if(j - left <= 0) break;
							if(tile->rec->pos.coeff(0) - (*it2)->grid.at(i).at(j - left)->rec->pos.coeff(0) > exclusion_dist)
								break;
							left++;
						}
						while(true) {
							if(j + right >= (*it2)->ncols - 1) break;
							if((*it2)->grid.at(i).at(j + right)->rec->pos.coeff(0) - tile->rec->pos.coeff(0) > exclusion_dist)
								break;
							right++;
						}
						// checking the remaining tiles for neighbours
						for(int m = i - down; m <= i + up; m++) {
							for(int n = j - left; n <= j + right; n++) {
								gridpiece* largerTile = (*it2)->grid.at(m).at(n);
								if((tile->rec->pos - largerTile->rec->pos).norm() <= exclusion_dist) {
									if(RectangleRectangleCollision(&collisionBox, largerTile->rec)) {
										tile->neighbours.at(it2 - grids.begin()).push_back(largerTile);
										largerTile->redundantNeighbours.at(it - grids.begin()).push_back(tile);
									}
								}
							}
						}
					}
				}
			}
		}
	}
}

void Map::createBorders() {
	double hw = 0.5*width;
	double hl = 0.5*height;
	double ht = 0.5*optimalTileSize;
	Eigen::Vector2d pos;
	Eigen::Matrix2d rot; rot << 1, 0, 0, 1;
	pos << hw, height + ht;
	borders.push_back(new MapBorder(ht, hw, pos, rot));
	pos << width + ht, hl;
	borders.push_back(new MapBorder(hl, ht, pos, rot));
	pos << hw, -ht;
	borders.push_back(new MapBorder(ht, hw, pos, rot));
	pos << -ht, hl;
	borders.push_back(new MapBorder(hl, ht, pos, rot));
}

void Map::Cleangrid() {
	for(auto grid: grids) {
		for(auto row: grid->grid) {
			for(auto tile: row) {
				tile->soldiers.clear();
				tile->p1Soldiers.clear();
				tile->p2Soldiers.clear();
				tile->projectiles.clear();
			}
		}
	}
}

void Map::Assign(Soldier* soldier, int i, int j, grid_container* grid) {
	grid->grid.at(i).at(j)->soldiers.push_back(soldier);
	soldier->tile_index = grid->grid.at(i).at(j)->soldiers.size() - 1;
	if(soldier->unit->player->player1)
		grid->grid.at(i).at(j)->p1Soldiers.push_back(soldier);
	else
		grid->grid.at(i).at(j)->p2Soldiers.push_back(soldier);
}

void Map::ProjectileAssign(Projectile* projectile, int i, int j, grid_container* grid) {
	grid->grid.at(i).at(j)->projectiles.push_back(projectile);
}

void Map::AddMapObject(MapObject* obj, bool update) {
	switch(obj->type) {
	case MAP_CIRCLE: {
		Circle* circ = dynamic_cast<Circle*>(obj);
		for(auto grid: grids) {
			Circle extended = Circle(circ->pos, circ->rad + 0.5*grid->tilesize);
			for(auto row: grid->grid) {
				for(auto tile: row) {
					if(CircleRectangleCollision(&extended, tile->rec)) {
						tile->mapObjects.push_back(obj);
					}
				}
			}
		}
		break;}
	case MAP_TRIANGLE: {
		Triangle* tri = dynamic_cast<Triangle*>(obj);
		for(auto grid: grids) {
			for(auto row: grid->grid) {
				for(auto tile: row) {
					Rrectangle extended = Rrectangle(tile->rec->hl*2, tile->rec->hw*2, tile->rec->pos, tile->rec->rot);
					if(PolygonPolygonCollision(tri, &extended)) {
						tile->mapObjects.push_back(obj);
					}
				}
			}
		}
		}break;
	case MAP_BORDER:
	case MAP_RECTANGLE: {
		Rrectangle* rec = dynamic_cast<Rrectangle*>(obj);
		for(auto grid: grids) {
			Rrectangle extended = Rrectangle(rec->hl + 0.5*grid->tilesize, rec->hw + 0.5*grid->tilesize, rec->pos, rec->rot);
			for(auto row: grid->grid) {
				for(auto tile: row) {
					if(RectangleRectangleCollision(&extended, tile->rec)) {
						tile->mapObjects.push_back(obj);
					}
				}
			}
		}
		}break;
	case MAP_DEPLOYMENT_ZONE:
		if(!update)
			deploymentZones.push_back(dynamic_cast<DeploymentZone*>(obj));
		break;
	case MAP_WAYPOINT:
		if(!update)
			waypoints.push_back(dynamic_cast<MapWaypoint*>(obj));
		break;
	}
	if(!update) mapObjects.push_back(obj);
}

void Map::UpdateGridsWithMapObjects() {
	std::cout << "mapping map objects to grid tiles (this may take a while)\n";
	for(auto obj: mapObjects) {
		AddMapObject(obj, true);
	}
}

void Map::RemoveMapObject(MapObject* obj) {
	std::erase(mapObjects, obj);
	switch(obj->type) {
	case MAP_CIRCLE:
	case MAP_TRIANGLE:
	case MAP_RECTANGLE:
		for(auto grid: grids) {
			for(auto row: grid->grid) {
				for(auto tile: row) {
					std::erase(tile->mapObjects, obj);
				}
			}
		}
		break;
	case MAP_DEPLOYMENT_ZONE:
		std::erase(deploymentZones, dynamic_cast<DeploymentZone*>(obj));
		break;
	case MAP_WAYPOINT:
		std::erase(waypoints, dynamic_cast<MapWaypoint*>(obj));
		break;
	}
}

bool Map::Borders() {
	for(auto obj : mapObjects) {
		if(obj->type == MAP_BORDER) return true;
	}
	return false;
}

void Map::toggelBorders() {
	if(Borders()) {
		std::vector<MapObject*> reference = mapObjects;
		for(auto obj : reference) {
			if(obj->type == MAP_BORDER) {
				std::erase(mapObjects, obj);
				for(auto grid: grids) {
					for(auto row: grid->grid) {
						for(auto tile: row) {
							std::erase(tile->mapObjects, obj);
						}
					}
				}
			}
		}
		std::cout << "Toggled off map borders.\n";
	}
	else {
		for(auto border : borders) {
			AddMapObject(border);
		}
		std::cout << "Toggled on map borders.\n";
	}
}


void SoldierCircleCollision(Soldier* soldier, Circle* circle) {
	double collisionStrength = 2.;
	Eigen::Vector2d posCorrection;
	Eigen::Vector2d knockVel;
	Eigen::Vector2d dist = soldier->pos - circle->pos;
	double d = dist.norm();
	if(d <= (soldier->rad + circle->rad)) {		
		double sin = dist.coeff(1) / d;
		double cos = dist.coeff(0) / d;
		Eigen::Matrix2d rot;
		rot << cos, -sin, sin, cos;
		Eigen::Vector2d rotVel = rot.transpose() * soldier->vel;
		if(rotVel.coeff(0) < 0) {
			rotVel(0) *= -collisionStrength;
			rotVel(1) = 0;
			rotVel = rot * rotVel;
			knockVel = rotVel;
			posCorrection = dist/d*(soldier->rad + circle->rad - d);
			soldier->knockVel += knockVel;
			soldier->pos += posCorrection;
		}
	}
}

enum COLLISION_TYPES {
	COLLISION_EDGE,
	COLLISION_CORNER
};

void SoldierPolygonCollision(Soldier* soldier, Ppolygon* pol) {
	// find closest edge
	int closestEdge = 0;
	int collision_type = COLLISION_CORNER;
	double minEdgeDist; minEdgeDist = std::numeric_limits<float>::infinity();
	Eigen::Vector2d edge;
	double len;
	Eigen::Matrix2d rot; Eigen::Matrix2d reversedRot;
	Eigen::Vector2d solPos;
	for(int i = 0; i < pol->corners.size(); i++) {
		edge = pol->corners.at((i + 1)%pol->corners.size())->pos
			- pol->corners.at(i)->pos;
		len = edge.norm();
		rot = Rotation(Angle(edge.coeff(1) / len, edge.coeff(0) / len));
		reversedRot = rot.transpose();
		solPos = reversedRot * (soldier->pos - pol->corners.at(i)->pos);
		if(0 <= solPos.coeff(0) && solPos.coeff(0) <= len && 0 <= solPos.coeff(1)) {
			minEdgeDist = abs(solPos.coeff(1));
			closestEdge = i;
			collision_type = COLLISION_EDGE;
		}
	}
	// find closest corner
	int closestCorner = 0;
	if(collision_type == COLLISION_CORNER) {
		double minCornerDist; minCornerDist = std::numeric_limits<float>::infinity();
		for(int i = 0; i < pol->corners.size(); i++) {
			Eigen::Vector2d diff = soldier->pos - pol->corners.at(i)->pos;
			double dist = diff.coeff(0)*diff.coeff(0) + diff.coeff(1)*diff.coeff(1);
			if(dist < minCornerDist) {
				minCornerDist = dist;
				closestCorner = i;
			}
		}
	}
	// compute speed and position changes
	Eigen::Vector2d rotVel, knockVel; 
	rotVel << 0., 0.; knockVel << 0., 0.;
	Eigen::Vector2d soldierPosCorrection; soldierPosCorrection << 0., 0.;
	switch(collision_type) {
	case COLLISION_EDGE: {
		edge = pol->corners.at((closestEdge + 1)%pol->corners.size())->pos
			- pol->corners.at(closestEdge)->pos;
		len = edge.norm();
		rot = Rotation(Angle(edge.coeff(1) / len, edge.coeff(0) / len));
		reversedRot = rot.transpose();
		solPos = reversedRot * (soldier->pos - pol->corners.at(closestEdge)->pos);
		rotVel = reversedRot * soldier->vel;
		if(rotVel.coeff(1) < 0 && solPos.coeff(1) < soldier->rad) {
			rotVel(0) = 0;
			rotVel(1) *= -2;
			rotVel = rot * rotVel;
			knockVel = rotVel;
			soldierPosCorrection(1) = soldier->rad - solPos.coeff(1);
			soldierPosCorrection = rot * soldierPosCorrection;
		}
	}break;
	case COLLISION_CORNER: {
		solPos = soldier->pos - pol->corners.at(closestCorner)->pos;
		len = solPos.norm();
		rot = Rotation(Angle(solPos.coeff(1) / len, solPos.coeff(0) / len));
		reversedRot = rot.transpose();
		rotVel = reversedRot * soldier->vel;
		if(rotVel.coeff(0) < 0 && len < soldier->rad) {
			rotVel(1) = 0;
			rotVel(0) *= -2;
			rotVel = rot * rotVel;
			soldierPosCorrection(0) = soldier->rad - len;
			soldierPosCorrection = rot * soldierPosCorrection;
		}
	}break;
	}
	// apply computed values
	soldier->knockVel += knockVel;
	soldier->pos += soldierPosCorrection;
}

void SoldierRectangleCollision(Soldier* soldier, Rrectangle* rec) {
	//deprecated
	SoldierPolygonCollision(soldier, rec);
}

void MapCircle::AutoWaypoints(double rad, Map* map) {
	double sin = rad / (2 * (rad + this->rad));
	double cos = std::sqrt(std::pow(rad + this->rad, 2) - std::pow(rad / 2, 2)) / (rad + this->rad);
	double ang = Angle(sin, cos);
	int nwaypoints = std::ceil(2*M_PI / ang);
	nwaypoints = ceil(nwaypoints * 0.75);
	double dphi = 2*M_PI / nwaypoints;
	for(int i = 0; i < nwaypoints; i=i+1) {
		sin = std::sin(i*dphi); cos = std::cos(i*dphi);
		Eigen::Matrix2d wp_rot; wp_rot << cos, -sin, sin, cos;
		Eigen::Vector2d wp_pos; wp_pos << rad + this->rad, 0; wp_pos = wp_rot*wp_pos + pos;
		MapWaypoint* w = new MapWaypoint(wp_pos, rad);
		w->_auto = true;
		map->AddMapObject(w);
	}
}

/*void PolygonAutoWaypoints(Polygon* pol, double rad, Map* map) {
	
}*/

void MapTriangle::AutoWaypoints(double rad, Map* map) {
	for(int i = 0; i < corners.size(); i++) {
		Eigen::Vector2d pos;
		Corner* c1 = corners.at(i);
		Corner* c2 = corners.at((i+1)%corners.size());
		Corner* c3 = corners.at((i+2)%corners.size());
		Eigen::Vector2d p1 = c1->pos - c2->pos;
		Eigen::Vector2d p3 = c3->pos - c2->pos;
		Eigen::Matrix2d rot1 = Rotation(Angle(-p1.coeff(1)/p1.norm(), -p1.coeff(0)/p1.norm()));
		p3 = rot1.transpose() * p3;
		double cornerAngle = Angle(-p3.coeff(1) / p3.norm(), -p3.coeff(0) / p3.norm());
		std::cout << "angle: " << cornerAngle * 180 / M_PI << "\n";
		MapWaypoint* w;
		if(cornerAngle == M_PI/2) {
			pos << rad, rad;
			w = new MapWaypoint(rot1 * pos + c2->pos, rad);
			w->_auto = true;
			map->AddMapObject(w);
			std::cout << w->pos << "\n";
		}
		else if(cornerAngle > M_PI/2) {
			pos << rad / p3.coeff(1) * (p3.coeff(0) - p3.norm() / std::sqrt(2)), rad;
			w = new MapWaypoint(rot1 * pos + c2->pos, rad);
			w->_auto = true;
			map->AddMapObject(w);
			std::cout << w->pos << "\n";
		}
		else {
			pos << rad, rad;
			w = new MapWaypoint(rot1 * pos + c2->pos, rad);
			w->_auto = true;
			map->AddMapObject(w);
			std::cout << w->pos << "\n";
			p3 = c3->pos - c2->pos;
			Eigen::Matrix2d rot3 = Rotation(Angle(-p3.coeff(1)/p3.norm(), -p3.coeff(0)/p3.norm()));
			pos << rad, -rad;
			w = new MapWaypoint(rot3 * pos + c2->pos, rad);
			w->_auto = true;
			map->AddMapObject(w);
			std::cout << w->pos << "\n";
		}
	}
	std::cout << "mapwaypoint size: " << map->waypoints.size() << "\n";
}

void MapRectangle::AutoWaypoints(double rad, Map* map) {
	std::vector<Eigen::Vector2d> centers;
	Eigen::Vector2d p1, p2, p3, p4;
	p1 << hw + rad, hl + rad; p1 = rot*p1 + pos; centers.push_back(p1);
	p2 << hw + rad, -hl - rad; p2 = rot*p2 + pos; centers.push_back(p2);
	p3 << -hw - rad, -hl - rad; p3 = rot*p3 + pos; centers.push_back(p3);
	p4 << -hw - rad, hl + rad; p4 = rot*p4 + pos; centers.push_back(p4);
	for(auto center : centers) {
		MapWaypoint* w = new MapWaypoint(center, rad);
		w->_auto = true;
		map->AddMapObject(w);
	}
}

void AutoWaypoints(double rad, Map* map) {
	//deleting old automatic waypoints
	std::vector<MapWaypoint*> wpreference = map->waypoints;
	for(auto wp : wpreference) {
		if(wp->_auto) {
			map->RemoveMapObject(wp);
		}
	}
	//creating new automatic waypoints
	std::vector<MapObject*> objreference = map->mapObjects;
	// this should access the virtual method instead of needing a switch
	for(auto obj : objreference) {
		switch(obj->type) {
		case MAP_CIRCLE:
			dynamic_cast<MapCircle*>(obj)->AutoWaypoints(rad, map);
			break;
		case MAP_TRIANGLE:
			dynamic_cast<MapTriangle*>(obj)->AutoWaypoints(rad, map);
			break;
		case MAP_RECTANGLE:
			dynamic_cast<MapRectangle*>(obj)->AutoWaypoints(rad, map);
		break;
		}
	}
	//deleting new automatic waypoints if they collide with objects
	wpreference = map->waypoints;
	for(auto wp : wpreference) {
		bool collision = false;
		for(auto obj : objreference) {
			switch(obj->type) {
			case MAP_CIRCLE:
				if(LenientCircleCircleCollision(dynamic_cast<Circle*>(obj), dynamic_cast<Circle*>(wp))) {
					collision = true;
				}
				break;
			case MAP_TRIANGLE:
				if(CirclePolygonCollision(dynamic_cast<Circle*>(wp), dynamic_cast<Triangle*>(obj))) {
					collision = true;
				}
				break;
			case MAP_RECTANGLE:
			case MAP_BORDER:
				if(CircleRectangleCollision(dynamic_cast<Circle*>(wp), dynamic_cast<Rrectangle*>(obj))) {
					collision = true;
				}
				break;
			}
			if(collision) {
				map->RemoveMapObject(wp);
				break;
			}
		}
	}
}

#endif