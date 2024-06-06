#ifndef PHYSICS
#define PHYSICS

#include <extra_math.h>
#include <base.h>
#include <soldiers2.h>
#include <units.h>
#include <orders.h>
#include <map.h>
#include <player.h>

#ifndef _USE_MATH_DEFINES
#define _USE_MATH_DEFINES
#endif

#include <Dense>
#include <cmath>
#include <deque>


void ReformUnit(Unit* unit) {
	struct RebasedSoldier {
		Soldier* soldier;
		Eigen::Vector2d rebasedPos;

		RebasedSoldier(Soldier* soldier, Eigen::Matrix2d reverseRot, Eigen::Vector2d posInUnit) {
			this->soldier = soldier;
			rebasedPos = reverseRot * posInUnit;
		}
	};

	std::vector<std::vector<Soldier*>>* soldiers = &(unit->soldiers);
	std::deque<RebasedSoldier*> temp1;
	std::deque<RebasedSoldier*> temp2;
	// Sorting soldiers by new "y coordinate" (front-back in formation)
	for(int i = 0; i < unit->nrows; i++) {
		for(int j = 0; j < unit->ncols; j++) {
			Soldier* soldier = soldiers->at(i).at(j);
			if(soldier) {
				Eigen::Matrix2d rot;
				Order* o;
				if(soldier->alive)
					o = unit->orders.at(soldier->currentOrder);
				else
					o = unit->orders.at(unit->currentOrder);
				if(o->type == ORDER_MOVE) {
					MoveOrder* mo = dynamic_cast<MoveOrder*>(o);
					rot = mo->rot;
				}
				else {rot = o->rot;}
				Eigen::Matrix2d rotDiff = rot.transpose() * unit->rot;
				RebasedSoldier* r = new RebasedSoldier(soldier, rotDiff, rot.transpose() * (soldier->pos - unit->pos));
				auto current = temp1.begin();
				while(current != temp1.end()) {
					if(!(*current)->soldier->alive) break;
					if (r->soldier->currentOrder < (*current)->soldier->currentOrder || !r->soldier->alive) {
						current = std::next(current);
					}
					else if (r->soldier->currentOrder == (*current)->soldier->currentOrder && 
						r->rebasedPos[0] < (*current)->rebasedPos[0]) {
						current = std::next(current);
					}
					else break;
				}
				temp1.insert(current, r);
			}
		}
	}
	// Grabbing rows of soldiers and sorting them by currentOrder and new "x coordinate" (left-right in formation)
	while(!temp1.empty()) {
		std::deque<RebasedSoldier*> tempRow;
		// Filling tempRow
		while((!temp1.empty()) && tempRow.size() < unit->ncols) {
			RebasedSoldier* node = temp1.at(0); temp1.pop_front();
			auto current = tempRow.begin();
			int n = 0;
			while(current != tempRow.end()) {
				if(!(*current)->soldier->alive) break;
				if(node->rebasedPos[1] < (*current)->rebasedPos[1] || !node->soldier->alive) {
					n++;
					current = std::next(current);
				}
				else break;
			}
			tempRow.insert(current, node);
		}
		// Emptying tempRow
		while(!tempRow.empty()) {
			RebasedSoldier* r = tempRow.at(0); tempRow.pop_front();
			temp2.push_back(r);
		}
	}
	// Pasting the new order into unit
	for(int i = 0; i < unit->nrows; i++) {
		for(int j = 0; j < unit->ncols; j++) {
			if(!temp2.empty()) {
				RebasedSoldier* r = temp2.at(0); temp2.pop_front();
				(*soldiers).at(i).at(j) = r->soldier;
				delete r;
			}
		}
	}
	PosInUnitByID(unit);
	debug("Reforming unit succeeded");
}


void DampenedHarmonicOscillator(Soldier* soldier, double dt) {	//deprecated / only for testing purposes
	double k = 1;
	double damp = 2;
	Eigen::Vector2d newPos;
	Eigen::Vector2d newVel;
	Eigen::Vector2d newForce;
	newPos = soldier->pos + soldier->vel * dt;
	newVel = soldier->vel + soldier->force * dt;
	newForce = (soldier->posTarget - newPos)*k - newVel*damp;
	soldier->pos = newPos;
	soldier->vel = newVel;
	soldier->force = newForce;
}

Eigen::Vector2d NoIPFPosTarget(Soldier* soldier) {	// NoIPF = No Individual Pathfinding
	if(soldier->meleeTarget) {
		if(!soldier->charging)
			return soldier->meleeTarget->pos;
		if(soldier->unit->enemyContact && soldier->unit->orders.at(soldier->currentOrder)->target)
			return soldier->meleeTarget->pos;
	}
	return soldier->posTarget;
}

Eigen::Vector2d PosTarget(Soldier* soldier) {
	if(soldier->indivPath.empty()) {
		return NoIPFPosTarget(soldier);
		/*if(soldier->meleeTarget) {
			if(!soldier->charging)
				return soldier->meleeTarget->pos;
			if(soldier->unit->enemyContact && soldier->unit->orders.at(soldier->currentOrder)->target)
				return soldier->meleeTarget->pos;
		}
		return soldier->posTarget;	*/
	}
	else {
		return soldier->indivPath.at(0);
	}
}

void TimeStep(Soldier* soldier, double dt) {
	Eigen::Vector2d posTarget = PosTarget(soldier);;
	Eigen::Vector2d newPos, newVel;
	Eigen::Matrix2d newRot;
	double newAngle;
	//new position
	newPos = soldier->pos + soldier->vel * dt;
	//new velocity
	Eigen::Vector2d fullVel = soldier->vel + soldier->knockVel;	//knockVel will have to be atomic eventually
	newVel = fullVel + soldier->force * dt;
	double speed = soldier->vel.norm();
	soldier->speed = speed;
	//new angle and rotation
	Eigen::Matrix2d rotT;
	Eigen::Matrix2d rotDiff;
	double angleDiff;
	Eigen::Matrix2d toRot;
	double maxTurn;
	Eigen::Vector2d dist = posTarget - soldier->pos;
	double dx = dist.norm();
	bool closeToTarget = dx < soldier->maxSpeed/4;
	bool almostOnTarget = dx < 50 
		&& soldier->unit->orders.at(soldier->currentOrder)->type == ORDER_MOVE 
		&& dynamic_cast<MoveOrder*>(soldier->unit->orders.at(soldier->currentOrder))->moveType == MOVE_FORMUP;
	bool inMelee = soldier->meleeTarget && soldier->meleeTarget->alive;
	bool movingAway = dist.dot(soldier->vel) < 0.;
	if(closeToTarget) {
		if(inMelee) {
			Eigen::Vector2d dm = soldier->meleeTarget->pos - soldier->pos;
			double mdist = dm.norm();
			rotT << dm.coeff(0)/mdist, -dm.coeff(1)/mdist, dm.coeff(1)/mdist, dm.coeff(0)/mdist;
		}
		else if(soldier->rangedTarget) { // not correct, what are the correct conditions?
			Eigen::Vector2d dm = soldier->rangedTarget->pos - soldier->pos;
			double mdist = dm.norm();
			rotT << dm.coeff(0)/mdist, -dm.coeff(1)/mdist, dm.coeff(1)/mdist, dm.coeff(0)/mdist;			
		}
		else
			rotT = soldier->rotTarget;
	}
	else if(almostOnTarget && inMelee) {
			Eigen::Vector2d dm = soldier->meleeTarget->pos - soldier->pos;
			double mdist = dm.norm();
			rotT << dm.coeff(0)/mdist, -dm.coeff(1)/mdist, dm.coeff(1)/mdist, dm.coeff(0)/mdist;
	}
	else {
		double sinT, cosT;
		sinT = dist.coeff(1) / dx; 
		cosT = dist.coeff(0) / dx;
		rotT << cosT, -sinT, sinT, cosT;
	}
	rotDiff = rotT * (soldier->rot.transpose());
	angleDiff = Angle(rotDiff.coeff(0,1), rotDiff.coeff(0,0));
	if(!(soldier->rot == rotT)) {
		maxTurn = soldier->turn * dt * (0.1 + 0.9 * std::max((soldier->maxSpeed - speed), 0.)/soldier->maxSpeed);
		if(abs(angleDiff) < maxTurn) {
			newRot = rotT;
			if(closeToTarget) {
				newAngle = soldier->angleTarget;
			}
			else {
				newAngle = Angle(rotT.coeff(0,1), rotT.coeff(0,0));
			}
		}
		else {
			double sin, cos;
			sin = std::sin(maxTurn); cos = std::cos(maxTurn);
			if(angleDiff < 0) {
				toRot << cos, -sin, sin, cos;
			}
			else {
				toRot << cos, sin, -sin, cos;
			}
			newRot = soldier->rot * toRot;
			newAngle = Angle(newRot.coeff(0,1), newRot.coeff(0,0));
		}
		soldier->rot = newRot;
		soldier->angle = newAngle;
	}
	soldier->angle = Angle(soldier->rot.coeff(0,1), soldier->rot.coeff(0,0));
	soldier->forwardSpeed = newVel.coeff(0) * soldier->rot.coeff(0,0) + newVel.coeff(1) * soldier->rot.coeff(1, 0);
	//new force
	Eigen::Vector2d newForce;
	if(closeToTarget || abs(angleDiff) < 0.25 || (inMelee && almostOnTarget)) {
		newForce = (posTarget - newPos)*1;
		double oscilStrength = newForce.norm();
		if(oscilStrength > soldier->Force || movingAway) {	// moving away and not in the process of knockdown
			newForce *= soldier->Force / oscilStrength;
		}
	}
	else {
		newForce << 0, 0;
	}
	double damp;
	if(closeToTarget) {
		damp = soldier->onTargetDamp;
	}
	else if(inMelee && almostOnTarget && abs(angleDiff) >= 0.25) {
		damp = soldier->onTargetDamp;
	}
	else if(movingAway) {
		damp = soldier->onTargetDamp*1.1;
	}
	else {
		damp = soldier->squareDamp;
		double linDamp = soldier->linearDamp;
		double squDamp = soldier->squareDamp * soldier->vel.norm();
		damp = std::max(linDamp, squDamp);
	}
	Eigen::Vector2d dampForce = soldier->vel*damp;
	newForce -= dampForce; //dampening not finalized
	// extra dampening perpendicular to orientation (experimental, when turned on there is less deviation from course, 
	//												 but collisions with big units are less satisfying)
	if(false) {
		Eigen::Vector2d perp; perp << soldier->rot.coeff(1,0), -soldier->rot.coeff(0,0);
		double perp_fraction = dampForce.dot(perp);
		if(perp_fraction > 0) {
			newForce -= perp_fraction * perp;
		}
		else {
			newForce -= perp_fraction * perp;
		}
	}
	//paste new values to soldier
	soldier->pos = newPos;
	soldier->vel = newVel;
	soldier->force = newForce;
	soldier->knockVel << 0., 0.;
	//check if order complete
	bool newestOrder = (soldier->currentOrder == soldier->unit->currentOrder);
	Order* o = soldier->unit->orders.at(soldier->currentOrder);
	if(o->type == ORDER_MOVE) {
		MoveOrder* mo = dynamic_cast<MoveOrder*>(o);
		if(!soldier->arrived && soldier->indivPath.empty()) {
			if(newestOrder) {// && (mo->moveType != MOVE_PASSINGTHROUGH)) {
				if(closeToTarget) {
					soldier->arrived = true;
					soldier->unit->nSoldiersArrived++;
				}
			}
			else {
				Rrectangle rec = SoldierRectangle(soldier);
				Point p(soldier->pos);
				if(PointRectangleCollision(&p, &rec)) {
					soldier->arrived = true;
				}
			}
		}
	}
	if(o->type == ORDER_TARGET) {
		TargetOrder* mo = dynamic_cast<TargetOrder*>(o);
		if(!soldier->arrived && soldier->indivPath.empty()) {
			if(newestOrder) {// && (mo->moveType != MOVE_PASSINGTHROUGH)) {
				if(closeToTarget) {
					soldier->arrived = true;
					soldier->unit->nSoldiersArrived++;
				}
			}
			else {
				Rrectangle rec = SoldierRectangle(soldier);
				Point p(soldier->pos);
				if(PointRectangleCollision(&p, &rec)) {
					soldier->arrived = true;
				}
			}
		}
	}
	else if(o->type == ORDER_ATTACK && !soldier->arrived && soldier->indivPath.empty()) {
		if(dynamic_cast<AttackOrder*>(o)->target->nLiveSoldiers <= 0) {
			soldier->arrived = true;
		}
	}
}

void KnockKnock(Soldier* sold1, Soldier* sold2) {
	double minDist = sold1->rad + sold2->rad;
	if((sold1->unit->player == sold2->unit->player) && (sold1->unit != sold2->unit)) {
		minDist *= 0.7;
	}
	Eigen::Vector2d dx = sold2->pos - sold1->pos;
	double d = dx.norm();
	if(d < minDist) {
		Eigen::Vector2d dv = sold2->vel - sold1->vel;
		double dxdv = dx.dot(dv);
		bool movingCloser = dxdv < 0.;
		if(movingCloser) {
			double totalMass = sold1->mass + sold2->mass;
			Eigen::Vector2d prod = 2. * dxdv * dx / (totalMass * pow(d,2));
			sold1->knockVel += sold2->mass * prod;
			sold2->knockVel -= sold1->mass * prod;
			Eigen::Vector2d target1 = PosTarget(sold1);
			Eigen::Vector2d target2 = PosTarget(sold2);
			if(sold1->unit->player == sold2->unit->player && sold1->unit == sold2->unit) {
				if(sold1->currentOrder < sold2->currentOrder) {
					if((sold1->pos - target2).norm() < (sold2->pos - target2).norm()) {
						sold1->arrived = true;
					}
				}
				else if(sold1->currentOrder > sold2->currentOrder) {
					if((sold1->pos - target1).norm() > (sold2->pos - target1).norm()) {
						sold2->arrived = true;
					}
				}
			}
		}
	}
}

void CollisionScrying(Map* map, Unit* unit) {
	for(int i = 0; i < unit->nrows; i++) {
		for(int j = 0; j < unit->ncols; j++) {
			Soldier* soldier = (*unit).soldiers.at(i).at(j);
			if(soldier->placed && soldier->alive) {
				int m = (int) (soldier->pos.coeff(1) / map->tilesize);
				if(m < 0) m=0;
				else if(m > map->nrows - 1) m = map->nrows - 1;
				int n = (int) (soldier->pos.coeff(0) / map->tilesize);
				if(n < 0) n = 0;
				else if(n > map->ncols - 1) n = map->ncols - 1;
				map->Assign(soldier, m, n);
			}
		}
	}
}

void ProjectileCollisionScrying(Map* map, std::vector<Projectile*> projectiles) {
	for(auto projectile : projectiles) {
		int m = (int) (projectile->get_pos().coeff(1) / map->tilesize);
		if(m < 0) m=0;
		else if(m > map->nrows - 1) m = map->nrows - 1;
		int n = (int) (projectile->get_pos().coeff(0) / map->tilesize);
		if(n < 0) n = 0;
		else if(n > map->ncols - 1) n = map->ncols - 1;
		map->ProjectileAssign(projectile, m, n);
	}
}

void ProjectileCollisionHandling(Map* map) {
	for(int i = 0; i < map->nrows; i++) {
		for(int j = 0; j < map->ncols; j++) {
			gridpiece* tile = map->tiles.at(i).at(j);
			for(auto projectile : tile->projectiles) {
				Point p(projectile->get_pos());
				//map_objects
				for(auto obj : tile->mapObjects) {
					if(!projectile->dead) {
						switch(obj->type()) {
						case MAP_RECTANGLE:
						case MAP_BORDER:
							if(PointRectangleCollision(&p, dynamic_cast<Rrectangle*>(obj))) {
								projectile->dead = true;
							}
							break;
						case MAP_CIRCLE:
							Circle* circ = dynamic_cast<Circle*>(obj);
							Eigen::Vector2d dist;
							if((projectile->get_pos() - circ->pos).norm() < circ->rad) {
								projectile->dead = true;
							}
							break;
						}
					}
				}
				//soldiers
				if(!projectile->dead && projectile->get_progress() > 0.99) {
					for(auto soldier : tile->soldiers) {
						if((projectile->get_pos() - soldier->pos).norm() < (soldier->rad + projectile->aoerad)) {
							projectile->targets.push_back(soldier);
						}
					}
					if(projectile->aoerad > 0.) {
						for(auto ntile : tile->neighbours) {
							for(auto soldier : ntile->soldiers) {
								if((projectile->get_pos() - soldier->pos).norm() < soldier->rad + projectile->aoerad) {
									projectile->targets.push_back(soldier);
								}
							}						
						}
						for(auto ntile : tile->redundantNeighbours) {
							for(auto soldier : ntile->soldiers) {
								if((projectile->get_pos() - soldier->pos).norm() < soldier->rad + projectile->aoerad) {
									projectile->targets.push_back(soldier);
								}
							}						
						}
					}
					projectile->dead = true;
				}
			}
		}
	}
}

void EvaluateRange(Soldier* sold1, Soldier* sold2) {
	Eigen::Vector2d dx = sold2->pos - sold1->pos;
	double dist = dx.norm();
	double drad = sold1->rad + sold2->rad;
	//if(dist < drad || true)
		KnockKnock(sold1, sold2);
	if(sold1->unit->player != sold2->unit->player) {
		double meleeDist = dist - drad;
		if(meleeDist < 5*sold1->meleeRange) {
			if(meleeDist < sold1->meleeRange) 
				sold1->enemiesInMeleeRange.push(SoldierNeighbourContainer(sold2, dx, dist, true));			
			else if(sold1->unit->orders.at(sold1->currentOrder)->type != ORDER_ATTACK)
					sold1->enemiesInMeleeRange.push(SoldierNeighbourContainer(sold2, dx, dist, false));
		}
		if(meleeDist < 4*sold2->meleeRange) {
			if(meleeDist < sold2->meleeRange) 
				sold2->enemiesInMeleeRange.push(SoldierNeighbourContainer(sold1, dx, dist, true));			
			else if(sold2->unit->orders.at(sold2->currentOrder)->type != ORDER_ATTACK)
				sold2->enemiesInMeleeRange.push(SoldierNeighbourContainer(sold1, dx, dist, false));
		}
		/*if(dist - drad < sold1->meleeRange) {
			sold1->enemiesInMeleeRange.push(SoldierNeighbourContainer(sold2, dx, dist, true));
		}
		if(dist - drad < sold2->meleeRange) {
			sold2->enemiesInMeleeRange.push(SoldierNeighbourContainer(sold1, dx, dist, true));
		}*/
	}
}

void CollisionResolution(Map* map) {
	for(int i = 0; i < map->nrows; i++) {
		for(int j = 0; j < map->ncols; j++) {
			Soldier* sold1;
			Soldier* sold2;
			gridpiece* tile1 = map->tiles.at(i).at(j);
			auto soldNode1 = tile1->soldiers.begin();
			while(soldNode1 != tile1->soldiers.end()) {
				sold1 = (*soldNode1);
				auto soldNode2 = std::next(soldNode1);
				while(soldNode2 != tile1->soldiers.end()) {
					sold2 = (*soldNode2);
					EvaluateRange(sold1, sold2);
					soldNode2 = std::next(soldNode2);
				}
				auto neighbour = tile1->neighbours.begin();
				while(neighbour != tile1->neighbours.end()) {
					gridpiece* tile2 = (*neighbour);
					soldNode2 = tile2->soldiers.begin();
					while(soldNode2 != tile2->soldiers.end()) {
						sold2 = (*soldNode2);
						EvaluateRange(sold1, sold2);
						soldNode2 = std::next(soldNode2);
					}
					neighbour = std::next(neighbour);
				}
				soldNode1 = std::next(soldNode1);
			}
		}
	}
}

void MapObjectCollisionHandling(Map* map) {
	for(int i = 0; i < map->nrows; i++) {
		for(int j = 0; j < map->ncols; j++) {
			gridpiece* tile = map->tiles.at(i).at(j);
			for(auto soldier : tile->soldiers) {
				if(soldier) {
					for(auto object : tile->mapObjects) {
						switch(object->type()) {
						case MAP_RECTANGLE:
						case MAP_BORDER:
							SoldierRectangleCollision(soldier, dynamic_cast<Rrectangle*>(object));
							break;
						case MAP_CIRCLE:
							SoldierCircleCollision(soldier, dynamic_cast<Circle*>(object));
							break;
						}
					}
				}
			}
		}
	}
}

double projectile_flight_time(Eigen::Vector2d r_target_rel, Eigen::Vector2d v_target, double vel_projectile) {
	// returns minimum possible time for colllision of projectile and target (center)
	//   if projectile too slow to reach target return -1
	//   r_target_rel = r_target - r_shooter
	double vel_target = v_target.norm();
	double dv = vel_target*vel_target - vel_projectile*vel_projectile;
	double ph = v_target.dot(r_target_rel) / dv;
	double q = pow(r_target_rel.norm(), 2) / dv;
	double rad = ph*ph - q; //radicant
	if(rad < 0) {
		return -1;
	}
	else {
		double srad = sqrt(rad);
		if(srad > -ph)
			return -ph + srad;
		return -ph - srad;
	}
}

struct Displacement{
	Eigen::Matrix2d rot;
	double t;

	Displacement(Eigen::Matrix2d rot, double t) {
		this->rot = rot;
		this->t = t;
	}
};

Displacement ShotAngle(Eigen::Vector2d r_target_rel, Eigen::Vector2d v_target, double vel_projectile, double t, double tans) {
	// tans is the tangens of the maximum angle error
	Eigen::Vector2d r_target_new = r_target_rel + t*v_target;
	
	double re = r_target_new.norm() * tans;
	Eigen::Vector2d miss;
	miss << 1., 1.;
	while(miss.norm() > 1) {
		double randx = 2*(static_cast<float>(rand()) / static_cast<float>(RAND_MAX)) - 0.5;
		double randy = 2*(static_cast<float>(rand()) / static_cast<float>(RAND_MAX)) - 0.5;
		miss << randx, randy;
	}
	r_target_new += re * miss;

	Eigen::Matrix2d rot;
	double cos = r_target_new(0) / (t*vel_projectile);
	double sin = r_target_new(1) / (t*vel_projectile);
	rot << cos, -sin, sin, cos;

	return Displacement(rot, r_target_new.norm() / vel_projectile);
	// does this work without new?
}

#endif