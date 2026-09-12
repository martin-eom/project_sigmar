#ifndef MODEL_FUNCTIONS
#define MODEL_FUNCTIONS

#include <model.h>
#include <simple_ai.h>

void Model::CreateSimpleAI(Player* player) {
	simpleAIs.push_back(new SimpleAI(player, this, this->em));
}

Model::~Model() {
	// Projectiles are owned by model. ProjectileAnimations
	// are owned (and freed) by ~View.
	for(auto projectile : projectiles) delete projectile;
	projectiles.clear();

	for(auto ai : simpleAIs) delete ai;
	simpleAIs.clear();

	for(auto player : players) delete player;
	players.clear();
	units.clear();
	soldiers.clear();
	player1 = NULL; player2 = NULL; currentPlayer = NULL;

	for(auto lock : unit_locks) {
		omp_destroy_lock(lock);
		delete lock;
	}
	unit_locks.clear();
	for(auto lock : soldier_locks) {
		omp_destroy_lock(lock);
		delete lock;
	}
	soldier_locks.clear();

	// map belongs to server.cpp.
}

void Model::GameStateCheck() {
	switch(state) {
	case MODEL_GAME_READY_TO_START: {
		state = MODEL_GAME_PAUSED;
		GamePausedEvent gpe;
		em->Post(&gpe);
		break;
	}
	case MODEL_GAME_RUNNING:
		if(toNextState.done()) {
			int sumLife1 = 0; 
			int sumLife2 = 0;
			for(auto unit : player1->units) sumLife1 += unit->nLiveSoldiers;
			for(auto unit : player2->units) sumLife2 += unit->nLiveSoldiers;
			if(sumLife1 == 0 || sumLife2 == 0) {
				state = MODEL_GAME_OVER;
				if(sumLife1 == 0) {
					if(sumLife2 == 0) result = "Game Over: Draw";
					else result = "Game Over: Player 2 wins";
				}
				else result = "Game Over: Player 1 wins";
			}
			else {
				state = MODEL_GAME_PAUSED;
				if(!currentPlayer)
					currentPlayer = player1;
				else if(currentPlayer == player1)
					currentPlayer = player2;
				else
					currentPlayer = player1;
				GamePausedEvent gpe;
				em->Post(&gpe);
				toNextState.reset();
			}
		}
		else {
			toNextState.decrement();
		}
		break;
	}
}

void Model::PlaceUnits() {
	for(auto player : players) {
		for(auto unit : player->units) {
			if(!unit->placed) {
				if(!unit->orders.empty()) {
					Order* o = unit->orders.at(0).get();
					if(o->type == ORDER_MOVE) {	// this is only triggered in simulation mode, if you try to place a unit with an attack order
						UnitPlaceRequest pev(unit, o->pos, o->rot);
						em->Post(&pev);
					}
				}
			}
		}
	}
}

void Model::MapSoldiersToGrid() {
	for(auto unit : units) {
		if(unit->placed) {
			CollisionScrying(map, unit);
		}
	}
}

void Model::MeleeCombat() {
	for(auto unit : units) {
		if(unit->placed) {
			for(auto row : unit->soldiers) {
				for(auto soldier : row) {
					if(soldier->alive) {
						std::vector<SoldierNeighbourContainer> targets;
						std::vector<SoldierNeighbourContainer> notInCone;
						soldier->meleeSwingTarget = NULL;
						if(soldier->melee) {
							soldier->ChooseMeleeTargetsByRangeAndCone(&targets, &notInCone);
							soldier->HandleCharging(&targets);
							soldier->CheckIfPathToTarget(map);
							soldier->ResolveAttacks(this, &targets);
						}
					}
				}
			}
		}
	}
}

void Model::RangedTargetFinding() {
	for(auto unit: units) {
		if(unit->placed && unit->ranged && unit->rangedTargetUpdateTimer.decrement()) {
			unit->rangedTarget = NULL;
			if(!unit->UseOrderTarget(map)) {
				unit->FindRangedTarget(players);
			}
			unit->rangedTargetUpdateTimer.reset();
		}
	}
}

void Model::Shooting() {
	for(auto unit : units) {
		if(unit->ranged) {
			for(auto soldier : unit->liveSoldiers) {
				if(unit->rangedTarget) {
					soldier->GetValidTargetRangedTarget();
					soldier->FireOrReloadIfPossible(map, em);
				}
				else
					soldier->rangedTarget = NULL;
					//if(!soldier->ReloadTimer.done() && soldier->speed < soldier->maxSpeedForFiring
					//	&& (soldier->MeleeTimer.done() || !soldier->melee))
					if(soldier->CanReload())
						soldier->ReloadTimer.decrement();
			}
		}
	}
}

void Model::ProjectileHitResolution() {
	for(auto projectile : projectiles) {
		if(projectile->dead && !projectile->longDead) {
			for(auto soldier : projectile->targets) {
				double hitChance = settings.ranged_base_attack - 0.01*soldier->rangedDefense;
				hitChance = std::min(std::max(settings.min_hit_chance, hitChance), settings.max_hit_chance);
				if(std::rand()/double(RAND_MAX) < hitChance) {
					double dmg = projectile->damage;
					dmg = dmg * (0.01*projectile->armorPiercing + (1 - 0.01*projectile->armorPiercing) * (1 - 0.01*soldier->armor));
					damages.push(DamageTick(soldier, dmg));
				}
			}
			projectile->targets.clear();
		}
	}
}

void Model::ProjectileCleanup() {
	for(auto it = projectiles.begin(); it != projectiles.end(); ) {
		Projectile* projectile = *it;
		if(projectile->longDead) {
			it = projectiles.erase(it);
			delete projectile;
		}
		else {
			if(projectile->dead)
				projectile->longDead = true;
			else
				projectile->advance();
			it = std::next(it);
		}
	}
}

void Model::DamageResolution() {
	while(!damages.empty()) {
		DamageTick d = damages.front();
		d.soldier->hp -= d.dmg;
		if(d.soldier->hp <= 0) {
			KillEvent e(d.soldier);
			em->Post(&e);
		}
		damages.pop();
	}
}

#endif