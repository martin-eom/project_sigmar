#ifndef ANIMATION
#define ANIMATION

#include <soldiers.h>

enum AnimationTypes {
	ANIMATION_GENERIC,
	ANIMATION_ATTACK,
	ANIMATION_DAMAGE
};

class SoldierAnimation {
public:
	virtual void ConversionEnabler() {}

	Soldier* soldier;
	int stage;
	int ticksToNextStage;
	bool running;
	bool onCooldown;
	int type;

	SoldierAnimation(Soldier* soldier) {
		this->soldier = soldier;
		stage = 0;
		ticksToNextStage = 0;
		running = false;
		onCooldown = false;
		type = ANIMATION_GENERIC;
	}
};

class AttackAnimation : public SoldierAnimation {
public:
	AttackAnimation(Soldier* soldier) : SoldierAnimation(soldier) {
		type = ANIMATION_ATTACK;
	}
};

class DamageAnimation : public SoldierAnimation {
public:
	int lastHP;

	DamageAnimation(Soldier* soldier) : SoldierAnimation(soldier) {
		lastHP = soldier->hp;
		type = ANIMATION_DAMAGE;
	}
};

#endif