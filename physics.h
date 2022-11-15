#define PHYSICS

#ifndef MATH
#include "math.h"
#endif
#ifndef BASE
#include "base.h"
#endif
#ifndef SOLDIERS
#include "soldiers.h"
#endif
#ifndef UNITS
#include "units.h"
#endif
#ifndef ORDERS
#include "orders.h"
#endif
#ifndef MAP
#include "map.h"
#endif
#ifndef PLAYER
#include "player.h"
#endif

#ifndef _USE_MATH_DEFINES
#define _USE_MATH_DEFINES
#endif

#include <Dense>
#include <cmath>


void ReformUnit(Unit* unit) {
	struct RebasedSoldier {
		Soldier* soldier;
		Eigen::Vector2d rebasedPos;

		RebasedSoldier(Soldier* soldier, Eigen::Matrix2d reverseRot, Eigen::Vector2d posInUnit) {
			this->soldier = soldier;
			rebasedPos = reverseRot * posInUnit;
		}
	};

	//Eigen::Matrix2d rotDiff = rot.transpose() * unit->rot;
	std::vector<std::vector<Soldier*>>* soldiers = unit->soldiers();
	LinkedList<RebasedSoldier*> temp1;
	LinkedList<RebasedSoldier*> temp2;
	// Sorting soldiers by new "y coordinate" (front-back in formation)
	for(int i = 0; i < unit->nrows(); i++) {
		for(int j = 0; j < unit->width(); j++) {
			Soldier* soldier = soldiers->at(i).at(j);
			if (soldier) {
				Eigen::Matrix2d rot;
				Order* o = unit->orders.at(soldier->currentOrder);
				if(o->type == ORDER_MOVE) {
					MoveOrder* mo = dynamic_cast<MoveOrder*>(o);
					rot = mo->rot;
				}
				else {rot = unit->rot;}
				Eigen::Matrix2d rotDiff = rot.transpose() * unit->rot;
				RebasedSoldier* r = new RebasedSoldier(soldier, rotDiff, rot.transpose() * (soldier->pos - unit->pos));
				int n = 0;
				Node<RebasedSoldier*>* current = temp1.head;
				while(current) {
					if (r->soldier->currentOrder < current->data->soldier->currentOrder) {
						current = current->next;
						n++;
					}
					else if (r->soldier->currentOrder == current->data->soldier->currentOrder && 
						r->rebasedPos[0] < current->data->rebasedPos[0]) {
						current = current->next;
						n++;
					}
					else break;
				}
				temp1.Insert(n, new Node<RebasedSoldier*>(r));
			}
		}
	}
	// Grabbing rows of soldiers and sorting them by currentOrder and new "x coordinate" (left-right in formation)
	while(temp1.length()) {
		LinkedList<RebasedSoldier*> tempRow;
		// Filling tempRow
		while(temp1.length() && tempRow.length() < unit->width()) {
			Node<RebasedSoldier*>* node = temp1.Pop(0);
			Node<RebasedSoldier*>* current = tempRow.head;
			int n = 0;
			while(current) {
				if(node->data->rebasedPos[1] < current->data->rebasedPos[1]) {
					n++;
					current = current->next;
				}
				else break;
			}
			tempRow.Insert(n, node);
		}
		// Emptying tempRow
		while(tempRow.head) {
			temp2.Append(tempRow.Pop(0));
		}
	}
	// Pasting the new order into unit
	for(int i = 0; i < unit->nrows(); i++) {
		for(int j = 0; j < unit->width(); j++) {
			if(temp2.head) {
				Node<RebasedSoldier*>* node = temp2.Pop(0);
				RebasedSoldier* r = node->data;
				(*soldiers).at(i).at(j) = r->soldier;
				delete r;
				delete node;
			}
		}
	}
	PosInUnitByID(unit);
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

void TimeStep(Soldier* soldier, double dt) {
	Eigen::Vector2d newPos, newVel;
	Eigen::Matrix2d newRot;
	double newAngle;
	//new position
	newPos = soldier->pos + soldier->vel * dt;
	//new velocity
	Eigen::Vector2d fullVel = soldier->vel + soldier->knockVel;	//knockVel will have to be atomic eventually
	newVel = fullVel + soldier->force * dt;
	double speed = soldier->vel.norm();
	//new angle and rotation
	Eigen::Matrix2d rotT;
	Eigen::Matrix2d rotDiff;
	double angleDiff;
	Eigen::Matrix2d toRot;
	double maxTurn;
	Eigen::Vector2d dist = soldier->posTarget - soldier->pos;
	double dx = dist.norm();
	bool closeToTarget = dx < soldier->maxSpeed()/4;
	bool movingAway = dist.dot(soldier->vel) < 0.;
	if(closeToTarget) {
		rotT = soldier->rotTarget;
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
		maxTurn = soldier->turn() * dt * (0.1 + 0.9 * std::max((soldier->maxSpeed() - speed), 0.)/soldier->maxSpeed());
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
	//new force
	Eigen::Vector2d newForce;
	if(closeToTarget || abs(angleDiff) < 0.25) {
		newForce = (soldier->posTarget - newPos)*1;
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
		damp = soldier->onTargetDamp();
	}
	else if(movingAway) {
		damp = soldier->onTargetDamp()*1.1;
	}
	else {
		damp = soldier->squareDamp();
		double linDamp = soldier->linearDamp();
		double squDamp = soldier->squareDamp() * soldier->vel.norm();
		damp = std::max(linDamp, squDamp);
	}
	newForce -= soldier->vel*damp; //dampening not finalized		
	//paste new values to soldier
	soldier->pos = newPos;
	soldier->vel = newVel;
	soldier->force = newForce;
	soldier->knockVel << 0., 0.;
	//check if order complete
	bool newestOrder = (soldier->currentOrder == soldier->unit->currentOrder);
	Order* o = soldier->unit->orders.at(soldier->currentOrder);
	if(o->type = ORDER_MOVE) {
		MoveOrder* mo = dynamic_cast<MoveOrder*>(o);
		if(!soldier->arrived) {
			if(newestOrder) {
				if(closeToTarget) {
					soldier->arrived = true;
					soldier->unit->nSoldiersArrived++;
				}
			}
			else {
				std::vector<Eigen::Matrix2d> rectangle = Rectangle(soldier);
				if(PointInRectangle(soldier->pos - mo->pos, rectangle.at(0), rectangle.at(1))) {
					soldier->arrived = true;
				}
			}
		}
	}
}

void KnockKnock(Soldier* sold1, Soldier* sold2) {
	double minDist = sold1->rad() + sold2->rad();
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
			double totalMass = sold1->mass() + sold2->mass();
			Eigen::Vector2d prod = 2. * dxdv * dx / (totalMass * pow(d,2));
			sold1->knockVel += sold2->mass() * prod;
			sold2->knockVel -= sold1->mass() * prod;
			if(sold1->unit->player == sold2->unit->player && sold1->unit == sold2->unit) {
				if(sold1->currentOrder < sold2->currentOrder) {
					sold1->arrived = true;
				}
				else if(sold1->currentOrder > sold2->currentOrder) {
					sold2->arrived = true;
				}
			}
		}
	}
}

void CollisionScrying(Map* map, Unit* unit) {
	for(int i = 0; i < unit->nrows(); i++) {
		for(int j = 0; j < unit->width(); j++) {
			Soldier* soldier = (*unit).soldiers()->at(i).at(j);
			if(soldier->placed) {
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

void CollisionResolution(Map* map) {
	for(int i = 0; i < map->nrows; i++) {
		for(int j = 0; j < map->ncols; j++) {
			Node<Soldier*>* soldNode1;
			Soldier* sold1;
			Node<Soldier*>* soldNode2;
			Soldier* sold2;
			gridpiece* tile1 = map->tiles.at(i).at(j);
			soldNode1 = tile1->soldiers.head;
			while(soldNode1) {
				sold1 = soldNode1->data;
				soldNode2 = soldNode1->next;
				while(soldNode2) {
					sold2 = soldNode2->data;
					if((sold2->pos - sold1->pos).norm() < (sold1->rad() + sold2->rad()) || true) {
						KnockKnock(sold1, sold2);
					}
					soldNode2 = soldNode2->next;
				}
				Node<gridpiece*>* neighbour = tile1->neighbours.head;
				while(neighbour) {
					gridpiece* tile2 = neighbour->data;
					soldNode2 = tile2->soldiers.head;
					while(soldNode2) {
						sold2 = soldNode2->data;
						if((sold2->pos - sold1->pos).norm() < (sold1->rad() + sold2->rad()) ||true) {
							KnockKnock(sold1, sold2);
						}
						soldNode2 = soldNode2->next;
					}
					neighbour = neighbour->next;
				}
				soldNode1 = soldNode1->next;
			}
		}
	}
}