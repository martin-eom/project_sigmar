#ifndef SOLDIERS2
#define SOLDIERS2

#include <extra_math.h>
#include <timer.h>
#include <debug.h>
#include <information.h>
//#include <projectiles.h>

#ifndef _USE_MATH_DEFINES
#define _USE_MATH_DEFINES
#endif

#include <cmath>
#include <Dense>
#include <ostream>
#include <queue>
#include <cstdlib>


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


class Soldier : public Point{
	public:
		double rad;
		double mass;
		double defaultMaxSpeed;
		double maxSpeed;
		double accel;	//acceleration the soldier can use
		double turn;	//turnspeed
		double linearDamp;	//coefficient for linear dampening (with regard to velocity)
		double squareDamp;	//coefficient for square dampening
		double onTargetDamp;	//dampening when close to posTarget
		std::string tag;

		int maxHP;
		bool melee;
		double meleeRange = 0.;
		double meleeAngle = 1./3.;
		virtual int damage(Soldier* target) {return 1;}
		Timer MeleeTimer = Timer(60);
		//int meleeCooldownTicks = 61;
		int meleeDamage = 1;
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
		int rangedDamage = 0;
		double rangedSpeed = 0.;
		double maxSpeedForFiring = 0.;
		double projectileSpeed = 0.;
		int rangedArmorPiercing = 0;
		double projectileAOE = 0.;
		
		//general members
		double Force, damp, defaultDamp;
		bool placed = false;
		int currentOrder;
		bool arrived;
		Eigen::Vector2i gridpos;
		//Eigen::Vector2d pos; // handled by inheriting from Point
		Eigen::Vector2d posTarget;
		Eigen::Vector2d oldPosTarget;
		Eigen::Vector2d vel;
		double speed;
		double forwardSpeed;
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
		Timer noTargetTimer = Timer(60);
		Eigen::Matrix2d meleeCone;
		std::vector<Eigen::Vector2d> indivPath;	// individual pathfinding when los to original next target is lost
		Timer indivPathTimer = Timer(60);
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
			noTargetTimer.unset();
			indivPathTimer.set_max(int((2 + rand()/RAND_MAX)*30));
			int reloadShift = rand()%30 - 15;
			ReloadTimer.set_max(std::max(ReloadTimer.get_max() + reloadShift, 1));
			tans = sin(rangedRad) / cos(rangedRad);

			debugFlag1 = false;
			debugFlag2 = false;
			debugFlag3 = false;

			this->unit = unit;		
		}

		Soldier() {};

		Soldier(SoldierInformation info, Unit* unit) : Soldier(){
			tag = info.tag;
			rad = info.radius;
			mass = info.mass;
			defaultMaxSpeed = info.max_speed;
			maxSpeed = defaultMaxSpeed;
			accel = info.acceleration;
			turn = M_PI * info.turn_speed;
			onTargetDamp = info.on_target_dampening;
			maxHP = info.max_hp;
			armor = info.armor;

			melee = info.melee_melee;
			meleeRange = info.melee_range;
			meleeAngle = info.melee_angle;
			MeleeTimer.set_max(info.melee_cooldown);
			meleeAOE = info.melee_aoe;
			meleeAttack = info.melee_attack;
			meleeDefense = info.melee_defense;
			armorPiercing = info.melee_armor_piercing;
			meleeDamage = info.melee_damage;

			ranged = info.ranged_ranged;
			rangedRange = info.ranged_range;
			rangedMinRange = info.ranged_min_range;
			rangedRad = info.ranged_radius;
			rangedHeavy = info.ranged_heavy;
			DrawTimer.set_max(info.ranged_draw_timer);
			ReloadTimer.set_max(info.ranged_reload_timer);
			maxSpeedForFiring = info.ranged_max_speed_for_firing;
			rangedDefense = info.ranged_defense;
			projectileSpeed = info.ranged_projectile_speed;
			rangedSpeed = info.ranged_projectile_speed;
			rangedArmorPiercing = info.ranged_armor_piercing;
			rangedAOE = info.ranged_ally_protection_aoe;
			projectileAOE = info.ranged_aoe;
			rangedDamage = info.ranged_damage;

			infantry = info.kw_infantry;
			large = info.kw_large;
			antiInfantry = info.kw_anti_infantry;
			antiLarge = info.kw_anti_large;
			
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



#endif