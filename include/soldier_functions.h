#ifndef SOLDIER_FUNCTIONS
#define SOLDIER_FUNCTIONS

#include <units.h>
#include <pathfinding.h>
#include <model.h>

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
	else if(CanReload()) {
		ReloadTimer.decrement();
	}
}

bool Soldier::CanReload() {
	return speed < maxSpeedForFiring && (MeleeTimer.done() || !melee);
}

void Soldier::IndivPathProgression(Map* map, double* time1, double* time2, double* timePass1) {
	indivPathTimer.decrement();
	auto start = std::chrono::system_clock::now();
	auto end = std::chrono::system_clock::now();
	bool condition;
	if(indivPathTimer.done()) {
		Circle c1(pos, rad);
		Circle c2(NoIPFPosTarget(this), rad);
		if(indivPath.empty()) {
			start = std::chrono::system_clock::now();
			condition = FreePath(&c1, &c2, map, false); // set the last one to true
			end = std::chrono::system_clock::now();
			if(time1)
				*time1 += std::chrono::duration<double>(end - start).count();
			if(!condition) {
				//do indiv pathfinding
				start = std::chrono::system_clock::now();
				//std::cout << "Soldier::IndivPathProgression findPath 1 start...\n";
				indivPath = findPath(&c2, &c1, map, timePass1);
				//std::cout << "Soldier::IndivPathProgression findPath 1 done.\n";
				end = std::chrono::system_clock::now();
				if(time2)
					*time2 += std::chrono::duration<double>(end - start).count();
			}
		}
		else {
			if(FreePath(&c1, &c2, map)) {
				indivPath.clear();
			}
			else {
				Circle c3(indivPath.at(0), rad);
				start = std::chrono::system_clock::now();
				condition = FreePath(&c1, &c3, map);
				end = std::chrono::system_clock::now();
				if(time1)
					*time1 += std::chrono::duration<double>(end - start).count();
				if(condition) {
					if(indivPath.size() > 1) {
						Circle c4(indivPath.at(1), rad);
						start = std::chrono::system_clock::now();
						condition = FreePath(&c1, &c4, map);
						end = std::chrono::system_clock::now();
						if(time1)
							*time1 += std::chrono::duration<double>(end - start).count();
						if(condition)
							std::erase(indivPath, indivPath.at(0));
					}
					else {
						if((c3.pos - c1.pos).norm() < rad)
							std::erase(indivPath, indivPath.at(0));
					}
				}
				else {
					//redo indiv pathfinding
					start = std::chrono::system_clock::now();
					std::cout << "Soldier::IndivPathProgression findPath 2 start...\n";
					indivPath = findPath(&c2, &c1, map, timePass1);
					std::cout << "Soldier::IndivPathProgression findPath 2 done.\n";
					end = std::chrono::system_clock::now();
					if(time2)
						*time2 += std::chrono::duration<double>(end - start).count();
				}
			}
		}
		indivPathTimer.reset();
	}
}

#endif