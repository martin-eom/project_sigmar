#ifndef ORDERS
#define ORDERS

#include <base.h>
#include <extra_math.h>

#include <memory>
#include <Eigen/Dense>

enum ORDER_TYPES{
	ORDER_ORDER,
	ORDER_MOVE,
	ORDER_ATTACK,
	ORDER_TARGET
};

enum MOVEMENT_TYPE{
	MOVE_FORMUP,
	MOVE_PASSINGTHROUGH
};

/* Orders are shared-owned.
*
*  Rule of thumb: OrderPtr for anything that stores an order, a raw Order*
*  (via .get()) for a local that merely reads one.
*
*  The OrderPtr alias is declared in events.h.
*/

class Order{

	public:
		virtual void ConversionEnabler() {}
		Eigen::Vector2d pos;
		Eigen::Matrix2d rot;
		double angleTarget;
		int type;
		bool _auto;
		bool _transition;
		bool _combat = false;
		Unit* target;

		void setCombat() {
			_combat = true;
		}

		Order() {
			type = ORDER_ORDER;
			pos << 0, 0;
			rot << 1, 0, 0, 1;
			angleTarget = Angle(rot.coeff(0,1), rot.coeff(0,0));
		}
		Order(bool _auto, bool _transition = false, Unit* target = NULL) : Order() {
			this->_auto = _auto;
			this->_transition = _transition;
			this->target = target;
		}
		// Orders are always held and deleted as Order*, so this must be virtual.
		virtual ~Order() = default;
};

class MoveOrder : public Order {
	public:
		int moveType;

		MoveOrder(Eigen::Vector2d pos, Eigen::Matrix2d rot, int moveType, 
			bool _auto = false, bool _transition = false, Unit* target = NULL)
			: Order(_auto, _transition, target) {
			type = ORDER_MOVE;
			this->moveType = moveType;
			this->pos = pos;
			this->rot = rot;
			angleTarget = Angle(rot.coeff(0,1), rot.coeff(0,0));
		}
};

class AttackOrder : public Order {
public:
	bool enemyContact;
	void SetPos();

	AttackOrder(Unit* unit, Eigen::Vector2d pos, bool _auto = false, bool _transition = false) : Order(_auto, _transition, unit) {
		type = ORDER_ATTACK;
		this->target = unit;
		enemyContact = false;
		this->pos = pos;
	}
};

class TargetOrder : public Order {
public:
	Unit* target;

	TargetOrder(Eigen::Vector2d pos, Eigen::Matrix2d rot, Unit* target, bool _auto = false, bool _transition = false) : Order(_auto, _transition, target) {
		type = ORDER_TARGET;
		this->target = target; // redundant
		this->pos = pos;
		this->rot = rot;
		this->angleTarget = Angle(rot.coeff(0,1), rot.coeff(0,0));
	}
};

#endif