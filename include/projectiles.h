#ifndef PROJECTILE
#define PROJECTILE

#include <events.h>
#include <timer.h>
#include <Dense>

enum PROJECTILE_TYPES {
	PROJECTILE_GENERIC,
	PROJECTILE_ARROW
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
	default:
		p = new Arrow(pos, vel, lifetime, dt);
	}
	return ProjectileSpawnEvent(p);
}

#endif