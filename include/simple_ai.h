#ifndef SIMPLE_AI
#define SIMPLE_AI

#include <random>
#include <algorithm>

#include <base.h>
#include <model.h>
#include <order_rules.h>
#include <rng.h>
#include <extra_math.h>


struct TargetContainer {
	Unit* target;
	double distance;

	TargetContainer(Unit* target, double distance) {
		this->target = target;
		this->distance = distance;
	}

	bool operator < (const TargetContainer& cont) const {
		return (distance < cont.distance);
	}
};


class SimpleAI : public Listener{
public:
	Player* player;
	Model* model;
	bool gameStarted = false;
	std::vector<std::vector<Order*>> orderList;
	std::vector<Unit*> targets;

	SimpleAI(Player* player, Model* model, EventManager* em) : Listener(em) {
		this->player = player;
		this->model = model;
		/*for(auto unit : player->units) {
			targets.push_back(NULL);
		}*/
	}
	
	void InitTargets() {
		// needs to be called after units have been assigned to player
		for(auto unit : player->units) {
			targets.push_back(NULL);
		}
		//std::cout << "########## no. uints: " << targets.size() << "###################\n";
	}

	void GiveOrders();
	Player* Opponent();
	
	void GivePlaceOrder(Unit* unit, std::vector<Order*>* orders);
	void ValidateOldTarget(int unitNum);
	void FindNewTarget(Unit* unit, int unitNum);
	void GiveOrderToTarget(Unit* unit, int unitNum, std::vector<Order*>* orders);

private:
	void Notify(Event* ev) {
		switch(ev->type) {
		case GAME_PAUSED_EVENT:
			std::cout << "The event is caght, but...\n";
			if(model->currentPlayer == player) {
				std::cout << "SimpleAI is selected.\n";
				GiveOrders();
			}
			break;
		}
	}
};


Player* SimpleAI::Opponent() {
	if(player == model->player1)
		return model->player2;
	else
		return model->player1;
}

void SimpleAI::GiveOrders() {
	Player* opponent = Opponent();
	orderList.clear();
	for(auto unit : player->units) {
		int unitNum = std::find(player->units.begin(), player->units.end(), unit) - player->units.begin();
		std::vector<Order*> orders;
		// place unit
		if(!unit->placed) {
			GivePlaceOrder(unit, &orders);
			/*Eigen::Vector2d pos;
			bool validOrder = false;
			while(!validOrder) {
				double x = RNG::uniformDouble(0, model->map->width);
				double y = RNG::uniformDouble(0, model->map->height);
				pos << x, y;
				if(orderInDeploymentZone(pos, model, unit)) {
					validOrder = true;
					break;
				}
			}
			Eigen::Matrix2d rot;
			MoveOrder* mo = new MoveOrder(pos, Eigen::Matrix2d::Identity(), MOVE_FORMUP);
			orders.push_back(mo);*/
		}
		if(player == model->player2 || gameStarted) {
			// check if target is still valid
			ValidateOldTarget(unitNum);
			/*if(targets.at(unitNum)) {
				if(targets.at(unitNum)->nLiveSoldiers <= 0)
					targets.at(unitNum = NULL);
			}*/
			//find new target
			if(!targets.at(unitNum)) {
				FindNewTarget(unit, unitNum);
				/*Unit* possibleTarget = opponent->units.at(RNG::uniformInt(0, opponent->units.size() - 1));
				if(possibleTarget->nLiveSoldiers > 0)
					targets.at(unitNum) = possibleTarget;
				else {
					std::vector<TargetContainer> possibleTargets;
					for(auto enemyUnit : opponent->units) {
						if(unit->nLiveSoldiers > 0)
							possibleTargets.push_back(TargetContainer(enemyUnit, (enemyUnit->pos - unit->pos).norm()));
					}
					std::sort(possibleTargets.begin(), possibleTargets.end());
					if(possibleTargets.size() > 0)
						targets.at(unitNum) = possibleTargets.at(0).target;
				}*/
			}
			// give orders to get target
			if(targets.at(unitNum)) {
				Unit* target = targets.at(unitNum);
				if(!unit->ranged) {
					if(unit->orders.empty() || unit->orders.at(unit->currentOrder)->type != ORDER_ATTACK) {
						//int unit_index = RNG::uniformInt(0, opponent->units.size() - 1);
						//target = opponent->units.at(unit_index);
						orders.push_back(new AttackOrder(target, target->pos));
					}
				}
				else {
					// if no ranged target, but some target in range and los: move order to stop and shoot them
					if(!unit->rangedTarget) {
						//Unit* targetInRange = NULL;
						/*for(auto target : model->player1->units) {
							double unitRad = std::max((unit->ncols - 1)*unit->xspacing*0.5 + unit->soldiers.at(0).at(0)->rad, (unit->nrows - 1)*unit->xspacing*0.5 + unit->soldiers.at(0).at(0)->rad);
							Circle cUnit(unit->pos, unitRad);
							Circle cTarget(target->pos, unitRad);
							if((target->pos - unit->pos).norm() < unit->range && FreePath(&cUnit, &cTarget, model->map, true)) {
								targetInRange = target;
								break;
							}
						}*/
						Unit* targetInRange = canShootSomethingFromOrder(unit->pos, unit, Opponent(), model->map);
						if(targetInRange) {
							Eigen::Vector2d path = targetInRange->pos - unit->pos;
							double sin = path.y() / path.norm();
							double cos = path.x() / path.norm();
							Eigen::Matrix2d rot = Rotation(Angle(sin, cos));
							orders.push_back(new MoveOrder(unit->pos, rot, MOVE_PASSINGTHROUGH));
						}
						else if(unit->orders.empty() 
							|| unit->CurrentOrderCompleted()
							|| !canShootSomethingFromOrder(unit->orders.at(unit->currentOrder)->pos, unit, Opponent(), model->map)) {
							//Unit* target = targets.at(unitNum);
							//int unit_index = RNG::uniformInt(0, opponent->units.size() - 1);
							//target = opponent->units.at(unit_index);
							bool validShootingPosition = false;
							Eigen::Vector2d shootingPos;
							Eigen::Matrix2d rot = Eigen::Matrix2d::Identity();
							int attemptCount = 0;
							while(!validShootingPosition && attemptCount < 100) {
								//add counter to prevent infinite loop from invalid target; pick any position that is valid instead
								double x = RNG::uniformDouble(0, model->map->width);
								double y = RNG::uniformDouble(0, model->map->height);
								shootingPos << x, y;
								//check Freepath
								double unitRad = std::max((unit->ncols - 1)*unit->xspacing*0.5 + unit->soldiers.at(0).at(0)->rad, (unit->nrows - 1)*unit->xspacing*0.5 + unit->soldiers.at(0).at(0)->rad);
								Circle cUnit(shootingPos, unitRad);
								Circle cTarget(target->pos, unitRad);
								Eigen::Vector2d path = target->pos - shootingPos;
								double sin = path.y() / path.norm();
								double cos = path.x() / path.norm();
								rot = Rotation(Angle(sin, cos));
								// get rot
								if((shootingPos - target->pos).norm() < unit->range && FreePath(&cUnit, &cTarget, model->map, true) && !orderCollidesWithMapObjects(shootingPos, rot, model->map, unit)) {
									validShootingPosition = true;
									break;
								}
								attemptCount++;
							}
							if(validShootingPosition) {
								orders.push_back(new MoveOrder(shootingPos, rot, MOVE_PASSINGTHROUGH));
							}
						}
					}
				}
			}
			else {
				orders.push_back(new MoveOrder(unit->pos, unit->rot, MOVE_PASSINGTHROUGH));
			}
		}
		orderList.push_back(orders);
	}
	if(!gameStarted)
		gameStarted = true;
	GiveAllOrdersRequest gaor = GiveAllOrdersRequest(player, orderList);
	em->Post(&gaor);
	ContinueGameEvent cge;
	em->Post(&cge);
}

void SimpleAI::GivePlaceOrder(Unit* unit, std::vector<Order*>* orders) {
	// under construction: does modifying ordedrs like work, or does it need a pointer to the vector?
	Eigen::Vector2d pos;
	bool validOrder = false;
	while(!validOrder) {
		double x = RNG::uniformDouble(0, model->map->width);
		double y = RNG::uniformDouble(0, model->map->height);
		pos << x, y;
		if(orderInDeploymentZone(pos, model, unit)) {
			validOrder = true;
			break;
		}
	}
	Eigen::Matrix2d rot;
	MoveOrder* mo = new MoveOrder(pos, Eigen::Matrix2d::Identity(), MOVE_FORMUP);
	orders->push_back(mo);
}

void SimpleAI::ValidateOldTarget(int unitNum) {
	if(targets.at(unitNum)) {
		if(targets.at(unitNum)->nLiveSoldiers <= 0)
			targets.at(unitNum) = NULL;
	}
}

void SimpleAI::FindNewTarget(Unit* unit, int unitNum) {
	Unit* possibleTarget = Opponent()->units.at(RNG::uniformInt(0, Opponent()->units.size() - 1));
	if(possibleTarget->nLiveSoldiers > 0)
		targets.at(unitNum) = possibleTarget;
	else {
		std::vector<TargetContainer> possibleTargets;
		for(auto enemyUnit : Opponent()->units) {
			if(enemyUnit->nLiveSoldiers > 0)
				possibleTargets.push_back(TargetContainer(enemyUnit, (enemyUnit->pos - unit->pos).norm()));
		}
		std::sort(possibleTargets.begin(), possibleTargets.end());
		if(possibleTargets.size() > 0)
			targets.at(unitNum) = possibleTargets.at(0).target;
	}
}

void SimpleAI::GiveOrderToTarget(Unit* unit, int unitNum, std::vector<Order*>* orders) {
	Unit* target = targets.at(unitNum);
	if(!unit->ranged) {
		if(unit->orders.empty() || unit->orders.at(unit->currentOrder)->type != ORDER_ATTACK) {
			orders->push_back(new AttackOrder(target, target->pos));
		}
	}
	else {
		// if no ranged target, but some target in range and los: move order to stop and shoot them
		if(!unit->rangedTarget) {
			Unit* targetInRange = canShootSomethingFromOrder(unit->pos, unit, Opponent(), model->map);
			if(targetInRange) {
				Eigen::Vector2d path = targetInRange->pos - unit->pos;
				double sin = path.y() / path.norm();
				double cos = path.x() / path.norm();
				Eigen::Matrix2d rot = Rotation(Angle(sin, cos));
				orders->push_back(new MoveOrder(unit->pos, rot, MOVE_PASSINGTHROUGH));
			}
			else if(unit->orders.empty() 
				|| unit->CurrentOrderCompleted()
				|| !canShootSomethingFromOrder(unit->orders.at(unit->currentOrder)->pos, unit, Opponent(), model->map)) {
				bool validShootingPosition = false;
				Eigen::Vector2d shootingPos;
				Eigen::Matrix2d rot = Eigen::Matrix2d::Identity();
				int attemptCount = 0;
				while(!validShootingPosition && attemptCount < 100) {
					//add counter to prevent infinite loop from invalid target; pick any position that is valid instead
					double x = RNG::uniformDouble(0, model->map->width);
					double y = RNG::uniformDouble(0, model->map->height);
					shootingPos << x, y;
					//check Freepath
					double unitRad = std::max((unit->ncols - 1)*unit->xspacing*0.5 + unit->soldiers.at(0).at(0)->rad, (unit->nrows - 1)*unit->xspacing*0.5 + unit->soldiers.at(0).at(0)->rad);
					Circle cUnit(shootingPos, unitRad);
					Circle cTarget(target->pos, unitRad);
					Eigen::Vector2d path = target->pos - shootingPos;
					double sin = path.y() / path.norm();
					double cos = path.x() / path.norm();
					rot = Rotation(Angle(sin, cos));
					// get rot
					if((shootingPos - target->pos).norm() < unit->range && FreePath(&cUnit, &cTarget, model->map, true) && !orderCollidesWithMapObjects(shootingPos, rot, model->map, unit)) {
						validShootingPosition = true;
						break;
					}
					attemptCount++;
				}
				if(validShootingPosition) {
					orders->push_back(new MoveOrder(shootingPos, rot, MOVE_PASSINGTHROUGH));
				}
			}
		}
	}
}

#endif