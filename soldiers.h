#define SOLDIERS

#ifndef MATH
#include "math.h"
#endif

#ifndef _USE_MATH_DEFINES
#define _USE_MATH_DEFINES
#endif

#include <cmath>
#include <Dense>


enum SOLDIER_IDS {
	SOLDIER_SOLDIER,
	SOLDIER_SUBCLASSTEMPLATE,
	SOLDIER_INFANTRYMAN,
	SOLDIER_RIDER,
	SOLDIER_MONSTER
};


class Unit;

class Soldier {
	private:
		double _rad = 5.;
		double _mass = 1.;
		double _defaultMaxSpeed = 15.;
		double _maxSpeed = _defaultMaxSpeed;
		double _accel = 3.;	//acceleration the soldier can use
		double _turn = M_PI/1.5;	//turnspeed
		double _linearDamp;	//coefficient for linear dampening (with regard to velocity)
		double _squareDamp;	//coefficient for square dampening
		double _onTargetDamp = 3.;	//dampening when close to posTarget
	public:
		virtual double rad() {return _rad;}
		virtual double mass() {return _mass;}
		virtual double turn() {return _turn;}
		virtual double maxSpeed() {return _maxSpeed;}
		virtual double linearDamp() {return _linearDamp;}
		virtual double squareDamp() {return _squareDamp;}
		virtual double onTargetDamp() {return _onTargetDamp;}	// is linear because velocity is expected to be low
		virtual int type() {return SOLDIER_SOLDIER;}
		double Force, damp, defaultDamp;
		bool placed = false;
		//bool allyClose, enemyClose;
		Eigen::Vector2i gridpos;
		Eigen::Vector2d pos;
		Eigen::Vector2d posTarget;
		Eigen::Vector2d oldPosTarget;
		Eigen::Vector2d vel;
		Eigen::Vector2d knockVel;
		Eigen::Vector2d force;
		Eigen::Matrix2d rot;
		double angle;
		Eigen::Matrix2d rotTarget;
		double angleTarget;
		Unit* unit;
	
		Soldier(Unit* unit){
			Force = _mass*_accel;
			defaultDamp = Force / pow(_defaultMaxSpeed, 1);
			_linearDamp = Force / pow(_defaultMaxSpeed, 1);
			_squareDamp = Force / pow(_defaultMaxSpeed, 2);
			this->unit = unit;
		}
		
		void SetDamp(double speed) {
			_maxSpeed = speed;
			damp = Force / pow(_maxSpeed, 2);
		};
};

class SoldierSubClassTemplate : public Soldier {
	private:
		double _rad = 7.;
		double _mass = 3.;
		double _defaultMaxSpeed = 50.;
		double _maxSpeed = _defaultMaxSpeed;
		double _accel = 6.;
		double _turn = M_PI/3;
		double _linearDamp;	//coefficient for linear dampening (with regard to velocity)
		double _squareDamp;	//coefficient for square dampening
		double _onTargetDamp = 3.;	//dampening when close to posTarget
	public:
		double rad() {return _rad;}
		double mass() {return _mass;}
		double turn() {return _turn;}
		double maxSpeed() {return _maxSpeed;}
		double linearDamp() {return _linearDamp;}
		double squareDamp() {return _squareDamp;}
		double onTargetDamp() {return _onTargetDamp;}
		int type() {return SOLDIER_SUBCLASSTEMPLATE;} // add new value to SOLDIER_IDS when creating a new subclass
		SoldierSubClassTemplate(Unit* unit) : Soldier(unit) {
			Force = _mass*_accel;
			defaultDamp = Force / pow(_defaultMaxSpeed, 1);
			_linearDamp = Force / pow(_defaultMaxSpeed, 1);
			_squareDamp = Force / pow(_defaultMaxSpeed, 2);
		};
};

class InfantryMan : public Soldier {
	private:
		double _rad = 5.;
		double _mass = 1.;
		double _defaultMaxSpeed = 15.;
		double _maxSpeed = _defaultMaxSpeed;
		double _accel = 3.;
		double _turn = M_PI/1.5;
		double _linearDamp;	//coefficient for linear dampening (with regard to velocity)
		double _squareDamp;	//coefficient for square dampening
		double _onTargetDamp = 3.;	//dampening when close to posTarget
	public:
		double rad() {return _rad;}
		double mass() {return _mass;}
		double turn() {return _turn;}
		double maxSpeed() {return _maxSpeed;}
		double linearDamp() {return _linearDamp;}
		double squareDamp() {return _squareDamp;}
		double onTargetDamp() {return _onTargetDamp;}
		int type() {return SOLDIER_INFANTRYMAN;}	// add new value to SOLDIER_IDS when creating a new subclass
		InfantryMan(Unit* unit) : Soldier(unit) {
			Force = _mass*_accel;
			defaultDamp = Force / pow(_defaultMaxSpeed, 1);
			_linearDamp = Force / pow(_defaultMaxSpeed, 1);
			_squareDamp = Force / pow(_defaultMaxSpeed, 2);
		};
};

class Rider : public Soldier {
	private:
		double _rad = 7.;
		double _mass = 4.;
		double _defaultMaxSpeed = 40.;
		double _maxSpeed = _defaultMaxSpeed;
		double _accel = 6.;
		double _turn = M_PI/3;
		double _linearDamp;	//coefficient for linear dampening (with regard to velocity)
		double _squareDamp;	//coefficient for square dampening
		double _onTargetDamp = 2.;	//dampening when close to posTarget
	public:
		double rad() {return _rad;}
		double mass() {return _mass;}
		double turn() {return _turn;}
		double maxSpeed() {return _maxSpeed;}
		double linearDamp() {return _linearDamp;}
		double squareDamp() {return _squareDamp;}
		double onTargetDamp() {return _onTargetDamp;}
		int type() {return SOLDIER_RIDER;}
		Rider(Unit* unit) : Soldier(unit) {
			Force = _mass*_accel;
			defaultDamp = Force / pow(_defaultMaxSpeed, 1);
			_linearDamp = Force / pow(_defaultMaxSpeed, 1);
			_squareDamp = Force / pow(_defaultMaxSpeed, 2);
		};
};

class Monster : public Soldier {
	private:
		double _rad = 15.;
		double _mass = 20.;
		double _defaultMaxSpeed = 40.;
		double _maxSpeed = _defaultMaxSpeed;
		double _accel = 2.8;
		double _turn = M_PI/5;
		double _linearDamp;	//coefficient for linear dampening (with regard to velocity)
		double _squareDamp;	//coefficient for square dampening
		double _onTargetDamp = 3.2;	//dampening when close to posTarget
	public:
		double rad() {return _rad;}
		double mass() {return _mass;}
		double turn() {return _turn;}
		double maxSpeed() {return _maxSpeed;}
		double linearDamp() {return _linearDamp;}
		double squareDamp() {return _squareDamp;}
		double onTargetDamp() {return _onTargetDamp;}
		int type() {return SOLDIER_MONSTER;}
		Monster(Unit* unit) : Soldier(unit) {
			Force = _mass*_accel;
			defaultDamp = Force / pow(_defaultMaxSpeed, 1);
			_linearDamp = Force / pow(_defaultMaxSpeed, 1);
			_squareDamp = Force / pow(_defaultMaxSpeed, 2);
		};
};