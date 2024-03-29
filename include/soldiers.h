#ifndef SOLDIERS
#define SOLDIERS

#include <extra_math.h>
#include <timer.h>
#include <debug.h>
#include <projectiles.h>

#ifndef _USE_MATH_DEFINES
#define _USE_MATH_DEFINES
#endif

#include <cmath>
#include <Dense>
#include <ostream>
#include <queue>
#include <cstdlib>

enum SOLDIER_IDS {
	SOLDIER_SOLDIER,
	SOLDIER_SUBCLASSTEMPLATE,
	SOLDIER_INFANTRYMAN,
	SOLDIER_RIDER,
	SOLDIER_MONSTER,
	SOLDIER_SHOOTA,
	SOLDIER_SLOW_SHOOTA
};


class Soldier;
class Unit;

class SoldierNeighbourContainer {
public:
	Soldier* soldier;
	Eigen::Vector2d dx;
	double dist;
	bool inTrueRange;

	SoldierNeighbourContainer() {}
	SoldierNeighbourContainer(Soldier* soldier, Eigen::Vector2d dx, double dist, bool inTrueRange) {
		this->soldier = soldier;
		this->dx = dx;
		this->dist = dist;
		this->inTrueRange = inTrueRange;
	}

};


struct compareContainers {
	bool operator()(const SoldierNeighbourContainer& c1,const SoldierNeighbourContainer& c2) {
		return c1.dist > c2.dist;
	}
};


class Soldier {
	public:
		virtual void ConversionEnabler() {}

		//soldier specific attributes
		double rad = 5;
		double mass = 1;
		double defaultMaxSpeed = 15;
		double maxSpeed = defaultMaxSpeed;
		double accel = 3;	//acceleration the soldier can use
		double turn = M_PI/1.5;	//turnspeed
		double linearDamp;	//coefficient for linear dampening (with regard to velocity)
		double squareDamp;	//coefficient for square dampening
		double onTargetDamp = 3;	//dampening when close to posTarget
		int type = SOLDIER_SOLDIER;

		int maxHP = 2;
		double meleeRange = 0.;
		double meleeAngle = 1./3.;
		virtual int damage(Soldier* target) {return 1;}
		Timer MeleeTimer = Timer(60);
		//int meleeCooldownTicks = 61;
		int meleeAttack = 0;
		int meleeDefense = 0;
		int armor = 0;
		int armorPiercing = 0;
		bool infantry = false;
		bool large = false;
		bool antiInfantry = false;
		bool antiLarge = false;
		bool meleeAOE = false;
		int rangedDefense = 0;
		double rangedAOE = 0.;

		bool ranged = false;
		double rangedRange = 0.;
		double rangedMinRange = 0.;
		double rangedRad = 0.;
		bool rangedHeavy = false;
		Timer DrawTimer = Timer(0);
		Timer ReloadTimer = Timer(0);
		Soldier* rangedTarget = NULL;
		// drawCDTicks = 0;
		//int reloadCDTicks = 0;
		int rangedDamage = 0;
		double rangedSpeed = 0.;
		double maxSpeedForFiring = 0.;
		int projectileType = 0;
		
		//general members
		double Force, damp, defaultDamp;
		bool placed = false;
		int currentOrder;
		bool arrived;
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
		//std::vector<Soldier*> enemiesInMeleeRange;
		std::priority_queue<SoldierNeighbourContainer, std::vector<SoldierNeighbourContainer>, compareContainers> enemiesInMeleeRange;
		int hp;
		bool alive;
		Soldier* meleeTarget;	// target they will turn to eventually
		Soldier* meleeSwingTarget;	// target they are striking at the moment
		bool charging;
		Timer chargeTimer = Timer(30);
		//int chargeGapTicks;
		Timer noTargetTimer = Timer(60);
		//int cantSeeTargetTimer = 0;
		Eigen::Matrix2d meleeCone;
		std::vector<Eigen::Vector2d> indivPath;	// individual pathfinding when los to original next target is lost
		Timer indivPathTimer = Timer(60);
		//int indivPathCooldown = 60;
		//int indivPathCDMax = 60;
		//int drawTimer;
		//int reloadTimer;
		double tans;

		bool debugFlag1;
		bool debugFlag2;
		bool debugFlag3;

		void init(Unit* unit) {
			Force = mass*accel;
			defaultDamp = Force / pow(defaultMaxSpeed, 1);
			linearDamp = Force / pow(defaultMaxSpeed, 1);
			squareDamp = Force / pow(defaultMaxSpeed, 2);

			meleeCone << std::cos(0.5*M_PI*meleeAngle), -std::sin(0.5*M_PI*meleeAngle), std::sin(0.5*M_PI*meleeAngle), std::cos(0.5*M_PI*meleeAngle);

			hp = maxHP;
			alive = true;
			meleeTarget = NULL;
			meleeSwingTarget = NULL;
			charging = true;
			chargeTimer.reset();
			//chargeGapTicks = 30;
			noTargetTimer.unset();
			indivPathTimer.set_max(int((2 + rand()/RAND_MAX)*30));
			//indivPathCDMax = int((2 + rand()/RAND_MAX)*30);
			//drawTimer = 0;
			//reloadTimer = 0;
			int reloadShift = rand()%30 - 15;
			ReloadTimer.set_max(std::max(ReloadTimer.get_max() + reloadShift, 1));
			tans = sin(rangedRad) / cos(rangedRad);

			debugFlag1 = false;
			debugFlag2 = false;
			debugFlag3 = false;

			this->unit = unit;		
		}

		Soldier() {}

		Soldier(Unit* unit) : Soldier(){
			//rad = 5.;
			//mass = 1.;
			//defaultMaxSpeed = 15.;
			//maxSpeed = defaultMaxSpeed;
			//accel = 3.;	//acceleration the soldier can use
			//turn = M_PI/1.5;	//turnspeed
			//onTargetDamp = 3.;	//dampening when close to posTarget
			//type = SOLDIER_SOLDIER;

			//maxHP = 2;
			//meleeRange = 0.;
			//meleeAngle = 1./3.;
			//meleeCooldownTicks = 61;

			init(unit);
		}
		
		void SetDamp(double speed) {
			maxSpeed = speed;
			damp = Force / pow(maxSpeed, 2);
		};

		Circle SoldierCircle() {
			return Circle(pos, rad);
		}
};

class SoldierSubClassTemplate : public Soldier {
	public:		
		SoldierSubClassTemplate(Unit* unit) : Soldier() {
			rad = 7.;
			mass = 3.;
			defaultMaxSpeed = 50.;
			maxSpeed = defaultMaxSpeed;
			accel = 6.;
			turn = M_PI/3;
			onTargetDamp = 3.;	//dampening when close to posTarget
			type = SOLDIER_SUBCLASSTEMPLATE;

			maxHP = 2;
			meleeRange = 0.;
			meleeAngle = 1./3.;
			MeleeTimer.set_max(60);
			//meleeCooldownTicks = 61;

			meleeAttack = 0;
			meleeDefense = 0;
			armor = 0;
			armorPiercing = 0;
			infantry = false;
			large = false;
			antiInfantry = false;
			antiLarge = false;

			init(unit);
		};
		
		int damage(Soldier* target) {return 1;}

};

class InfantryMan : public Soldier {
	public:
		InfantryMan(Unit* unit) : Soldier() {
			rad = 5.;
			mass = 1.;
			defaultMaxSpeed = 15.;
			maxSpeed = defaultMaxSpeed;
			accel = 3.;
			turn = M_PI/1.5;
			onTargetDamp = 3.;	//dampening when close to posTarget
			type = SOLDIER_INFANTRYMAN;

			maxHP = 2;
			meleeRange = 7.;
			meleeAngle = 1./3.;
			MeleeTimer.set_max(70);
			//meleeCooldownTicks = 71;

			meleeAttack = 25;
			meleeDefense = 20;
			infantry = true;
			rangedDefense = 0;

			init(unit);
		};

		int damage(Soldier* target) {debug("Ahhhhh!");return 1;}
};

class Rider : public Soldier {
	public:
		Rider(Unit* unit) : Soldier() {
			rad = 7.;
			mass = 4.;
			defaultMaxSpeed = 40.;
			maxSpeed = defaultMaxSpeed;
			accel = 6.;
			turn = M_PI/3;
			onTargetDamp = 2.;	//dampening when close to posTarget
			type = SOLDIER_RIDER;

			maxHP = 5;
			meleeRange = 7.;
			meleeAngle = 1.;
			MeleeTimer.set_max(30);
			//meleeCooldownTicks = 31;

			meleeAttack = 35;
			meleeDefense = 25;
			large = true;
			rangedDefense = 40;

			init(unit);
		};

		int damage(Soldier* target) {
			debug("Hyahh!");
			int dmg = 2;
			double speed = vel.norm();
			if(speed > maxSpeed/5.) dmg++;
			if(speed > 2*maxSpeed/5.) dmg++;
			if(speed > 4*maxSpeed/5.) dmg++;
			return dmg;
		}
};

class Monster : public Soldier {
	public:
		Monster(Unit* unit) : Soldier() {
			rad = 15.;
			mass = 20.;
			defaultMaxSpeed = 40.;
			maxSpeed = defaultMaxSpeed;
			accel = 2.8;
			turn = M_PI/4;
			onTargetDamp = 3.2;	//dampening when close to posTarget
			type = SOLDIER_MONSTER;

			maxHP = 50;
			meleeRange = 15.;
			meleeAngle = 1./2.;
			MeleeTimer.set_max(70);
			//meleeCooldownTicks = 71;

			meleeAttack = 55;
			meleeDefense = 40;
			rangedDefense = 40;
			large = true;
			antiInfantry = true;
			meleeAOE = true;

			init(unit);
		};

		int damage(Soldier* target) {debug("RARR!"); return 4;}
};

class Shoota : public Soldier {
	public:		
		Shoota(Unit* unit) : Soldier() {
			rad = 5.;
			mass = 1.;
			defaultMaxSpeed = 15.;
			maxSpeed = defaultMaxSpeed;
			accel = 3.;
			turn = M_PI/1.5;
			onTargetDamp = 3.;	//dampening when close to posTarget
			type = SOLDIER_SHOOTA;

			maxHP = 2;
			meleeRange = 7.;
			meleeAngle = 1./3.;
			MeleeTimer.set_max(70);
			//meleeCooldownTicks = 61;

			meleeAttack = 15;
			meleeDefense = -10;
			armor = 0;
			armorPiercing = 0;
			infantry = true;
			large = false;
			antiInfantry = false;
			antiLarge = false;

			ranged = true;
			rangedRange = 400.;
			rangedMinRange = 7.;
			rangedRad = 0.1;//M_PI / 6;
			DrawTimer = Timer(10);
			ReloadTimer = Timer(300);
			// drawCDTicks = 0;
			//int reloadCDTicks = 0;
			rangedDamage = 2;
			rangedSpeed = 120.;
			maxSpeedForFiring = 10.;
			projectileType = PROJECTILE_ARROW;
			rangedMinRange = 60;
			rangedAOE = 10;

			init(unit);
		};
		
		int damage(Soldier* target) {return 1;}

};

class SlowShoota : public Soldier {
	public:		
		SlowShoota(Unit* unit) : Soldier() {
			rad = 5.;
			mass = 1.;
			defaultMaxSpeed = 15.;
			maxSpeed = defaultMaxSpeed;
			accel = 3.;
			turn = M_PI/60.;
			onTargetDamp = 3.;	//dampening when close to posTarget
			type = SOLDIER_SLOW_SHOOTA;

			maxHP = 2;
			meleeRange = 7.;
			meleeAngle = 1./3.;
			MeleeTimer.set_max(70);
			//meleeCooldownTicks = 61;

			meleeAttack = 15;
			meleeDefense = -10;
			armor = 0;
			armorPiercing = 0;
			infantry = true;
			large = false;
			antiInfantry = false;
			antiLarge = false;

			ranged = true;
			rangedRange = 700.;
			rangedMinRange = 7.;
			rangedRad = 0.1;//M_PI / 6;
			rangedHeavy = true;
			DrawTimer = Timer(10);
			ReloadTimer = Timer(600);
			// drawCDTicks = 0;
			//int reloadCDTicks = 0;
			rangedDamage = 0;
			rangedSpeed = 120.;
			maxSpeedForFiring = 10.;
			projectileType = PROJECTILE_GRENADE;
			rangedMinRange = 60;
			rangedAOE = 40.; // determines when not shooting close to allies

			init(unit);
		};
		
		int damage(Soldier* target) {return 1;}

};



#endif