#ifndef MAP
#define MAP

//#include <base.h>
#include <soldiers.h>
#include <units.h>
#include <extra_math.h>
#include <debug.h>

#include <iostream>
#include <vector>
#include <algorithm>
#include <Dense>

enum MAP_OBJECT_TYPES {
	MAP_NONE,
	MAP_RECTANGLE,
	MAP_BORDER,
	MAP_CIRCLE,
	MAP_WAYPOINT
};

class Map;

class MapObject {
public:
	virtual int type() {return MAP_NONE;}
	virtual void AutoWaypoints(double rad, Map* map) {}
	MapObject() {}
};

class MapCircle : public MapObject, public Circle {
public:
	int type() {return MAP_CIRCLE;}
	MapCircle(Eigen::Vector2d pos, double rad) : MapObject(), Circle(pos, rad) {}
	void AutoWaypoints(double rad, Map* map);
};

class MapWaypoint : public MapObject, public Circle {
public:
	int type() {return MAP_WAYPOINT;}
	bool _auto;
	MapWaypoint(Eigen::Vector2d pos, double rad) : MapObject(), Circle(pos, rad) {_auto = false;}
};

class MapRectangle : public MapObject, public Rrectangle {
public:
	int type() {return MAP_RECTANGLE;}
	MapRectangle(double hl, double hw, Eigen::Vector2d pos, Eigen::Matrix2d rot) : MapObject(), Rrectangle(hl, hw, pos, rot) {}
	void AutoWaypoints(double rad, Map* map);
};

class MapBorder : public MapObject, public Rrectangle {
public:
	int type() {return MAP_BORDER;}
	MapBorder(double hl, double hw, Eigen::Vector2d pos, Eigen::Matrix2d rot) : MapObject(), Rrectangle(hl, hw, pos, rot) {}
};

class gridpiece{
	public:
		std::vector<Soldier*> soldiers;
		std::vector<Soldier*> p1Soldiers;
		std::vector<Soldier*> p2Soldiers;
		std::vector<MapObject*> mapObjects;
		std::vector<gridpiece*> neighbours;
		Rrectangle* rec;
};

class Map {
	public:
		const static int optimalTileSize = 31;
		int tilesize;
		int width;
		int height;
		int nrows;
		int ncols;
		std::vector<std::vector<gridpiece*>> tiles;
		std::vector<MapObject*> mapObjects;
		std::vector<MapWaypoint*> waypoints;
		std::vector<MapBorder*> borders;
		std::vector<std::vector<float>> wp_path_dist;
		std::vector<std::vector<int>> wp_path_next;
		
	private:
		void init() {
			nrows = height / tilesize + bool(height % tilesize);
			ncols = width / tilesize + bool(width % tilesize);
			tiles = std::vector<std::vector<gridpiece*>>(nrows, std::vector<gridpiece*>(ncols, NULL));
			for(int i = 0; i < nrows; i++) {
				for(int j = 0; j < ncols; j++) {
					tiles.at(i).at(j) = new gridpiece();
					Eigen::Vector2d center;
					center << tilesize/2. + j*tilesize, tilesize/2. + i*tilesize;
					Eigen::Matrix2d rot;
					rot << 1., 0., 0., 1.;
					tiles.at(i).at(j)->rec = new Rrectangle(tilesize / 2., tilesize / 2., center, rot);
				}
			}
			for(int i = 0; i < nrows - 1; i++) {
				for(int j = 0; j < ncols - 1; j++) {
					tiles.at(i).at(j)->neighbours.push_back(tiles.at(i+1).at(j));
					tiles.at(i).at(j)->neighbours.push_back(tiles.at(i).at(j+1));
					tiles.at(i).at(j)->neighbours.push_back(tiles.at(i+1).at(j+1));
					if(i == 0) {
						tiles.at(nrows-1).at(j)->neighbours.push_back(tiles.at(nrows-1).at(j+1));
					}
				}
				tiles.at(i).at(ncols-1)->neighbours.push_back(tiles.at(i+1).at(ncols-1));
			}
			//creating map borders, but not adding them to objects yet
			double hw = 0.5*width;
			double hl = 0.5*height;
			double ht = 0.5*tilesize;
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
		
	public:
		Map(int width, int height, int tilesize) {
			this->width = width;
			this->height = height;
			this->tilesize = tilesize;
			init();
		}

		Map(int width, int height) : Map(width, height, optimalTileSize) {}

		Map(std::string filename);	//defined in fileio.h
		
		void Cleangrid() {
			for(int i = 0; i < nrows; i++) {
				for(int j = 0; j < ncols; j++) {
					tiles.at(i).at(j)->soldiers.clear();
					tiles.at(i).at(j)->p1Soldiers.clear();
					tiles.at(i).at(j)->p2Soldiers.clear();
				}
			}
		}
		
		void Assign(Soldier* soldier, int i, int j) {
			tiles.at(i).at(j)->soldiers.push_back(soldier);
			if(soldier->unit->player->player1)
				tiles.at(i).at(j)->p1Soldiers.push_back(soldier);
			else
				tiles.at(i).at(j)->p2Soldiers.push_back(soldier);
		}

		void AddMapObject(MapObject* obj) {
			switch(obj->type()) {
			case MAP_CIRCLE: {
				Circle* circ = dynamic_cast<Circle*>(obj);
				Circle extended = Circle(circ->pos, circ->rad + 0.5*tilesize);
				for(auto row : tiles) {
					for(auto tile : row) {
						if(CircleRectangleCollision(&extended, tile->rec)) {
							tile->mapObjects.push_back(obj);
						}
					}
				}
				break;}
			case MAP_BORDER:
			case MAP_RECTANGLE: {
				Rrectangle* rec = dynamic_cast<Rrectangle*>(obj);
				Rrectangle extended = Rrectangle(rec->hl + 0.5*tilesize, rec->hw + 0.5*tilesize, rec->pos, rec->rot);
				for(auto row : tiles) {
					for(auto tile : row) {
						if(RectangleRectangleCollision(&extended, tile->rec)) {
							tile->mapObjects.push_back(obj);
						}
					}
				}
				}break;
			case MAP_WAYPOINT:
				waypoints.push_back(dynamic_cast<MapWaypoint*>(obj));
				break;
			}
			mapObjects.push_back(obj);
		}

		void RemoveMapObject(MapObject* obj) {
			std::erase(mapObjects, obj);
			switch(obj->type()) {
			case MAP_CIRCLE:
			case MAP_RECTANGLE:
				for(auto row : tiles) {
					for(auto tile : row) {
						std::erase(tile->mapObjects, obj);
					}
				}
				break;
			case MAP_WAYPOINT:
				std::erase(waypoints, dynamic_cast<MapWaypoint*>(obj));
				break;
			}
		}

		bool Borders() {
			for(auto obj : mapObjects) {
				if(obj->type() == MAP_BORDER) return true;
			}
			return false;
		}

		void toggelBorders() {
			if(Borders()) {
				std::vector<MapObject*> reference = mapObjects;
				for(auto obj : reference) {
					if(obj->type() == MAP_BORDER) {
						std::erase(mapObjects, obj);
						for(auto row : tiles) {
							for(auto tile : row) {
								std::erase(tile->mapObjects, obj);
							}
						}
					}
				}
				std::cout << "Toggled on map borders.\n";
			}
			else {
				for(auto border : borders) {
					AddMapObject(border);
				}
				std::cout << "Toggled off map borders.\n";
			}
		}
};

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

void SoldierRectangleCollision(Soldier* soldier, Rrectangle* rec) {
	Eigen::Vector2d knockVel;
	knockVel << 0., 0.;
	Eigen::Vector2d normPos = soldier->pos - rec->pos;
	Eigen::Vector2d rotPos = rec->rot.transpose() * (soldier->pos - rec->pos);
	Eigen::Vector2d soldierPosCorrection;
	soldierPosCorrection << 0., 0.;
	bool _long, _wide;
	_long = (-rec->hw <= rotPos.coeff(0) && rotPos.coeff(0) <= rec->hw && -rec->hl - soldier->rad <= rotPos.coeff(1) && rotPos.coeff(1) <= rec->hl + soldier->rad);
	_wide = (-rec->hw - soldier->rad <= rotPos.coeff(0) && rotPos.coeff(0) <= rec->hw + soldier->rad && -rec->hl <= rotPos.coeff(1) && rotPos.coeff(1) <= rec->hl);
	double dl;
	double dw;
	if(rotPos.coeff(1) < 0) {dl = -(rec->hl + soldier->rad) - rotPos.coeff(1);}
	else {dl = rec->hl + soldier->rad - rotPos.coeff(1);}
	if(rotPos.coeff(0) < 0) {dw = -(rec->hw + soldier->rad) - rotPos.coeff(0);}
	else {dw = rec->hw + soldier->rad - rotPos.coeff(0);}
	int edge;
	if(_long) {
		if(_wide) {
			if(dl < 0) {
				if(dw < 0) {
					if(dw < dl) {edge = 3;}
					else {edge = 4;}
				}
				else {
					if(-dl < dw) {edge = 3;}
					else {edge = 2;}
				}
			}
			else {
				if(dw < 0) {
					if(dl < -dw) {edge = 1;}
					else {edge = 4;}
				}
				else {
					if(dl < dw) {edge = 1;}
					else {edge = 2;}
				}
			}		
		}
		else {
			if(dl < 0) {edge = 3;}
			else {edge = 1;}
		}
	}
	else {
		if(_wide) {
			if(dw < 0) {edge = 4;}
			else {edge = 2;}
		}
		else {
			edge = 0;
		}
	}
	Eigen::Vector2d rotVel = rec->rot.transpose() * soldier->vel;
	double collisionStrength = 2.;	//2. is physical
	switch(edge) {
	case 1:
		if(rotVel.coeff(1) < 0) {
			rotVel(0) = 0;
			rotVel(1) *= -collisionStrength;
			rotVel = rec->rot * rotVel;
			knockVel = rotVel;
			soldierPosCorrection(1) += dl;
			soldierPosCorrection = rec->rot * soldierPosCorrection;
		}
		break;
	case 2:
		if(rotVel.coeff(0) < 0) {
			rotVel(0) *= -collisionStrength;
			rotVel(1) = 0;
			rotVel = rec->rot * rotVel;
			knockVel = rotVel;
			soldierPosCorrection(0) += dw;
			soldierPosCorrection = rec->rot * soldierPosCorrection;
		}
		break;
	case 3:
		if(rotVel.coeff(1) > 0) {
			rotVel(0) = 0;
			rotVel(1) *= -collisionStrength;
			rotVel = rec->rot * rotVel;
			knockVel = rotVel;
			soldierPosCorrection(1) += dl;
			soldierPosCorrection = rec->rot * soldierPosCorrection;
		}
		break;
	case 4:
		if(rotVel.coeff(0) > 0) {
			rotVel(0) *= -collisionStrength;
			rotVel(1) = 0;
			rotVel = rec->rot * rotVel;
			knockVel = rotVel;
			soldierPosCorrection(0) += dw;
			soldierPosCorrection = rec->rot * soldierPosCorrection;
		}
		break;
	case 0:
		for(int i = 0; i < 4; i++) {
			Corner* corner = rec->corners.at(i);
			Eigen::Vector2d dist = soldier->pos - corner->pos;
			double d = dist.norm();
			if(d <= soldier->rad) {
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
					soldierPosCorrection(0) += soldier->rad - d;
					soldierPosCorrection = rot * soldierPosCorrection;
				}
			}
		}
	}
	//knockVel << 0., 0.;
	soldier->knockVel += knockVel;
	soldier->pos += soldierPosCorrection;
}

void MapCircle::AutoWaypoints(double rad, Map* map) {
	double sin = rad / (2 * (rad + this->rad));
	double cos = std::sqrt(std::pow(rad + this->rad, 2) - std::pow(rad / 2, 2)) / (rad + this->rad);
	debug(std::to_string(sin));
	debug(std::to_string(cos));
	double ang = Angle(sin, cos);
	int nwaypoints = std::ceil(2*M_PI / ang);
	nwaypoints = ceil(nwaypoints * 0.75);
	debug(std::to_string(nwaypoints));
	debug(std::to_string(M_PI));
	debug(std::to_string(ang));
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
	for(auto obj : objreference) {
		switch(obj->type()) {
		case MAP_CIRCLE:
			dynamic_cast<MapCircle*>(obj)->AutoWaypoints(rad, map);
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
			switch(obj->type()) {
			case MAP_CIRCLE:
				if(LenientCircleCircleCollision(dynamic_cast<Circle*>(obj), dynamic_cast<Circle*>(wp)))
					collision = true;
				break;
			case MAP_RECTANGLE:
			case MAP_BORDER:
				if(CircleRectangleCollision(dynamic_cast<Circle*>(wp), dynamic_cast<Rrectangle*>(obj)))
					collision = true;
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