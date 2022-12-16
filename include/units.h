#ifndef UNITS
#define UNITS

#include <soldiers.h>
#include <orders.h>
#include <extra_math.h>

#include <iostream>
#include <vector>

class Unit;
class Player;

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
	UNIT_MONSTER
};

class Unit {
	private:
		//array width and length have to be set manually in every declaration and defintion because there could only be one flexible array member but more would be needed
		static const int _maxSoldiers = 1;
		const double _spacing = 0.;
		static const int _width = 1;
		static const int _nrows = 1;//maxSoldiers / width;	// being not fun
	public:
		int type;
		Player* player;
		std::vector<std::vector<Soldier*>> _soldiers{_nrows, std::vector<Soldier*>(_width, NULL)};
		std::vector<std::vector<Eigen::Vector2d>> _posInUnit{_nrows, std::vector<Eigen::Vector2d>(_width, Eigen::Vector2d())};
		virtual int width() {return _width;}
		virtual int nrows() {return _nrows;}
		virtual int maxSoldiers() {return _maxSoldiers;}
		virtual double spacing() {return _spacing;}
		virtual int soldierType() {return SOLDIER_SOLDIER;}
		virtual std::vector<std::vector<Soldier*>>* soldiers() {return &_soldiers;}
		virtual std::vector<std::vector<Eigen::Vector2d>>* posInUnit() {return &_posInUnit;}
		int nSoldiers;
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
		
		Unit(bool subclass, Player* player) {
			if (!subclass) {
				Populate(this);
				PosInUnitByID(this);
			}
			nSoldiers = 0;
			placed = false;
			this->player = player;
			type = UNIT_GENERIC;
		}
		Unit(Player* player) : Unit(false, player) {}
				
};

class UnitSubClassTemplate : public Unit {
	private:
		const int _maxSoldiers = 16;
		const double _spacing = 50.;
		static const int _width = 4;
		static const int _nrows = 4;// keep in mind if this conflicts with _maxSoldiers
	public:
		std::vector<std::vector<Soldier*>> _soldiers{_nrows, std::vector<Soldier*>(_width, NULL)};
		std::vector<std::vector<Eigen::Vector2d>> _posInUnit{_nrows, std::vector<Eigen::Vector2d>(_width, Eigen::Vector2d())};
		int width() {return _width;}
		int nrows() {return _nrows;}
		int maxSoldiers() {return _maxSoldiers;}
		double spacing() {return _spacing;}
		int soldierType() {return SOLDIER_SUBCLASSTEMPLATE;}	// you have to add a new entry to Populate(unit) when creating a new Unit from a new Soldier class
		std::vector<std::vector<Soldier*>>* soldiers() {return &_soldiers;}
		std::vector<std::vector<Eigen::Vector2d>>* posInUnit() {return &_posInUnit;}
		UnitSubClassTemplate(Player* player) : Unit(true, player) {
			Populate(this);
			PosInUnitByID(this);
			type = UNIT_TEMPLATE;
		};
};

class Infantry : public Unit {
	private:
		const int _maxSoldiers = 90;
		const double _spacing = 12.;
		static const int _width = 10;
		static const int _nrows = 9;// keep in mind if this conflicts with _maxSoldiers
	public:
		std::vector<std::vector<Soldier*>> _soldiers{_nrows, std::vector<Soldier*>(_width, NULL)};
		std::vector<std::vector<Eigen::Vector2d>> _posInUnit{_nrows, std::vector<Eigen::Vector2d>(_width, Eigen::Vector2d())};
		int width() {return _width;}
		int nrows() {return _nrows;}
		int maxSoldiers() {return _maxSoldiers;}
		double spacing() {return _spacing;}
		int soldierType() {return SOLDIER_INFANTRYMAN;}	
		std::vector<std::vector<Soldier*>>* soldiers() {return &_soldiers;}
		std::vector<std::vector<Eigen::Vector2d>>* posInUnit() {return &_posInUnit;}
		Infantry(Player* player) : Unit(true, player) {
			Populate(this);
			PosInUnitByID(this);
			type = UNIT_INFANTRY;
		};
};

class Cavalry : public Unit {
	private:
		const int _maxSoldiers = 16;
		const double _spacing = 18.;
		static const int _width = 4;
		static const int _nrows = 4;// keep in mind if this conflicts with _maxSoldiers
	public:
		std::vector<std::vector<Soldier*>> _soldiers{_nrows, std::vector<Soldier*>(_width, NULL)};
		std::vector<std::vector<Eigen::Vector2d>> _posInUnit{_nrows, std::vector<Eigen::Vector2d>(_width, Eigen::Vector2d())};
		int width() {return _width;}
		int nrows() {return _nrows;}
		int maxSoldiers() {return _maxSoldiers;}
		double spacing() {return _spacing;}
		int soldierType() {return SOLDIER_RIDER;}
		std::vector<std::vector<Soldier*>>* soldiers() {return &_soldiers;}
		std::vector<std::vector<Eigen::Vector2d>>* posInUnit() {return &_posInUnit;}
		Cavalry(Player* player) : Unit(true, player) {
			Populate(this);
			PosInUnitByID(this);
			type = UNIT_CAVALRY;
		};
};

class MonsterUnit : public Unit {
	private:
		const int _maxSoldiers = 1;
		const double _spacing = 0.;
		static const int _width = 1;
		static const int _nrows = 1;// keep in mind if this conflicts with _maxSoldiers
	public:
		std::vector<std::vector<Soldier*>> _soldiers{_nrows, std::vector<Soldier*>(_width, NULL)};
		std::vector<std::vector<Eigen::Vector2d>> _posInUnit{_nrows, std::vector<Eigen::Vector2d>(_width, Eigen::Vector2d())};
		int width() {return _width;}
		int nrows() {return _nrows;}
		int maxSoldiers() {return _maxSoldiers;}
		double spacing() {return _spacing;}
		int soldierType() {return SOLDIER_MONSTER;}
		std::vector<std::vector<Soldier*>>* soldiers() {return &_soldiers;}
		std::vector<std::vector<Eigen::Vector2d>>* posInUnit() {return &_posInUnit;}
		MonsterUnit(Player* player) : Unit(true, player) {
			Populate(this);
			PosInUnitByID(this);
			type = UNIT_MONSTER;
		};
};

void Populate(Unit* unit) {
	std::vector<std::vector<Soldier*>>* soldiers = unit->soldiers();
	for(int i = 0; i < unit->nrows(); i++) {
		for(int j = 0; j < unit->width(); j++) {
			switch (unit->soldierType()) {
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
					std::cout << "Population of Unit: Soldier type does not exist!\n";
			}
			
		}
	}
};

void Place(Unit* unit, Eigen::Vector2d pos, Eigen::Matrix2d rot) {
	std::vector<std::vector<Soldier*>>* soldiers = unit->soldiers();
	std::vector<std::vector<Eigen::Vector2d>>* posInUnit = unit->posInUnit();
	for(int i = 0; i < unit->nrows(); i++) {
		for (int j = 0; j < unit->width(); j++) {
			if (unit->nSoldiers < unit->maxSoldiers()) {
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
}


void MoveTarget(Unit* unit) {
	std::vector<std::vector<Soldier*>>* soldiers = unit->soldiers();
	std::vector<std::vector<Eigen::Vector2d>>* posInUnit = unit->posInUnit();
	for(int i = 0; i < unit->nrows(); i++) {
		for(int j = 0; j < unit->width(); j++) {
			Soldier* soldier = soldiers->at(i).at(j);
			if(soldier->placed) {	//change to something like soldier->alive
				Order* o = unit->orders.at(soldier->currentOrder);
				if(o->type == ORDER_MOVE) {
					MoveOrder* mo = dynamic_cast<MoveOrder*>(o);
					//soldier->posTarget = mo->pos + mo->rot * posInUnit->at(i).at(j);
					soldier->posTarget = mo->pos + mo->rot * posInUnit->at(i).at(j);
					soldier->rotTarget = mo->rot;
					soldier->angleTarget = mo->angleTarget;
				}
			}
		}
	}
	Order* o = unit->orders.at(unit->currentOrder);
	if(o->type == ORDER_MOVE) {
		MoveOrder* mo = dynamic_cast<MoveOrder*>(o);
		unit->posTarget = mo->pos;
		unit->rot = mo->rot;
		unit->rotTarget = mo->rot;
	}
}

void UpdatePos(Unit* unit) {
	std::vector<std::vector<Soldier*>>* soldiers = unit->soldiers();
	Eigen::Vector2d pos;
	pos << 0., 0.;
	for(int i = 0; i < unit->nrows(); i++) {
		for(int j = 0; j < unit->width(); j++) {
			Soldier* soldier = (*soldiers)[i][j];
			if(soldier->placed) {
				pos += soldier->pos;
			}
		}
	}
	pos /= unit->nSoldiers;
	unit->pos = pos;
}

void UpdateVel(Unit* unit) {
	std::vector<std::vector<Soldier*>>* soldiers = unit->soldiers();
	Eigen::Vector2d vel;
	vel << 0., 0.;
	for(int i = 0; i < unit->nrows(); i++) {
		for(int j = 0; j < unit->width(); j++) {
			Soldier* soldier = (*soldiers)[i][j];
			if(soldier->placed) {
				vel += soldier->vel;
			}
		}
	}
	vel /= unit->nSoldiers;
	unit->vel = vel;
}

void PosInUnitByID(Unit* unit) {
	//std::vector<std::vector<Soldier*>>* soldiers = unit->soldiers();
	std::vector<std::vector<Eigen::Vector2d>>* posInUnit = unit->posInUnit();
	double x0 = unit->spacing()*(unit->nrows() - 1)/2.;
	double y0 = unit->spacing()*(unit->width() - 1)/2.;
	for(int i = 0; i < unit->nrows(); i++) {
		for(int j = 0; j < unit->width(); j++) {
			(*posInUnit).at(i).at(j) << x0 - i*unit->spacing(), y0 - j*unit->spacing();
		}
	}
}

bool CurrentOrderCompleted(Unit* unit) {
	if(unit->placed) {
		Order* currentOrder = unit->orders.at(unit->currentOrder);
		if(currentOrder->type == ORDER_MOVE) {
			MoveOrder* mo = dynamic_cast<MoveOrder*>(currentOrder);
			if(mo->moveType == MOVE_FORMUP) {
				return unit->nSoldiersArrived >= 0.9*unit->nSoldiers;
			}
			else {
				return unit->nSoldiersArrived > 0;
			}
		}
		else {
			//combat not implemented
			return false;
		}
	}
	else {return false;}
}

void UnitNextOrder(Unit* unit) {
	unit->currentOrder++;
	unit->nSoldiersArrived = 0;
	std::cout << "Onwards!\n";
}

void DeleteObsoleteOrder(Unit* unit) {
	if(unit->orders.size() > 1) {
		unit->orders.erase(unit->orders.begin());
		unit->currentOrder--;
		std::vector<std::vector<Soldier*>> soldiers = *(unit->soldiers());
		for(auto row : soldiers) {
			for(auto soldier: row) {
				soldier->currentOrder--;
				if(soldier->currentOrder == 0 && soldier->placed) {
					unit->nSoldiersOnFirstOrder++;
				}
			}
		}
	}
}

void SoldierNextOrder(Soldier* soldier, Eigen::Vector2d posInUnit) {
	if(soldier->currentOrder == 0) {soldier->unit->nSoldiersOnFirstOrder--;}
	soldier->currentOrder++;
	soldier->arrived = false;
	Order* o = soldier->unit->orders.at(soldier->currentOrder);
	if(o->type == ORDER_MOVE) {
		MoveOrder* mo = dynamic_cast<MoveOrder*>(o);
		soldier->posTarget = mo->pos + mo->rot * posInUnit;
		soldier->rotTarget = mo->rot;
		soldier->angleTarget = mo->angleTarget;
	}
}

Rrectangle SoldierRectangle(Soldier* soldier) {
	Unit* unit = soldier->unit;
	double halfWidth = (unit->width() - 1) * unit->spacing() * 0.5 + 1.*soldier->rad();
	double halfDepth = (unit->nrows() - 1) * unit->spacing() * 0.5 + 1.*soldier->rad();
	Order* o = unit->orders.at(soldier->currentOrder);
	Eigen::Vector2d pos;
	Eigen::Matrix2d rot;
	if(o->type == ORDER_MOVE) {
		MoveOrder* mo = dynamic_cast<MoveOrder*>(o);
		pos = mo->pos;
		rot = mo->rot;
	}
	return Rrectangle(halfWidth, halfDepth, pos, rot);
}

Rrectangle UnitRectangle(Unit* unit, int orderID) {
	double halfWidth = (unit->width() - 1) * unit->spacing() * 0.5;
	double halfDepth = (unit->nrows() - 1) * unit->spacing() * 0.5;
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
	double halfWidth = (unit->width() - 1) * unit->spacing() * 0.5;
	double halfDepth = (unit->nrows() - 1) * unit->spacing() * 0.5;
	Order* o = orders.at(orderID);
	Eigen::Vector2d pos;
	Eigen::Matrix2d rot;
	if(o->type == ORDER_MOVE) {
		MoveOrder* mo = dynamic_cast<MoveOrder*>(o);
		pos = mo->pos;
		rot = mo->rot;
	}
	return Rrectangle(halfWidth, halfDepth, pos, rot);
}

#endif