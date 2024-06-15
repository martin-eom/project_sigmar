#ifndef UNITS
#define UNITS

#include <soldiers.h>
#include <orders.h>
#include <player.h>
#include <timer.h>
#include <extra_math.h>

#include <iostream>
#include <vector>
#include <cstdlib>
#include <map>

class Unit;
//class Player;

void Populate(Unit* unit, std::map<std::string, SoldierInformation> classMap);
void Place(Unit* unit, Eigen::Vector2d pos, Eigen::Matrix2d rot);
void MoveTarget(Unit* unit, Eigen::Vector2d pos, Eigen::Matrix2d rot);
void UpdatePos(Unit* unit);
void UpdateVel(Unit* unit);
void PosInUnitByID(Unit* unit);


class Unit {
	public:
		//virtual void ConversionEnabler() {}
		//array width and length have to be set manually in every declaration and defintion because there could only be one flexible array member but more would be needed
		int maxSoldiers = 1;
		//double spacing = 0;
		double xspacing = 0.;
		double yspacing = 0.;
		int ncols = 1;//int width = 1;
		int nrows = 1;//maxSoldiers / width;	// being not fun
		//int soldierType = SOLDIER_SOLDIER;
		//int type = UNIT_GENERIC;
		std::string tag;
		std::string soldierType;
		Player* player;
		std::vector<std::vector<Soldier*>> soldiers;//{_nrows, std::vector<Soldier*>(_width, NULL)};
		std::vector<Soldier*> liveSoldiers;
		std::vector<std::vector<Eigen::Vector2d>> posInUnit;;//{_nrows, std::vector<Eigen::Vector2d>(_width, Eigen::Vector2d())};
		//virtual std::vector<std::vector<Soldier*>>* soldiers() {return &_soldiers;}
		//virtual std::vector<std::vector<Eigen::Vector2d>>* posInUnit() {return &_posInUnit;}
		int nSoldiers;
		int nLiveSoldiers;
		int currentOrder;
		int nSoldiersArrived;
		int nSoldiersOnFirstOrder;
		bool placed;
		Eigen::Vector2d pos;
		Eigen::Vector2d posTarget;
		Eigen::Matrix2d rot;
		Eigen::Matrix2d rotTarget;
		std::vector<Order*> orders;
		Eigen::Vector2d vel;
		bool enemyContact;
		Timer targetUpdateTimer = Timer(60);
		bool ranged = false;
		double range = 0.;
		double rangedAngle = 0.;
		Timer rangedTargetUpdateTimer = Timer(int(80 + (rand()/RAND_MAX)*20));
		Unit* rangedTarget = NULL;
		//int targetUpdateCounter = 60;
		std::vector<Unit*> targetedBy;
		
		void init(std::map<std::string, SoldierInformation> classMap) {
			soldiers = std::vector<std::vector<Soldier*>>(nrows, std::vector<Soldier*>(ncols, NULL));
			posInUnit = std::vector<std::vector<Eigen::Vector2d>>(nrows, std::vector<Eigen::Vector2d>(ncols, Eigen::Vector2d()));
			nLiveSoldiers = 0;
			Populate(this, classMap);
			PosInUnitByID(this);
			enemyContact = false;
		}

		Unit(UnitInformation info, Player* player, std::map<std::string, SoldierInformation> classMap) {
			tag = info.tag;
			soldierType = info.soldier_type;
			maxSoldiers = info.formation_max_soldiers;
			ncols = info.formation_columns;
			nrows = info.formation_rows;
			xspacing = info.formation_x_spacing;
			yspacing = info.formation_y_spacing;

			ranged = info.ranged_ranged;
			range = info.ranged_range;
			rangedAngle = info.ranged_angle;

			this->player = player;

			nSoldiers = 0;
			placed = false;

			init(classMap);

		}

		//Unit(Player* player) : Unit(false, player) {}
				
};

void Populate(Unit* unit, std::map<std::string, SoldierInformation> classMap) {
	std::vector<std::vector<Soldier*>>* soldiers = &(unit->soldiers);
	for(int i = 0; i < unit->nrows; i++) {
		for(int j = 0; j < unit->ncols; j++) {
			(*soldiers).at(i).at(j) = new Soldier(classMap.at(unit->soldierType), unit);
			unit->liveSoldiers.push_back(unit->soldiers.at(i).at(j));
			unit->nLiveSoldiers++;
			if(unit->soldiers.at(i).at(j)->ranged)
				unit->soldiers.at(i).at(j)->ReloadTimer.unset();
		}
	}
};

void Place(Unit* unit, Eigen::Vector2d pos, Eigen::Matrix2d rot) {
	std::vector<std::vector<Soldier*>>* soldiers = &(unit->soldiers);
	std::vector<std::vector<Eigen::Vector2d>>* posInUnit = &(unit->posInUnit);
	for(int i = 0; i < unit->nrows; i++) {
		for (int j = 0; j < unit->ncols; j++) {
			if (unit->nSoldiers < unit->maxSoldiers) {
				Soldier* soldier = (*soldiers)[i][j];
				soldier->pos = pos + rot * (*posInUnit)[i][j];
				soldier->posTarget = soldier->pos;
				soldier->rot = rot;
				soldier->angle = Angle(rot.coeff(0,1), rot.coeff(0,0));
				soldier->rotTarget = soldier->rot;
				soldier->angleTarget = Angle(rot.coeff(0,1),rot.coeff(0,0));
				soldier->vel << 0., 0.;
				soldier->speed = 0.;
				soldier->forwardSpeed = 0.;
				soldier->knockVel << 0., 0.;
				soldier->force << 0., 0.;
				unit->nSoldiers++;
				soldier->placed = true;
				soldier->currentOrder = 0;
				soldier->arrived = false;
			}
		}
	}
	unit->pos = pos;
	unit->posTarget = unit->pos;
	unit->rot = rot;
	unit->rotTarget = unit->rot;
	unit->vel << 0., 0.;
	unit->placed = true;
	unit->nSoldiersArrived = 0;
	unit->nSoldiersOnFirstOrder = unit->nSoldiers;
	std::cout  << "Unit placed with " << unit->nLiveSoldiers << " live soldiers.\n";
}


void MoveTarget(Unit* unit) {
	std::vector<std::vector<Soldier*>>* soldiers = &(unit->soldiers);
	std::vector<std::vector<Eigen::Vector2d>>* posInUnit = &(unit->posInUnit);
	for(int i = 0; i < unit->nrows; i++) {
		for(int j = 0; j < unit->ncols; j++) {
			Soldier* soldier = soldiers->at(i).at(j);
			if(soldier->placed && soldier->alive) {	//change to something like soldier->alive
				Order* o = unit->orders.at(soldier->currentOrder);
				if(o->type == ORDER_MOVE ||true) {
					//MoveOrder* mo = dynamic_cast<MoveOrder*>(o);
					//soldier->posTarget = mo->pos + mo->rot * posInUnit->at(i).at(j);
					soldier->posTarget = o->pos + o->rot * posInUnit->at(i).at(j);
					soldier->rotTarget = o->rot;
					soldier->angleTarget = o->angleTarget;
				}
			}
		}
	}
	Order* o = unit->orders.at(unit->currentOrder);
	if(o->type == ORDER_MOVE || true) {
		//MoveOrder* mo = dynamic_cast<MoveOrder*>(o);
		unit->posTarget = o->pos;
		unit->rot = o->rot;
		unit->rotTarget = o->rot;
	}
}

void UpdatePos(Unit* unit) {
	std::vector<std::vector<Soldier*>>* soldiers = &(unit->soldiers);
	int nSoldiers = 0;
	Eigen::Vector2d pos;
	pos << 0., 0.;
	for(int i = 0; i < unit->nrows; i++) {
		for(int j = 0; j < unit->ncols; j++) {
			Soldier* soldier = (*soldiers)[i][j];
			if(soldier->placed && soldier->alive) {
				if(soldier->currentOrder == unit->currentOrder) {
					pos += soldier->pos;
					nSoldiers++;
				}
			}
		}
	}
	if(nSoldiers > 0)
		pos /= nSoldiers;
	unit->pos = pos;
}

void UpdateVel(Unit* unit) {
	std::vector<std::vector<Soldier*>>* soldiers = &(unit->soldiers);
	int nSoldiers = 0;
	Eigen::Vector2d vel;
	vel << 0., 0.;
	for(int i = 0; i < unit->nrows; i++) {
		for(int j = 0; j < unit->ncols; j++) {
			Soldier* soldier = (*soldiers)[i][j];
			if(soldier->placed && soldier->alive) {
				if(soldier->currentOrder == unit->currentOrder) {
					vel += soldier->vel;
					nSoldiers++;
				}
			}
		}
	}
	vel /= nSoldiers;
	unit->vel = vel;
}

void PosInUnitByID(Unit* unit) {
	//std::vector<std::vector<Soldier*>>* soldiers = unit->soldiers();
	std::vector<std::vector<Eigen::Vector2d>>* posInUnit = &(unit->posInUnit);
	double x0 = unit->xspacing*(unit->nrows - 1)/2.;
	double y0 = unit->yspacing*(unit->ncols - 1)/2.;
	for(int i = 0; i < unit->nrows; i++) {
		for(int j = 0; j < unit->ncols; j++) {
			(*posInUnit).at(i).at(j) << x0 - i*unit->xspacing, y0 - j*unit->yspacing;
		}
	}
}

bool CurrentOrderCompleted(Unit* unit) {
	if(unit->placed) {
		Order* currentOrder = unit->orders.at(unit->currentOrder);
		switch(currentOrder->type) {
		case ORDER_ATTACK:
			return (dynamic_cast<AttackOrder*>(currentOrder)->target->nLiveSoldiers) <= 0; break;
		case ORDER_TARGET:
			return (dynamic_cast<TargetOrder*>(currentOrder)->target->nLiveSoldiers) <= 0; break;
		case ORDER_MOVE: {
			debug("Checking order completion...");
			MoveOrder* mo = dynamic_cast<MoveOrder*>(currentOrder);
			if(mo->moveType == MOVE_FORMUP) {
				//debug("Form up:");
				//std::cout << (unit->nSoldiersArrived >= 0.9*unit->nLiveSoldiers) << "\n";
				return unit->nSoldiersArrived >= 0.9*unit->nLiveSoldiers; break;
			}
			else {
				//debug("Passing through:");
				//std::cout << (unit->nSoldiersArrived > 0) << "\n";
				return unit->nSoldiersArrived > 0; break;
			}
		}
		default:
			return false; break;
		}
	}
}

void UnitNextOrder(Unit* unit) {
	Order* o = unit->orders.at(unit->currentOrder);
	if(o->type == ORDER_ATTACK) {
		std::erase(dynamic_cast<AttackOrder*>(o)->target->targetedBy, unit);
		unit->enemyContact = false;
	}
	unit->currentOrder++;
	unit->nSoldiersArrived = 0;
	Order* no = unit->orders.at(unit->currentOrder);
	if(!no->target || no->target != o->target)
		unit->enemyContact = false;
	std::cout << "Onwards to the next order!\n";
}

void DeleteObsoleteOrder(Unit* unit) {
	if(unit->orders.size() > 1) {
		unit->orders.erase(unit->orders.begin());
		unit->currentOrder--;
		std::vector<std::vector<Soldier*>> soldiers = unit->soldiers;
		for(auto row : soldiers) {
			for(auto soldier: row) {
				if(soldier->alive)
					soldier->currentOrder--;
				if(soldier->currentOrder == 0 && soldier->placed && soldier->alive) {
					unit->nSoldiersOnFirstOrder++;
				}
			}
		}
		debug("Deleted first order.");
	}
}

void SoldierNextOrder(Soldier* soldier, Eigen::Vector2d posInUnit) {
	if(soldier->currentOrder == 0) {soldier->unit->nSoldiersOnFirstOrder--;}
	soldier->currentOrder++;
	soldier->arrived = false;
	Order* o = soldier->unit->orders.at(soldier->currentOrder);
	Order* po = soldier->unit->orders.at(soldier->currentOrder - 1);
	soldier->posTarget = o->pos + o->rot * posInUnit;
	soldier->rotTarget = o->rot;
	soldier->angleTarget = o->angleTarget;
	if(!o->target || (o->target != po->target)) {
		soldier->charging = true;
		soldier->chargeTimer.reset();
		//soldier->chargeGapTicks = 30;
	}
}

Rrectangle SoldierRectangle(Soldier* soldier) {
	Unit* unit = soldier->unit;
	double halfWidth = (unit->ncols - 1) * unit->yspacing * 0.5 + 1.*soldier->rad;
	double halfDepth = (unit->nrows - 1) * unit->xspacing * 0.5 + 1.*soldier->rad;
	Order* o = unit->orders.at(soldier->currentOrder);
	Eigen::Vector2d pos;
	Eigen::Matrix2d rot;
	if(o->type == ORDER_MOVE || true) {
		MoveOrder* mo = dynamic_cast<MoveOrder*>(o);
		pos = o->pos;
		rot = o->rot;
	}
	return Rrectangle(halfWidth, halfDepth, pos, rot);
}

Rrectangle UnitRectangle(Unit* unit, int orderID) {
	double halfWidth = (unit->ncols - 1) * unit->yspacing * 0.5;
	double halfDepth = (unit->nrows - 1) * unit->xspacing * 0.5;
	Order* o = unit->orders.at(orderID);
	Eigen::Vector2d pos;
	Eigen::Matrix2d rot;
	if(o->type == ORDER_MOVE) {
		MoveOrder* mo = dynamic_cast<MoveOrder*>(o);
		pos = mo->pos;
		rot = mo->rot;
	}
	return Rrectangle(halfWidth, halfDepth, pos, rot);
}

Rrectangle OnSpotUnitRectangle(Unit* unit) {
	double halfWidth = (unit->ncols - 1) * unit->yspacing * 0.5;
	double halfDepth = (unit->nrows - 1) * unit->xspacing * 0.5;
	return Rrectangle(halfWidth, halfDepth, unit->pos, unit->rot);
};

Rrectangle UnitRectangle(Unit* unit, int orderID, std::vector<Order*> orders) {
	double halfWidth = (unit->ncols - 1) * unit->yspacing * 0.5;
	double halfDepth = (unit->nrows - 1) * unit->xspacing * 0.5;
	Order* o = orders.at(orderID);
	Eigen::Vector2d pos;
	Eigen::Matrix2d rot;
	if(o->type == ORDER_MOVE) {
		MoveOrder* mo = dynamic_cast<MoveOrder*>(o);
		pos = mo->pos;
		rot = mo->rot;
	}
	else {
		pos = o->pos;
		rot = o->rot;
	}
	return Rrectangle(halfWidth, halfDepth, pos, rot);
}

struct UnitDistance {
	Unit* unit;
	double dist;

	UnitDistance(Unit* me, Unit* them) {
		unit = them;
		dist = (me->pos - them->pos).norm();
	}
};

bool compareUnitDistance(const UnitDistance &a, const UnitDistance &b) {
	return a.dist < b.dist;
}

#endif