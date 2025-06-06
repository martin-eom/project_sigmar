#ifndef COMBAT_MECHANICS
#define COMBAT_MECHANICS

#include <units.h>
#include <pathfinding.h>
#include <model.h>

bool Unit::UseOrderTarget(Map* map) {
	Order* current = orders.at(currentOrder);
	if(current->type == ORDER_TARGET && current->target->nLiveSoldiers > 0 && (current->target->pos - pos).norm() < range) {
		Circle c1 = Circle(pos, OnSpotUnitRectangle(this).hw*0.7);
		Circle c2 = Circle(current->target->pos, OnSpotUnitRectangle(current->target).hw*0.7);
		if(FreePath(&c1, &c2, map, true)) {
			rangedTarget = current->target;
			return true;
		}
	}
	return false;
}

void Unit::FindRangedTarget(std::vector<Player*> players) {
	std::vector<UnitDistance> inRange;
	Eigen::Matrix2d rangedCone;
	rangedCone << std::cos(0.5*M_PI*rangedAngle), -std::sin(0.5*M_PI*rangedAngle), 
		std::sin(0.5*M_PI*rangedAngle), std::cos(0.5*M_PI*rangedAngle);
	for(auto player2 : players) {
		if(player2 != player) {
			for(auto unit2 : player2->units) {
				if(unit2->placed) {
					Circle circ(unit2->pos, 0);
					if(ConeCircleCollision(pos, rot, rangedCone, range, &circ)
						&& (pos - unit2->pos).norm() < range) {
						inRange.push_back(UnitDistance(this, unit2));
					}
				}
			}
		}
	}
	std::sort(inRange.begin(), inRange.end(), compareUnitDistance);
	if(!inRange.empty()) {
		rangedTarget = inRange.at(0).unit;
	}
	else {
		rangedTarget = NULL;
	}
}

void Soldier::ChooseMeleeTargetsByRangeAndCone(std::vector<SoldierNeighbourContainer>* targets, std::vector<SoldierNeighbourContainer>* notInCone) {
	bool newTarget = false;
	while(!enemiesInMeleeRange.empty()) {
		SoldierNeighbourContainer enemy = enemiesInMeleeRange.top();
		Circle circ = *(enemy.soldier); //enemy.soldier->SoldierCircle();
		if((targets->empty() || meleeAOE) && ConeCircleCollision(pos, rot, meleeCone, rad, &circ) && enemy.inTrueRange) { //closest enemy that is in the cone
			meleeTarget = enemy.soldier; // in/excluding this line could significantly change how flanking works
			targets->push_back(enemy);
			if(!meleeAOE)
				meleeSwingTarget = enemy.soldier;
			if(!newTarget || (meleeAOE && !meleeTarget && !meleeTarget->alive)) {		// what to do here
				meleeTarget = enemy.soldier;
				newTarget = true;
			}
			//debug("Set target!");
		}
		else
			notInCone->push_back(enemy);
		enemiesInMeleeRange.pop();
	}
	if(!newTarget && !notInCone->empty()) {
		meleeTarget = notInCone->at(0).soldier;
		newTarget = true;
		notInCone->clear();
	}
}

void Soldier::HandleCharging(std::vector<SoldierNeighbourContainer>* targets) {
	//handling "charging" status
	//	while charging soldiers will push into the enemy position
	//  if they have no target in front of them for 1 second they will stop charging and seek out enemies close to them
	Order* o = unit->orders.at(currentOrder);
	if(o->target) {
		if(charging) {
			UpdateChargingStatus(targets, o);
		}
		else {
			FindTargetIfNoneInRange(o);
		}
	}
}

void Soldier::UpdateChargingStatus(std::vector<SoldierNeighbourContainer>* targets, Order* o) {
	if(unit->enemyContact) {
		if(!targets->empty() && (!meleeAOE && unit->maxSoldiers == 1) && o->type == ORDER_ATTACK)	{//lone monsters stop charging after impact
			chargeTimer.reset();
		}
		else {
			/*if(!chargeTimer.done())
				chargeTimer.decrement();
			else
				charging = false;*/
			if(chargeTimer.decrement())
				charging = false;
		}
	}
	else {
		if((!targets->empty() && targets->at(0).soldier->unit == o->target) || ((unit->pos - unit->posTarget).norm() < 20 && o->type == ORDER_ATTACK)) {
			unit->enemyContact = true;
		}
	}
}

void Soldier::FindTargetIfNoneInRange(Order* o) {
	if((!meleeTarget || !meleeTarget->alive) && !o->target->liveSoldiers.empty()) {
		Soldier* target = NULL;
		// for monsters
		if(meleeAOE && unit->maxSoldiers == 1) {
			double maxDist = 0;
			for(int i = 0; i < 8; i++) {
				Soldier* current = o->target->liveSoldiers.at(rand()%o->target->liveSoldiers.size());
				double dist = (current->pos - pos).norm();
				if(dist > maxDist) {
					target = current;
					maxDist = dist;
				}
			}
		}
		// for non-monsters
		else {
			double minDist = std::numeric_limits<double>::infinity();
			for(int i = 0; i < 10; i++) {
				Soldier* current = o->target->liveSoldiers.at(rand()%o->target->liveSoldiers.size());
				double dist = (current->pos - pos).norm();
				if(dist < minDist) {
					target = current;
					minDist = dist;
				}
			}
		}
		meleeTarget = target;
	}
}

void Soldier::CheckIfPathToTarget(Map* map) {
	if(unit->orders.at(currentOrder)->target
	&& meleeTarget && !charging) {//soldier->unit->enemyContact) {
		if(noTargetTimer.decrement()) {
			Circle c1(pos, rad);
			Circle c2(meleeTarget->pos, rad);
			if(!FreePath(&c1, &c2, map)) {
				meleeTarget = NULL;
			}
			noTargetTimer.reset();
		}
	}
}

void Soldier::ResolveAttacks(Model* model, std::vector<SoldierNeighbourContainer>* targets) {
	if(MeleeTimer.decrement() && !targets->empty()) {
		for(auto targetContainer : *targets) {
			Soldier* target = targetContainer.soldier;
			double hitChance = model->settings.base_melee_attack + 0.01*(meleeAttack - target->meleeDefense) 
				+ model->settings.anti_infantry_attack_bonus*(antiInfantry && target->infantry) 
				+ model->settings.anti_large_attack_bonus*(antiLarge && target->large);
			hitChance = std::min(std::max(model->settings.min_hit_chance, hitChance), model->settings.max_hit_chance);
			if(std::rand()/double(RAND_MAX) < hitChance) {
				double dmg = meleeDamage;
				// melee damage function
				dmg = dmg * (1 + 0.3*(antiInfantry && target->infantry) + 0.3*(antiLarge && target->large));
				dmg = dmg * (0.01 * armorPiercing + (1 - 0.01*armorPiercing) * (1 - 0.01*target->armor));
				model->damages.push(DamageTick(target, dmg));
			}
		}
		MeleeTimer.reset();
	}
}

bool Soldier::OnAttackOrder() {
	return unit->orders.at(currentOrder)->target;
}

void Soldier::GetValidTargetRangedTarget() {
	// drop taget if lagging behind
	if(currentOrder < unit->currentOrder) {
		rangedTarget = NULL;
	}
	// drop target if target in wrong unit
	else if(rangedTarget && (rangedTarget->unit != unit->rangedTarget)) {
		rangedTarget = NULL;
	}
	// pick frontal target in correct unit
	else if(!rangedTarget) {
		if(unit->rangedTarget->nLiveSoldiers > 0) {
			Soldier* target = unit->rangedTarget->liveSoldiers.at(rand()%unit->rangedTarget->liveSoldiers.size());
			if(target->currentOrder == unit->rangedTarget->currentOrder) {
				rangedTarget = target;
			}
		}
	}
	// drop dead target
	if(rangedTarget && !rangedTarget->alive) {
		rangedTarget = NULL;
	}
}

bool Soldier::TargetInRangedCone() {
	Eigen::Vector2d dist = rangedTarget->pos - pos;
	return (rot.transpose() * dist).x() / dist.norm() > 0.7;
}

bool Soldier::HasLOSToPointOfImpact(Eigen::Vector2d pointOfImpact, Map* map) {
	//Eigen::Vector2d pointOfImpact = pos + vel * t;
	Circle c1 = Circle(pos, rad);
	Circle c2 = Circle(pointOfImpact, rad);//soldier->rad);
	return FreePath(&c1, &c2, map, true);
}

bool Soldier::AllyTooCloseToTarget(Eigen::Vector2d pointOfImpact, double flightTime) {
	if(rangedTarget->meleeTarget && !rangedTarget->meleeTarget->large) {
		Soldier* mtarget = rangedTarget->meleeTarget;
		Eigen::Vector2d allyPos = mtarget->pos + mtarget->vel * flightTime;
		double rmin = tans * (pos - pointOfImpact).norm() + rangedAOE;
		if((pointOfImpact - allyPos).norm() - mtarget->rad < rmin)
			return true;
	}
	return false;
}


void Soldier::FireOrReloadIfPossible(Map* map, EventManager* em) {
	bool swinging = !MeleeTimer.done() && melee;
	if(rangedTarget
		&& currentOrder == unit->currentOrder
		&& vel.norm() < maxSpeedForFiring
		&& (!meleeTarget || (rangedTarget->pos - pos).norm() > rangedMinRange)) {
		bool canFire = TargetInRangedCone();
		if(canFire && ReloadTimer.done()) {
			double flightTime = projectile_flight_time(rangedTarget->pos - pos,
				rangedTarget->vel, rangedSpeed);
			if(flightTime > 0 && projectileSpeed * flightTime < rangedRange) {
				Displacement dis = ShotAngle(rangedTarget->pos - pos,
					rangedTarget->vel, rangedSpeed, flightTime, tans);
				Eigen::Vector2d vel;
				vel << 1., 0.;
				vel = dis.rot * vel * rangedSpeed;
				Eigen::Vector2d pointOfImpact = pos + vel * flightTime;
				canFire = HasLOSToPointOfImpact(pointOfImpact, map);
				if(canFire)
					canFire = !AllyTooCloseToTarget(pointOfImpact, flightTime);
				if(swinging)
					canFire = false;
				// create projectile spawn event
				if(canFire) {
					ProjectileSpawnEvent pev = SpawnProjectile(tag, pos, vel, static_cast<int>(flightTime/em->dt), em->dt, 
						rangedDamage, rangedArmorPiercing, projectileAOE, projectileTilesize);
					em->Post(&pev);
					ReloadTimer.reset();
				}
				else
					rangedTarget = NULL;
			}
			else
				rangedTarget = NULL;
		}
		else if(!swinging){
			ReloadTimer.decrement();
		}
	}
	else if(vel.norm() < maxSpeedForFiring && !swinging) {
		ReloadTimer.decrement();
	}
}

#endif