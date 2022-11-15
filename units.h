#define UNITS

#ifndef SOLDIERS
#include "soldiers.h"
#endif
#ifndef ORDERS
#include "orders.h"
#endif
#ifndef MATH
#include "math.h"
#endif

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

class Unit {
	private:
		//array width and length have to be set manually in every declaration and defintion because there could only be one flexible array member but more would be needed
		static const int _maxSoldiers = 1;
		const double _spacing = 0.;
		static const int _width = 1;
		static const int _nrows = 1;//maxSoldiers / width;	// being not fun
	public:
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
	std::vector<std::vector<Soldier*>>* soldiers = unit->soldiers();
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
}

void UnitNextOrder(Unit* unit) {
	unit->currentOrder++;
	unit->nSoldiersArrived = 0;
	std::cout << "Onwards!\n";
}

void SoldierNextOrder(Soldier* soldier, Eigen::Vector2d posInUnit) {
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

std::vector<Eigen::Matrix2d> Rectangle(Soldier* soldier) {	//rectangle is defined by unrotated top left and bottom right points
	Eigen::Matrix2d rec;
	Eigen::Matrix2d rot;
	std::vector<Eigen::Matrix2d> rectangle;
	Unit* unit = soldier->unit;
	double halfWidth = (unit->width() - 1) * unit->spacing() * 0.5 + 1.*soldier->rad();
	double halfDepth = (unit->nrows() - 1) * unit->spacing() * 0.5 + 1.*soldier->rad();
	Order* o = unit->orders.at(soldier->currentOrder);
	if(o->type == ORDER_MOVE) {
		MoveOrder* mo = dynamic_cast<MoveOrder*>(o);
		rec << - halfDepth, + halfWidth,
			   halfDepth, - halfWidth;
		rot = mo->rot;
	}
	rectangle.push_back(rec);
	rectangle.push_back(rot);
	return rectangle;
}

std::vector<Eigen::Matrix2d> Rectangle(Unit* unit, int orderID) {	//translated to 0 0
	Eigen::Matrix2d rec;
	Eigen::Matrix2d rot;
	std::vector<Eigen::Matrix2d> rectangle;
	double halfWidth = (unit->width() - 1) * unit->spacing() * 0.5;
	double halfDepth = (unit->nrows() - 1) * unit->spacing() * 0.5;
	Order* o = unit->orders.at(orderID);
	if(o->type == ORDER_MOVE) {
		MoveOrder* mo = dynamic_cast<MoveOrder*>(o);
		rec << - halfDepth, halfWidth,
			   halfDepth, - halfWidth;
		rot = mo->rot;
	}
	rectangle.push_back(rec);
	rectangle.push_back(rot);
	return rectangle;
}