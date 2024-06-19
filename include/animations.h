#ifndef ANIMATION
#define ANIMATION

#include <extra_math.h>
#include <information.h>
#include <soldiers.h>
#include <projectiles.h>
#include <textures.h>
#include <timer.h>


class Animation {
public:
	Point* object;
	ImgTexture* texture;
	AnimationInformation info;
	int stage = 0;
	Timer toNextStage = Timer(5);

	Animation(Point* object) {
		this->object = object;
	}

	Animation(Point* object, AnimationInformation info) {
		this->object = object;
		this->info = info;
		toNextStage.set_max(info.ticks_per_frame);
	}

	void advance() {
		if(toNextStage.done()) {
			stage = (stage + 1) % info.num_frames;
			toNextStage.reset();
		}
		else
			toNextStage.decrement();
	}
};

class SoldierAnimation : public Animation {
public:
	Soldier* soldier;

	SoldierAnimation(Point* object) : Animation(object) {
		soldier = dynamic_cast<Soldier*>(object);
	}

	SoldierAnimation(Point* object, AnimationInformation info) : Animation(object, info) {
		soldier = dynamic_cast<Soldier*>(object);
	}

	virtual void advance(){};
};

class LegAnimation : public SoldierAnimation {
public:
	bool upwards = true;
	int nextStage = 0;
	//Timer toNextStage = Timer(5);

	LegAnimation(Point* object, AnimationInformation info) : SoldierAnimation(object, info) {}

	void advance() {
		if(soldier->forwardSpeed < soldier->maxSpeed * 0.05)
			stage = 0;
		else {
			if(!toNextStage.done()) {
				toNextStage.decrement();
			}
			else {
				stage = nextStage;
				double step;
				if(info.step)
					step = info.step;
				else
					step = soldier->rad;
				double tNext;
				if(soldier->forwardSpeed < soldier->maxSpeed * 0.5) {
					switch(stage){
					case 0:
						if(upwards)
							nextStage = 1;
						else
							nextStage = 2;
						break;
					case 3:
						nextStage = 1;
						upwards = !upwards;
						break;
					case 4:
						nextStage = 2;
						upwards = !upwards;
						break;
					default:
						nextStage = 0;
						upwards = !upwards;
						break;
					}
				}
				else {
					switch(stage) {
					case 0:
						if(upwards)
							nextStage = 1;
						else
							nextStage = 2;
						break;
					case 1:
						if(upwards)
							nextStage = 3;
						else
							nextStage = 0;
						break;
					case 2:
						if(upwards)
							nextStage = 0;
						else
							nextStage = 4;
						break;
					case 3:
						nextStage = 1;
						upwards = !upwards;
						break;
					case 4:
						nextStage = 2;
						upwards = !upwards;
						break;
					}
				}
				tNext = step / (2 * soldier->forwardSpeed) *30;
				toNextStage.set_max(tNext);
			}
		}
	}
};

class MeleeAnimation : public SoldierAnimation {
public:
	//Timer toNextStage = Timer(6);
	bool running = false;
	bool onCooldown = true;

	MeleeAnimation(Point* object, AnimationInformation info) : SoldierAnimation(object, info) {
		toNextStage.set_max(info.ticks_per_frame);
	}

	void advance() {
		if(!running && !onCooldown && !soldier->MeleeTimer.done()) {
			running = true;
			onCooldown = true;
			toNextStage.set(-1);
		}
		else if(running) {
			if(toNextStage.done()) {
				if(stage == info.num_frames - 1) {
					stage = 0;
					running = false;
				}
				else {
					stage++;
					toNextStage.reset();
				}
			}
			else toNextStage.decrement();
		}
		else if(onCooldown && soldier->MeleeTimer.done()) {
			onCooldown = false;
		}
	}
};

class RangedAnimation : public SoldierAnimation {
public:

	RangedAnimation(Point* object, AnimationInformation info) : SoldierAnimation(object, info) {}

	void advance() {
		if(soldier->ReloadTimer.done())
			stage = info.num_frames - 1;
		else {
			double progress = (soldier->ReloadTimer.get_max() - soldier->ReloadTimer.get_current()) 
				/ static_cast<double>(soldier->ReloadTimer.get_max()) * info.num_frames;
			stage = std::min(static_cast<int>(progress), info.num_frames - 1);
		}
	}
};

class ProjectileAnimation : public Animation {
public:
	Projectile* projectile;
	double length;

	ProjectileAnimation(Point* object, AnimationInformation info) : Animation(object, info) {
		projectile = dynamic_cast<Projectile*>(object);
		length = info.length;
	}

	void advance() {
		double progress = projectile->get_progress() * static_cast<double>(info.num_frames);
		stage = std::min(static_cast<int>(progress), info.num_frames - 1);
	}
};

class DamageAnimation : public SoldierAnimation {
public:
	bool running = false;
	int last_hp;
	//Timer ticksToNextStage;

	DamageAnimation(Soldier* soldier, AnimationInformation info) : SoldierAnimation(soldier, info) {
		last_hp = soldier->hp;
		toNextStage.set_max(info.ticks_per_frame);
		toNextStage.set(-1);
	}

	void advance() {
		if(!running && soldier->hp < last_hp) {
			running = true;
			toNextStage.reset();
			last_hp = soldier->hp;
		}
		else if(running) {
			if(toNextStage.done()) {
				stage = (stage+1)%info.num_frames;
				toNextStage.reset();
				if(stage == 0)
					running = false;
			}
			else
				toNextStage.decrement();
		}
	}
};


#endif