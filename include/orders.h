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
		int type;
		virtual void ConversionEnabler() {}
		Order() {
			type = ORDER_ORDER;
		}
};

class MoveOrder : public Order {
	public:
		int moveType;
		Eigen::Vector2d pos;
		Eigen::Matrix2d rot;
		double angleTarget;

		MoveOrder(Eigen::Vector2d pos, Eigen::Matrix2d rot, int moveType) : Order() {
			type = ORDER_MOVE;
			this->moveType = moveType;
			this->pos = pos;
			this->rot = rot;
			angleTarget = Angle(rot.coeff(0,1), rot.coeff(0,0));
		}
};

#endif