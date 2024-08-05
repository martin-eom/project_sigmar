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

//void Populate(Unit* unit, std::map<std::string, SoldierInformation> classMap);
//void Place(Unit* unit, Eigen::Vector2d pos, Eigen::Matrix2d rot);
//void MoveTarget(Unit* unit, Eigen::Vector2d pos, Eigen::Matrix2d rot);
//void UpdatePos(Unit* unit);
//void UpdateVel(Unit* unit);
//void PosInUnitByID(Unit* unit);


class Unit {
public:
	// loaded stats
	std::string tag;
	int maxSoldiers = 1;
	int ncols = 1;
	int nrows = 1;
	double xspacing = 0.;
	double yspacing = 0.;
	std::string soldierType;
	bool ranged = false;
	double range = 0.;
	double rangedAngle = 0.;
	// generated members
	Player* player;
	int model_index;
	std::vector<std::vector<Soldier*>> soldiers;//{_nrows, std::vector<Soldier*>(_width, NULL)};
	std::vector<Soldier*> liveSoldiers;
	std::vector<std::vector<Eigen::Vector2d>> posInUnit;;//{_nrows, std::vector<Eigen::Vector2d>(_width, Eigen::Vector2d())};
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
	Timer rangedTargetUpdateTimer = Timer(int(80 + (rand()/RAND_MAX)*20));
	Unit* rangedTarget = NULL;
	std::vector<Unit*> targetedBy;
	
	void Populate(std::map<std::string, SoldierInformation> classMap);
	void Place(Eigen::Vector2d pos, Eigen::Matrix2d rot);
	void MoveTarget();
	void UpdatePos();
	void UpdateVel();
	void PosInUnitByID();
	bool CurrentOrderCompleted();
	void NextOrder();
	void DeleteObsoleteOrder();

	void init(std::map<std::string, SoldierInformation> classMap) {
		soldiers = std::vector<std::vector<Soldier*>>(nrows, std::vector<Soldier*>(ncols, NULL));
		posInUnit = std::vector<std::vector<Eigen::Vector2d>>(nrows, std::vector<Eigen::Vector2d>(ncols, Eigen::Vector2d()));
		nLiveSoldiers = 0;
		//Populate(this, classMap);
		Populate(classMap);
		//PosInUnitByID(this);
		PosInUnitByID();
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
};

struct UnitSorter {
	inline bool operator() (const Unit* unit1, const Unit* unit2) {
		return (unit1->nLiveSoldiers < unit2->nLiveSoldiers);
	}
};

void Unit::Populate(std::map<std::string, SoldierInformation> classMap) {
	//std::vector<std::vector<Soldier*>>* soldiers = &(this->soldiers);
	std::vector<std::vector<Soldier*>>* soldiers = &(this->soldiers);
	for(int i = 0; i < nrows; i++) {
		for(int j = 0; j < ncols; j++) {
			(*soldiers).at(i).at(j) = new Soldier(classMap.at(soldierType), this);
			liveSoldiers.push_back(this->soldiers.at(i).at(j));
			nLiveSoldiers++;
			if(this->soldiers.at(i).at(j)->ranged)
				this->soldiers.at(i).at(j)->ReloadTimer.unset();
		}
	}
};

void Unit::Place(Eigen::Vector2d pos, Eigen::Matrix2d rot) {
	std::vector<std::vector<Soldier*>>* soldiers = &(this->soldiers);
	std::vector<std::vector<Eigen::Vector2d>>* posInUnit = &(this->posInUnit);
	for(int i = 0; i < nrows; i++) {
		for (int j = 0; j < ncols; j++) {
			if (nSoldiers < maxSoldiers) {
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
				nSoldiers++;
				soldier->placed = true;
				soldier->currentOrder = 0;
				soldier->arrived = false;
			}
		}
	}
	this->pos = pos;
	posTarget = this->pos;
	this->rot = rot;
	rotTarget = this->rot;
	vel << 0., 0.;
	placed = true;
	nSoldiersArrived = 0;
	nSoldiersOnFirstOrder = nSoldiers;
	std::cout  << "Unit placed with " << nLiveSoldiers << " live soldiers.\n";
}


void Unit::MoveTarget() {
	std::vector<std::vector<Soldier*>>* soldiers = &(this->soldiers);
	std::vector<std::vector<Eigen::Vector2d>>* posInUnit = &(this->posInUnit);
	for(int i = 0; i < nrows; i++) {
		for(int j = 0; j < ncols; j++) {
			Soldier* soldier = soldiers->at(i).at(j);
			if(soldier->placed && soldier->alive) {	//change to something like soldier->alive
				Order* o = orders.at(soldier->currentOrder);
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
	Order* o = orders.at(currentOrder);
	if(o->type == ORDER_MOVE || true) {
		//MoveOrder* mo = dynamic_cast<MoveOrder*>(o);
		posTarget = o->pos;
		this->rot = o->rot;
		rotTarget = o->rot;
	}
}

void Unit::UpdatePos() {
	std::vector<std::vector<Soldier*>>* soldiers = &(this->soldiers);
	int nSoldiers = 0;
	Eigen::Vector2d pos;
	pos << 0., 0.;
	for(int i = 0; i < nrows; i++) {
		for(int j = 0; j < ncols; j++) {
			Soldier* soldier = (*soldiers)[i][j];
			if(soldier->placed && soldier->alive) {
				if(soldier->currentOrder == currentOrder) {
					pos += soldier->pos;
					nSoldiers++;
				}
			}
		}
	}
	if(nSoldiers > 0)
		pos /= nSoldiers;
	this->pos = pos;
}

void Unit::UpdateVel() {
	std::vector<std::vector<Soldier*>>* soldiers = &(this->soldiers);
	int nSoldiers = 0;
	Eigen::Vector2d vel;
	vel << 0., 0.;
	for(int i = 0; i < nrows; i++) {
		for(int j = 0; j < ncols; j++) {
			Soldier* soldier = (*soldiers)[i][j];
			if(soldier->placed && soldier->alive) {
				if(soldier->currentOrder == currentOrder) {
					vel += soldier->vel;
					nSoldiers++;
				}
			}
		}
	}
	vel /= nSoldiers;
	this->vel = vel;
}

void Unit::PosInUnitByID() {
	//std::vector<std::vector<Soldier*>>* soldiers = unit->soldiers();
	std::vector<std::vector<Eigen::Vector2d>>* posInUnit = &(this->posInUnit);
	double x0 = xspacing*(nrows - 1)/2.;
	double y0 = yspacing*(ncols - 1)/2.;
	for(int i = 0; i < nrows; i++) {
		for(int j = 0; j < ncols; j++) {
			(*posInUnit).at(i).at(j) << x0 - i*xspacing, y0 - j*yspacing;
		}
	}
}

bool Unit::CurrentOrderCompleted() {
	if(placed) {
		Order* currentOrder = orders.at(this->currentOrder);
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
				return nSoldiersArrived >= 0.9*nLiveSoldiers; break;
			}
			else {
				//debug("Passing through:");
				//std::cout << (unit->nSoldiersArrived > 0) << "\n";
				return nSoldiersArrived > 0; break;
			}
		}
		default:
			return false; break;
		}
	}
}

void Unit::NextOrder() {
	Order* o = orders.at(currentOrder);
	if(o->type == ORDER_ATTACK) {
		std::erase(dynamic_cast<AttackOrder*>(o)->target->targetedBy, this);
		enemyContact = false;
	}
	currentOrder++;
	nSoldiersArrived = 0;
	Order* no = orders.at(currentOrder);
	if(!no->target || no->target != o->target)
		enemyContact = false;
	std::cout << "Onwards to the next order!\n";
}

void Unit::DeleteObsoleteOrder() {
	if(orders.size() > 1) {
		orders.erase(orders.begin());
		currentOrder--;
		std::vector<std::vector<Soldier*>> soldiers = this->soldiers;
		for(auto row : soldiers) {
			for(auto soldier: row) {
				if(soldier->alive)
					soldier->currentOrder--;
				if(soldier->currentOrder == 0 && soldier->placed && soldier->alive) {
					nSoldiersOnFirstOrder++;
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