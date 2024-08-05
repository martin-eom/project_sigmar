#ifndef PROJECTILE
#define PROJECTILE

#include <events.h>
#include <extra_math.h>
#include <timer.h>
#include <Dense>

enum PROJECTILE_TYPES {
	PROJECTILE_GENERIC,
	PROJECTILE_DUMMY_ARROW,
	PROJECTILE_ARROW,
	PROJECTILE_GRENADE
};


class Soldier;


class Projectile : public Point{
	//Eigen::Vector2d pos;
	Eigen::Vector2d vel;
	double percentage;	// can provide details when a projectile can fly over obstacles and when not, etc.
	Timer life;
	double dt;

public:
	double angle;
	std::string soldierType;
	bool dead;
	bool longDead;
	std::vector<Soldier*> targets;
	//virtual int damage(Soldier* target) {return 100;}
	double damage;
	double armorPiercing;
	double aoerad = 0.;

	void advance() {
		pos += vel*dt;
		life.decrement();
		percentage = (life.get_max() - life.get_current()) / static_cast<double>(life.get_max());
	}

	Eigen::Vector2d get_pos() {
		return pos;
	}

	Eigen::Vector2d get_vel() {
		return vel;
	}

	double get_progress() {
		return percentage;
	}

	Projectile(std::string soldierType, Eigen::Vector2d start, Eigen::Vector2d vel, int lifetime, double dt, double damage, int armorPiercing, double aoerad) : Point(start) {
		this->soldierType = soldierType;
		//pos = start;
		this->vel = vel;
		Eigen::Vector2d normVel = vel / vel.norm();
		angle = Angle(-normVel.coeff(1), normVel.coeff(0));
		percentage = 0.;
		life = Timer(lifetime);
		this->dt = dt;
		dead = false;
		longDead = false;
		this->damage = damage;
		this->armorPiercing = armorPiercing;
		this->aoerad = aoerad;
	}
};


ProjectileSpawnEvent SpawnProjectile(std::string soldierType, Eigen::Vector2d pos, Eigen::Vector2d vel, int lifetime, double dt, int damage, int armorPiercing, double aoerad) {
	Projectile* p;
	/*switch(type) {
	case PROJECTILE_ARROW:
		p = new Arrow(pos, vel, lifetime, dt);
		break;
	case PROJECTILE_DUMMY_ARROW:
		p = new DummyArrow(pos, vel, lifetime, dt);
		break;
	case PROJECTILE_GRENADE:
		p = new Grenade(pos, vel, lifetime, dt);
		break;
	default:
		p = new Arrow(pos, vel, lifetime, dt);
	}*/
	p = new Projectile(soldierType, pos, vel, lifetime, dt, damage, armorPiercing, aoerad);
	return ProjectileSpawnEvent(p);
}

#endif