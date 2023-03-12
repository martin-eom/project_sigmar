#ifndef UNITS
#define UNITS

#include <soldiers.h>
#include <orders.h>
#include <player.h>
#include <extra_math.h>

#include <iostream>
#include <vector>

class Unit;
//class Player;

void Populate(Unit* unit);
void Place(Unit* unit, Eigen::Vector2d pos, Eigen::Matrix2d rot);
void MoveTarget(Unit* unit, Eigen::Vector2d pos, Eigen::Matrix2d rot);
void UpdatePos(Unit* unit);
void UpdateVel(Unit* unit);
void PosInUnitByID(Unit* unit);

//SOLDIER_TYPES (handled in soldiers.h)

enum UNIT_TYPE {
	UNIT_GENERIC,
	UNIT_TEMPLATE,
	UNIT_INFANTRY,
	UNIT_CAVALRY,
	UNIT_MONSTER,
	UNIT_LONE_RIDER
};

class Unit {
	public:
		virtual void ConversionEnabler() {}
		//array width and length have to be set manually in every declaration and defintion because there could only be one flexible array member but more would be needed
		int maxSoldiers = 1;
		double spacing = 0;
		int width = 1;
		int nrows = 1;//maxSoldiers / width;	// being not fun
		int soldierType = SOLDIER_SOLDIER;
		int type = UNIT_GENERIC;
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
		int targetUpdateCounter = 60;
		std::vector<Unit*> targetedBy;
		
		void init() {
			soldiers = std::vector<std::vector<Soldier*>>(nrows, std::vector<Soldier*>(width, NULL));
			posInUnit = std::vector<std::vector<Eigen::Vector2d>>(nrows, std::vector<Eigen::Vector2d>(width, Eigen::Vector2d()));
			nLiveSoldiers = 0;
			Populate(this);
			PosInUnitByID(this);
			enemyContact = false;
		}

		Unit(bool subclass, Player* player) {
			nSoldiers = 0;
			placed = false;
			this->player = player;
			if(!subclass) {
				//maxSoldiers = 1;
				//spacing = 0;
				//width = 1;
				//nrows = 1;
				//soldierType = SOLDIER_SOLDIER;
				//type = UNIT_GENERIC;

				init();
			}
		}

		Unit(Player* player) : Unit(false, player) {}
				
};

class UnitSubClassTemplate : public Unit {
	public:
		UnitSubClassTemplate(Player* player) : Unit(true, player) {
			maxSoldiers = 16;
			spacing = 50.;
			width = 4;
			nrows = 4;
			soldierType = SOLDIER_SUBCLASSTEMPLATE;
			Populate(this);
			PosInUnitByID(this);
			type = UNIT_TEMPLATE;

			init();
		};
};

class Infantry : public Unit {
	public:
		Infantry(Player* player) : Unit(true, player) {
			maxSoldiers = 90;
			spacing = 12.;
			width = 10;
			nrows = 9;
			soldierType = SOLDIER_INFANTRYMAN;
			type = UNIT_INFANTRY;

			init();
		};
};

class Cavalry : public Unit {
	public:
		Cavalry(Player* player) : Unit(true, player) {
			maxSoldiers = 16;
			spacing = 18.;
			width = 4;
			nrows = 4;
			soldierType = SOLDIER_RIDER;
			type = UNIT_CAVALRY;

			init();
		};
};

class MonsterUnit : public Unit {
	public:
		MonsterUnit(Player* player) : Unit(true, player) {
			maxSoldiers = 1;
			spacing = 0.;
			width = 1;
			nrows = 1;
			soldierType = SOLDIER_MONSTER;
			type = UNIT_MONSTER;

			init();
		};
};

class LoneRider : public Unit {
	public:
		LoneRider(Player* player) : Unit(true, player) {
			maxSoldiers = 1;
			spacing = 0.;
			width = 1;
			nrows = 1;
			soldierType = SOLDIER_RIDER;
			type = UNIT_TEMPLATE;

			init();
		};
};

void Populate(Unit* unit) {
	std::vector<std::vector<Soldier*>>* soldiers = &(unit->soldiers);
	for(int i = 0; i < unit->nrows; i++) {
		for(int j = 0; j < unit->width; j++) {
			switch (unit->soldierType) {
				case SOLDIER_SOLDIER:
					(*soldiers).at(i).at(j) = new Soldier(unit); break;
				case SOLDIER_SUBCLASSTEMPLATE:
					(*soldiers).at(i).at(j) = new SoldierSubClassTemplate(unit); break;
				case SOLDIER_INFANTRYMAN:
					(*soldiers).at(i).at(j) = new InfantryMan(unit); break;
				case SOLDIER_RIDER:
					(*soldiers).at(i).at(j) = new Rider(unit); break;
				case SOLDIER_MONSTER:
					(*soldiers).at(i).at(j) = new Monster(unit); break;
				default:
					std::cout << "[ERROR:] Population of Unit: Soldier type does not exist!\n";
			}
			unit->liveSoldiers.push_back(unit->soldiers.at(i).at(j));
			unit->nLiveSoldiers++;
		}
	}
};

void Place(Unit* unit, Eigen::Vector2d pos, Eigen::Matrix2d rot) {
	std::vector<std::vector<Soldier*>>* soldiers = &(unit->soldiers);
	std::vector<std::vector<Eigen::Vector2d>>* posInUnit = &(unit->posInUnit);
	for(int i = 0; i < unit->nrows; i++) {
		for (int j = 0; j < unit->width; j++) {
			if (unit->nSoldiers < unit->maxSoldiers) {
				Soldier* soldier = (*soldiers)[i][j];
				soldier->pos = pos + rot * (*posInUnit)[i][j];
				soldier->posTarget = soldier->pos;
				soldier->rot = rot;
				soldier->angle = Angle(rot.coeff(0,1), rot.coeff(0,0));
				soldier->rotTarget = soldier->rot;
				soldier->angleTarget = Angle(rot.coeff(0,1),rot.coeff(0,0));
				soldier->vel << 0., 0.;
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
		for(int j = 0; j < unit->width; j++) {
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
		for(int j = 0; j < unit->width; j++) {
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
		for(int j = 0; j < unit->width; j++) {
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
	double x0 = unit->spacing*(unit->nrows - 1)/2.;
	double y0 = unit->spacing*(unit->width - 1)/2.;
	for(int i = 0; i < unit->nrows; i++) {
		for(int j = 0; j < unit->width; j++) {
			(*posInUnit).at(i).at(j) << x0 - i*unit->spacing, y0 - j*unit->spacing;
		}
	}
}

bool CurrentOrderCompleted(Unit* unit) {
	if(unit->placed) {
		Order* currentOrder = unit->orders.at(unit->currentOrder);
		switch(currentOrder->type) {
		case ORDER_ATTACK:
			return (dynamic_cast<AttackOrder*>(currentOrder)->target->nLiveSoldiers) <= 0; break;
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
		soldier->chargeGapTicks = 30;
	}
}

Rrectangle SoldierRectangle(Soldier* soldier) {
	Unit* unit = soldier->unit;
	double halfWidth = (unit->width - 1) * unit->spacing * 0.5 + 1.*soldier->rad;
	double halfDepth = (unit->nrows - 1) * unit->spacing * 0.5 + 1.*soldier->rad;
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
	double halfWidth = (unit->width - 1) * unit->spacing * 0.5;
	double halfDepth = (unit->nrows - 1) * unit->spacing * 0.5;
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

Rrectangle UnitRectangle(Unit* unit, int orderID, std::vector<Order*> orders) {
	double halfWidth = (unit->width - 1) * unit->spacing * 0.5;
	double halfDepth = (unit->nrows - 1) * unit->spacing * 0.5;
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

#endif