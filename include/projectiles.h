#ifndef PROJECTILE
#define PROJECTILE

#include <events.h>
#include <timer.h>
#include <Dense>

enum PROJECTILE_TYPES {
	PROJECTILE_GENERIC,
	PROJECTILE_DUMMY_ARROW,
	PROJECTILE_ARROW,
	PROJECTILE_GRENADE
};

class Soldier;

class Projectile{
	Eigen::Vector2d pos;
	Eigen::Vector2d vel;
	double percentage;	// can provide details when a projectile can fly over obstacles and when not, etc.
	Timer life;
	double dt;

public:
	int type = PROJECTILE_GENERIC;
	bool dead;
	std::vector<Soldier*> targets;
	virtual int damage(Soldier* target) {return 100;}
	double aoerad = 0.;

	void advance() {
		pos += vel*dt;
		life.decrement();
		percentage = (life.get_max() - life.get_current()) / life.get_max();
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

	Projectile(Eigen::Vector2d start, Eigen::Vector2d vel, int lifetime, double dt) {
		pos = start;
		this->vel = vel;
		percentage = 0.;
		life = Timer(lifetime);
		this->dt = dt;
		dead = false;
	}
};

class DummyArrow : public Projectile {
public:
	int damage(Soldier* soldier) {return 0;}

	DummyArrow(Eigen::Vector2d start, Eigen::Vector2d vel, int lifetime, double dt) : Projectile(start, vel, lifetime, dt) {
		type = PROJECTILE_DUMMY_ARROW;
	}
};

class Grenade : public Projectile {
public:
	int damage(Soldier* soldier) {return 5;}

	Grenade(Eigen::Vector2d start, Eigen::Vector2d vel, int lifetime, double dt) : Projectile(start, vel, lifetime, dt) {
		type = PROJECTILE_GRENADE;
		aoerad = 15.;
	}
};

class Arrow : public Projectile {
public:
	int damage(Soldier* soldier) {return 2;}

	Arrow(Eigen::Vector2d start, Eigen::Vector2d vel, int lifetime, double dt) : Projectile(start, vel, lifetime, dt){
		type = PROJECTILE_ARROW;
	}
};


ProjectileSpawnEvent SpawnProjectile(int type, Eigen::Vector2d pos, Eigen::Vector2d vel, int lifetime, double dt) {
	Projectile* p;
	switch(type) {
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
	}
	return ProjectileSpawnEvent(p);
}

#endif