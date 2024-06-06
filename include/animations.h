#ifndef OLDANIMATION
#define OLDANIMATION

//#include <soldiers.h>
#include <soldiers2.h>

enum AnimationTypes {
	ANIMATION_GENERIC,
	ANIMATION_ATTACK,
	ANIMATION_DAMAGE
};

class OldSoldierAnimation {
public:
	virtual void ConversionEnabler() {}

	Soldier* soldier;
	int stage;
	int ticksToNextStage;
	bool running;
	bool onCooldown;
	int type;

	OldSoldierAnimation(Soldier* soldier) {
		this->soldier = soldier;
		stage = 0;
		ticksToNextStage = 0;
		running = false;
		onCooldown = false;
		type = ANIMATION_GENERIC;
	}
};

class OldAttackAnimation : public OldSoldierAnimation {
public:
	OldAttackAnimation(Soldier* soldier) : OldSoldierAnimation(soldier) {
		type = ANIMATION_ATTACK;
	}
};

class DamageAnimation : public OldSoldierAnimation {
public:
	int lastHP;

	DamageAnimation(Soldier* soldier) : OldSoldierAnimation(soldier) {
		lastHP = soldier->hp;
		type = ANIMATION_DAMAGE;
	}
};

#endif