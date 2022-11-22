#define EVENTS

#ifndef LINKEDLISTS
#include "linkedlists.h"
#endif

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
	APPEND_ORDERS_REQUEST,
	UNIT_MOVE_REQUEST,
	UNIT_PLACE_REQUEST,
	KILL_EVENT,
	UNIT_SELECT_EVENT,
	PLAYER_SELECT_EVENT
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

class PlayerSelectEvent : public Event {
	public:
		int playerID;
		
		PlayerSelectEvent(int pID) : Event() {
			name = "PlayerSelectEvent";
			type = PLAYER_SELECT_EVENT;
			playerID = pID;
		}
};