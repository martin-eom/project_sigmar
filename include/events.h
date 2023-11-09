#ifndef EVENTS
#define EVENTS


#include <list>
#include <string>
#include <Dense>
#include <SDL.h>
#include <vector>

enum EVENT_IDS {
	GENERIC_EVENT,
	SDL_EVENT,
	TICK_EVENT,
	REFORM_EVENT,
	QUIT_EVENT,
	CLICK_EVENT,
	GIVE_ORDERS_REQUEST,
	REMEMBER_ORDERS,
	APPEND_ORDERS_REQUEST,
	UNIT_MOVE_REQUEST,
	UNIT_PLACE_REQUEST,
	KILL_EVENT,
	UNIT_SELECT_EVENT,
	UNIT_ADD_EVENT,
	UNIT_DELETE_EVENT,
	UNIT_ROSTER_MODIFIED_EVENT,
	PLAYER_SELECT_EVENT,
	PLAYER_ADD_EVENT,
	PLAYER_DELETE_EVENT,
	CTRL_STATE_EVENT,
	INPUT_RECEIVED_EVENT,
	CHANGE_TEXTBOX_EVENT,
	PROJECTILE_SPAWN_EVENT
};


class Event {
	public:
		std::string name;
		int type;
		virtual void ConversionEnabler() {}

		Event() {
			name = "GenericEvent";
			type = GENERIC_EVENT;
		}
};

class SDLEvent : public Event {
	public:
		SDL_Event event;

		SDLEvent(SDL_Event event) : Event() {
			this->event = event;
			name = "SDLEvent";
			type = SDL_EVENT;
		}
};

class TickEvent : public Event {
	public:
		TickEvent() : Event() {
			name = "TickEvent";
			type = TICK_EVENT;
		}
};

class ReformEvent : public Event {
	public:
		ReformEvent() : Event() {
			name = "ReformEvent";
			type = REFORM_EVENT;
		}
};

class QuitEvent : public Event {
	public:
		QuitEvent() : Event() {
			name = "QuitEvent";
			type = QUIT_EVENT;
		}
};

class ClickEvent : public Event {
	public:
		Eigen::Vector2d pos;

		ClickEvent(Eigen::Vector2d pos) : Event() {
			name = "ClickEvent";
			type = CLICK_EVENT;
			this->pos = pos;
		}
};

class Unit;
class Order;

class GiveOrdersRequest : public Event {
	public:
		Unit* unit;
		std::vector<Order*> orders;

		GiveOrdersRequest(Unit* unit, std::vector<Order*> orders) : Event(){
			name = "GiveOrdersRequest";
			type = GIVE_ORDERS_REQUEST;
			this->unit = unit;
			this->orders = orders;
		}
};

class RememberOrders : public Event {
	public:
		std::vector<Order*> orders;

		RememberOrders(std::vector<Order*> orders) : Event() {
			name = "RememberOrders";
			type = REMEMBER_ORDERS;
			this->orders = orders;
		}
};

class AppendOrdersRequest : public Event {
	public:
		Unit* unit;
		std::vector<Order*> orders;

		AppendOrdersRequest(Unit* unit, std::vector<Order*> orders) : Event(){
			name = "AppendOrdersRequest";
			type = APPEND_ORDERS_REQUEST;
			this->unit = unit;
			this->orders = orders;
		}
};

class UnitMoveRequest : public Event {
	public:
		Unit* unit;
		Eigen::Vector2d pos;
		Eigen::Matrix2d rot;
		
		UnitMoveRequest(Unit* unit, Eigen::Vector2d pos, Eigen::Matrix2d rot) : Event() {
			name = "UnitMoveRequest";
			type = UNIT_MOVE_REQUEST;
			this->unit = unit;
			this->pos = pos;
			this->rot = rot;
		}
};

class UnitPlaceRequest : public Event {
	public:
		Unit* unit;
		Eigen::Vector2d pos;
		Eigen::Matrix2d rot;
		
		UnitPlaceRequest(Unit* unit, Eigen::Vector2d pos, Eigen::Matrix2d rot) : Event() {
			name = "UnitPlaceRequest";
			type = UNIT_PLACE_REQUEST;
			this->unit = unit;
			this->pos = pos;
			this->rot = rot;
		}
};

class Soldier;
class Unit;

class KillEvent : public Event {
	public:
		Soldier* soldier;
		Unit* unit;
		
		KillEvent(Soldier* soldier) : Event() {
			name = "KillEvent";
			type = KILL_EVENT;
			this->soldier = soldier;
		}
};

class UnitSelectEvent : public Event {
	public:
		int unitID;
		
		UnitSelectEvent(int uID) : Event() {
			name = "UnitSelectEvent";
			type = UNIT_SELECT_EVENT;
			unitID = uID;
		}
};

class UnitAddEvent : public Event {
	public:
		int unitType;

		UnitAddEvent(int type) : Event() {
			name = "UnitAddEvent";
			this->type = UNIT_ADD_EVENT;
			unitType = type;
		}
};

class UnitDeleteEvent : public Event {
	public:
		UnitDeleteEvent() : Event() {
			name = "UnitDeleteEvent";
			type = UNIT_DELETE_EVENT;
		}
};

class UnitRosterModifiedEvent : public Event {
	public:
		UnitRosterModifiedEvent() : Event() {
			name = "UnitRosterModifiedEvent";
			type = UNIT_ROSTER_MODIFIED_EVENT;
		}
};

class PlayerSelectEvent : public Event {
	public:
		int playerID;
		
		PlayerSelectEvent(int pID) : Event() {
			name = "PlayerSelectEvent";
			type = PLAYER_SELECT_EVENT;
			playerID = pID;
		}
};

class PlayerAddEvent : public Event {
	public:
		PlayerAddEvent() : Event() {
			name = "PlayerAddEvent";
			type = PLAYER_ADD_EVENT;
		}
};

class PlayerDeleteEvent : public Event {
	public:
		PlayerDeleteEvent() : Event() {
			name = "PlayerDeleteEvent";
			type = PLAYER_DELETE_EVENT;
		}
};

class CtrlStateEvent : public Event {
public:
	int state;

	CtrlStateEvent(int state) : Event() {
		name = "CtrlStateEvent";
		type = CTRL_STATE_EVENT;
		this->state = state;
	}
};

class InputReceivedEvent : public Event {
public:
	InputReceivedEvent() : Event() {
		name = "InputReceivedEvent";
		type = INPUT_RECEIVED_EVENT;
	}
};

class ChangeTextboxEvent : public Event {
public:
	std::string text;

	ChangeTextboxEvent(std::string text) : Event() {
		name = "ChangeTextboxEvent";
		type = CHANGE_TEXTBOX_EVENT;
		this->text = text;
	}
};


class Projectile;

class ProjectileSpawnEvent : public Event {
public:
	Projectile* p;

	ProjectileSpawnEvent(Projectile* p) {
		name = "ProjectileSpawnEvent";
		type = PROJECTILE_SPAWN_EVENT;
		this->p = p;
	}
};

#endif