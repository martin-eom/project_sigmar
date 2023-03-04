#ifndef ORDERS
#define ORDERS

#include <base.h>

#include <Dense>

enum ORDER_TYPES{
	ORDER_ORDER,
	ORDER_MOVE,
	ORDER_ATTACK
};

enum MOVEMENT_TYPE{
	MOVE_FORMUP,
	MOVE_PASSINGTHROUGH
};


class Order{

	public:
		virtual void ConversionEnabler() {}
		Eigen::Vector2d pos;
		Eigen::Matrix2d rot;
		double angleTarget;
		int type;
		bool _auto;
		bool _transition;
		Order() {
			type = ORDER_ORDER;
			pos << 0, 0;
			rot << 1, 0, 0, 1;
			angleTarget = Angle(rot.coeff(0,1), rot.coeff(0,0));
		}
		Order(bool _auto, bool _transition = false) : Order() {
			this->_auto = _auto;
			this->_transition = _transition;
		}
};

class MoveOrder : public Order {
	public:
		int moveType;

		MoveOrder(Eigen::Vector2d pos, Eigen::Matrix2d rot, int moveType, bool _auto = false, bool _transition = false) : Order(_auto, _transition) {
			type = ORDER_MOVE;
			this->moveType = moveType;
			this->pos = pos;
			this->rot = rot;
			angleTarget = Angle(rot.coeff(0,1), rot.coeff(0,0));
		}
};

class AttackOrder : public Order {
public:
	Unit* unit;
	bool enemyContact;
	void SetPos();

	AttackOrder(Unit* unit, bool _auto = false, bool _transition = false) : Order(_auto, _transition) {
		type = ORDER_ATTACK;
		this->unit = unit;
		enemyContact = false;
	}
};

#endif