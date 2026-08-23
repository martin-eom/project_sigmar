#ifndef ORDER_RULES
#define ORDER_RULES

//#include <orders.h>
#include <pathfinding.h>
#include <model.h>

bool orderInDeploymentZone(Eigen::Vector2d pos, Model* model, Unit* unit) {
	Point orderPos(pos);
	for(auto dz : model->map->deploymentZones) {
		if(dz->player_id == 
			std::find(model->players.begin(), model->players.end(), unit->player)
			- model->players.begin())
			if(PointPolygonCollision(&orderPos, dz)) {
				return true;
			}
	}
	return false;
}

bool orderInMapBoundaries(Eigen::Vector2d pos, Map* map) {
	return 0 < pos.x() && pos.x() < map->width && 0 < pos.y() && pos.y() < map->height;
}

bool orderCollidesWithMapObjects(Eigen::Vector2d pos, Eigen::Matrix2d rot, Map* map, Unit* unit) {
	double halfWidth, halfDepth;
	halfWidth = (unit->ncols - 1) * unit->yspacing * 0.5 + 1.*unit->soldiers.at(0).at(0)->rad;
	halfDepth = (unit->nrows - 1) * unit->xspacing * 0.5 + 1.*unit->soldiers.at(0).at(0)->rad;

	Rrectangle orderRectangle(halfWidth, halfDepth, pos, rot);
	for(auto object : map->mapObjects) {
		switch(object->type) {
		case MAP_RECTANGLE:
		case MAP_TRIANGLE:
		case MAP_BORDER:
			if(PolygonPolygonCollision(&orderRectangle, dynamic_cast<Ppolygon*>(object))) return true;
			break;
		case MAP_CIRCLE:
			if(CirclePolygonCollision(dynamic_cast<Circle*>(object), &orderRectangle)) return true;
			break;
		}
	}
	return false;
}

Unit* canShootSomethingFromOrder(Eigen::Vector2d pos, Unit* unit, Player* player, Map* map) {
	for(auto target : player->units) {
		double unitRad = std::max((unit->ncols - 1)*unit->xspacing*0.5 + unit->soldiers.at(0).at(0)->rad, (unit->nrows - 1)*unit->xspacing*0.5 + unit->soldiers.at(0).at(0)->rad);
		Circle cUnit(unit->pos, unitRad);
		Circle cTarget(target->pos, unitRad);
		if((target->pos - unit->pos).norm() < unit->range && FreePath(&cUnit, &cTarget, map, true) && target->nLiveSoldiers > 0) {
			return target;
		}
	}
	return NULL;
}

#endif