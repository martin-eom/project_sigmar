#ifndef INPUT
#define INPUT

#include <base.h>
#include <gui_base.h>
#include <events.h>
#include <player.h>
#include <fileio.h>
#include <information.h>

#include <stdio.h>
#include <iostream>
#include <SDL.h>
#include <Dense>
#include <cmath>
#include <string.h>


enum CONTROLLER_STATES {
	CTRL_IDLE,
	CTRL_LOADING,
	CTRL_QUITTING,
	CTRL_SELECTING_PLAYER,
	CTRL_SELECTING_UNIT,
	CTRL_ADDING_UNIT,
	CTRL_GIVING_ORDERS
};


class KeyboardAndMouseController : public ZoomableGUIController {
private:
	bool firstPointSet;
	bool queueingOrders;
	bool _inputConfirmed;
	std::string _input;
	int _state;
	bool _help;
	bool _shift;
	bool _ctrl;
	void SetPlayer();
	void SetUnit();
public:
	std::vector<Order*> orders;
	std::vector<std::vector<Order*>> orderList;
	Eigen::Vector2d p0, p1;
	Eigen::Matrix2d rot;
	bool passingThrough;
	Model* model;
	Player* selectedPlayer;
	Unit* selectedUnit;
		
	bool help() {return _help;}
	int state() {return _state;}
	bool shift() {return _shift;}
	bool ctrl() {return _ctrl;}
	bool inputConfirmed() {return _inputConfirmed;}
	std::string input() {return _input;}
	int newUnitType;

	KeyboardAndMouseController(EventManager* em, int SCREEN_WIDTH, int SCREEN_HEIGHT, Map* map) : ZoomableGUIController(em, SCREEN_WIDTH, SCREEN_HEIGHT, map) {
		_state = CTRL_IDLE;
		_help = false;
		_shift = false;
		_ctrl = false;
		firstPointSet = false;
		passingThrough = false;
		queueingOrders = false;
		_inputConfirmed = false;
		newUnitType = 0;
		rot << 1, 0, 0, 1;
		model = Gem()->model;
		selectedPlayer = model->player1;
		SetPlayer();
		newOrderList(selectedPlayer);
		SetUnit();
	};
private:
	void handleSDLEvent(SDL_Event e, GameEventManager* gem, Model* model, GeneralView* view, Map* map);
	void handleMouseKeyUpEvent(SDL_Event e, GameEventManager* gem, Model* model, GeneralView* view, Map* map);
	void handleKeyDownEvent(SDL_Event e, GameEventManager* gem, Model* model, GeneralView* view, Map* map);
	void handleKeyUpEvent(SDL_Event e, GameEventManager* gem, Model* model, GeneralView* view, Map* map);
	void handleTextInputEvent(SDL_Event e, GameEventManager* gem, Model* model, GeneralView* view, Map* map);
	void handleMouseMotionEvent(SDL_Event e, GameEventManager* gem, Model* model, GeneralView* view, Map* map);
	void newOrderList(Player* player);

	void Notify(Event* ev) {
		GameEventManager* gem = Gem();
		Model* model = gem->model;
		GeneralView* view = gem->view;
		Map* map = gem->map;
		switch(ev->type) {
		case CTRL_STATE_EVENT: {
			CtrlStateEvent* cev = dynamic_cast<CtrlStateEvent*>(ev);
			_state = cev->state; }
			break;
		case INPUT_RECEIVED_EVENT:
			_inputConfirmed = false;
			_input = "";
			break;
		case REMEMBER_ORDERS: {
			debug("Remembered Orders!");
			RememberOrders* rem = dynamic_cast<RememberOrders*>(ev);
			orders = rem->orders; }
			break;
		case SDL_EVENT:
			SDL_Event e = dynamic_cast<SDLEvent*>(ev)->event;
			handleSDLEvent(e, gem, model, view, map);
			break;
		case GAME_PAUSED_EVENT:
			if(!selectedPlayer) SetPlayer();
			else {
				selectedPlayer = model->players.at(
					(std::find(model->players.begin(), model->players.end(), selectedPlayer)
					- model->players.begin() + 1)
					% model->players.size()
				);
			}
			newOrderList(selectedPlayer);
			SetUnit();
			break;
		case TICK_EVENT:
			debug("TickEvent: ctrl - begin");
			double oldZoom = zoom;
			zoom = zoom * std::pow(maxZoom, em->dt/1*(zoomSpeedIn - zoomSpeedOut));
			if(zoom < minZoom) zoom = minZoom;
			else if(zoom > maxZoom) zoom = maxZoom;
			Eigen::Vector2d newCenter;
			double x, y;
			double xmin = view->SCREEN_WIDTH/2. - 200;
			double xmax = zoom*map->width - view->SCREEN_WIDTH/2. + 200;
			double ymin = view->SCREEN_HEIGHT/2. - 200;
			double ymax = zoom*map->height - view->SCREEN_HEIGHT/2. + 200;
			x = center.coeff(0)*zoom/oldZoom + em->dt * (zoomSpeedRight - zoomSpeedLeft) * view->SCREEN_WIDTH;
			if(x < xmin) x = xmin;
			if(x > xmax) x = xmax;
			y = center.coeff(1)*zoom/oldZoom + em->dt * (zoomSpeedUp - zoomSpeedDown) * view->SCREEN_WIDTH;
			if(y < ymin) y = ymin;
			if(y > ymax) y = ymax;
			newCenter << x, y;
			center = newCenter;
			debug("TickEvent: ctrl - end");
			break;
		}
	}
};

void KeyboardAndMouseController::SetPlayer() {
	selectedPlayer = NULL;
	if(model->players.size() > 0) {
		selectedPlayer = model->players.at(0);
	}
}

void KeyboardAndMouseController::SetUnit() {
	selectedUnit = NULL;
	if(!selectedPlayer) {
		SetPlayer();
	}
	if(selectedPlayer) {
		if(selectedPlayer->units.size() > 0) {
			selectedUnit = selectedPlayer->units.at(0);
		}
	}
}

void KeyboardAndMouseController::newOrderList(Player* player) {
	orderList.clear();
	for(auto unit: player->units) {
		orderList.push_back(std::vector<Order*>());
	}
	//add sublists for every unit of player
}

void KeyboardAndMouseController::handleSDLEvent(SDL_Event e, GameEventManager* gem, Model* model, GeneralView* view, Map* map) {
	switch(e.type) {
	case SDL_MOUSEBUTTONUP:
		handleMouseKeyUpEvent(e, gem, model, view, map); break;
	case SDL_KEYDOWN:
		handleKeyDownEvent(e, gem, model, view, map); break;
	case SDL_KEYUP:
		handleKeyUpEvent(e, gem, model, view, map); break;
	case SDL_TEXTINPUT:
		handleTextInputEvent(e, gem, model, view, map); break;
	case SDL_MOUSEMOTION:
		handleMouseMotionEvent(e, gem, model, view, map); break;
	}
}

void KeyboardAndMouseController::handleMouseKeyUpEvent(SDL_Event e, GameEventManager* gem, Model* model, GeneralView* view, Map* map) {
	Eigen::Vector2d diag; diag << view->SCREEN_WIDTH / 2., view->SCREEN_HEIGHT / 2.;
	Eigen::Vector2d pos; pos << e.button.x, view->SCREEN_HEIGHT - e.button.y;
	Eigen::Vector2d mousePos;
	mousePos = (pos + center - diag) / zoom;
	int x, y;
	x = mousePos.coeff(0);
	y = mousePos.coeff(1);
	switch(_state) {
	case CTRL_GIVING_ORDERS: {
		Order* o = NULL;
		if(e.button.button == SDL_BUTTON_RIGHT) {
			if(firstPointSet) {
				p1 << x, y;
				double dx = p1.coeff(0) - p0.coeff(0);
				double dy = p1.coeff(1) - p0.coeff(1);
				double dp = sqrt(dx*dx + dy*dy);
				if(dp !=0 ) {
					double cos = dx / dp;
					double sin = dy / dp;
					rot << cos, -sin, sin, cos;
					MoveOrder* mo = new MoveOrder(p0, rot, MOVE_FORMUP);
					o = new MoveOrder(p0, rot, MOVE_FORMUP);
					if(passingThrough) {mo->moveType = MOVE_PASSINGTHROUGH; dynamic_cast<MoveOrder*>(o)->moveType = MOVE_PASSINGTHROUGH;}
				}
				else {
					std::cout << "First and second point are identical, can't get angel.\n";
				}
			}
			else {
				p0 << x, y;
			}
			firstPointSet = !firstPointSet;
		}
		else if(e.button.button == SDL_BUTTON_LEFT) {
			Eigen::Vector2d realPos; realPos << x, y;
			Player* player = selectedPlayer;
			if(selectedPlayer->player1)
				player = model->players.at(1);
			else
				player = model->players.at(0);
			double dist;
			double minDist = std::numeric_limits<double>::infinity();
			Unit* target = NULL;
			for(auto unit : player->units) {
				if(unit->placed && unit->nLiveSoldiers > 0) {
					dist = (realPos - unit->pos).norm();
					if(dist < minDist) {
						target = unit;
						minDist = dist;
					}
				}
			}
			if(target) {
				if(_shift) {
					Eigen::Vector2d pos;
					Eigen::Matrix2d rot;
					if(orders.size() > 0) {
						pos = orders.at(orders.size()-1)->pos;
						rot = orders.at(orders.size()-1)->rot;
					}
					else {
						pos = selectedUnit->pos;
						rot = selectedUnit->rot;
					}
					o = new TargetOrder(pos, rot, target);
				}
				else {
					o = new AttackOrder(target);
				}
			}
		}
		if(!queueingOrders) {orders.clear();}
		if(o) {
			orders.push_back(o);
		}
		std::cout << orders.size() << " orders queued\n";
		}break;
	case CTRL_SELECTING_UNIT:
		double dist;
		double minDist = 1000000;	//infinity
		Eigen::Vector2d mouse; mouse << x, y;
		for(auto unit : selectedPlayer->units) {
			if(unit->placed) {
				dist = (unit->pos - mouse).norm();
				if(dist < minDist) {
					selectedUnit = unit;
					em->Post(new RememberOrders(unit->orders));
					minDist = dist;
				}
			}
		}
	}
}

void KeyboardAndMouseController::handleKeyDownEvent(SDL_Event e, GameEventManager* gem, Model* model, GeneralView* view, Map* map) {
	switch(e.key.keysym.sym) {
	case SDLK_LSHIFT:
		_shift = true;
		break;
	case SDLK_LCTRL:
		_ctrl = true;
		if(!e.key.repeat) {
			queueingOrders = true;
			std::cout << "Queueing orders...\n";
		}
		break;
	case SDLK_PLUS:
		if(!(_shift || SDL_IsTextInputActive()))
		zoomSpeedIn = zoomSpeed;
		break;
	case SDLK_MINUS:
		if(!(_shift || SDL_IsTextInputActive()))
		zoomSpeedOut = zoomSpeed;
		break;
	case SDLK_i:
		if(!(_shift || SDL_IsTextInputActive()))
		zoomSpeedUp = zoomMoveSpeed;
		break;
	case SDLK_j:
		if(!(_shift || SDL_IsTextInputActive()))
		zoomSpeedLeft = zoomMoveSpeed;
		break;
	case SDLK_k:
		if(!(_shift || SDL_IsTextInputActive()))
		zoomSpeedDown = zoomMoveSpeed;
		break;
	case SDLK_l:
		if(!(_shift || SDL_IsTextInputActive()))
		zoomSpeedRight = zoomMoveSpeed;
		break;
	}
}

void KeyboardAndMouseController::handleKeyUpEvent(SDL_Event e, GameEventManager* gem, Model* model, GeneralView* view, Map* map) {
	Event* nev = new Event();
	switch(e.key.keysym.sym) {
	case SDLK_RETURN:
		debug(std::to_string(_state));
		switch(_state) {
		case CTRL_IDLE:
			break;
		case CTRL_GIVING_ORDERS:
			if(selectedUnit) {
				if(orders.size() > 0) {
					switch(model->state) {
					case MODEL_SIMULATION: {
						if(queueingOrders) {
							nev = new AppendOrdersRequest(selectedUnit, orders);
						}
						else {
							nev = new GiveOrdersRequest(selectedUnit, orders);
						}
						}break;
					case MODEL_GAME_PAUSED:
						orderList.at(selectedPlayer->getUnitID(selectedUnit)) = orders;
					}
					debug(std::to_string(_state));

				}
				else {std::cout << "No orders sent, order list is empty.\n";}
			}
			else {
				std::cout << "No unit was selected. Attempting to set unit..\n";
				SetUnit();
			}
			debug(std::to_string(_state));
			break;
		case CTRL_LOADING:
			_inputConfirmed = true;
			break;
		case CTRL_ADDING_UNIT:
			debug("unit type:" + std::to_string(newUnitType));
			/*switch(newUnitType) {
			case 0:
				em->Post(new UnitAddEvent(UNIT_INFANTRY)); break;
			case 1: case 1-4:
				em->Post(new UnitAddEvent(UNIT_CAVALRY)); break;
			case 2: case 2-4:
				em->Post(new UnitAddEvent(UNIT_MONSTER)); break;
			case 3: case 3-4:
				em->Post(new UnitAddEvent(UNIT_LONE_RIDER)); break;
			}*/
			_state = CTRL_SELECTING_UNIT;
			debug("New Unit Type: " + std::to_string(newUnitType));
			break;
		}
		break;
	case SDLK_BACKSPACE:
		switch(SDL_IsTextInputActive()) {
		case true:
			if(_input.size() > 0) _input.pop_back(); break;
		}
		break;
	case SDLK_ESCAPE:
		switch(_state) {
		case CTRL_LOADING:
			_state = CTRL_IDLE;
			_inputConfirmed = false;
			_input = "";
			break;
		case CTRL_SELECTING_PLAYER:
		case CTRL_SELECTING_UNIT:
		case CTRL_GIVING_ORDERS:
			_state = CTRL_IDLE;
			break;
		case CTRL_ADDING_UNIT:
			_state = CTRL_SELECTING_UNIT;
			break;
		case CTRL_IDLE:
			selectedUnit = NULL;
			break;
		}break;
	case SDLK_LEFT:
		switch(_state) {
		case CTRL_SELECTING_UNIT:
			if(!selectedPlayer) SetPlayer();
			if(selectedPlayer) {
				auto it = std::find(selectedPlayer->units.begin(), selectedPlayer->units.end(), selectedUnit);
				if(it == selectedPlayer->units.begin()) selectedUnit = *(--selectedPlayer->units.end());
				else selectedUnit = *std::prev(it);
			}
			else selectedUnit = NULL;
			break;
		}break;
	case SDLK_RIGHT:
		switch(_state) {
		case CTRL_SELECTING_UNIT:
			if(!selectedPlayer) SetPlayer();
			if(selectedPlayer) {
				auto it = std::find(selectedPlayer->units.begin(), selectedPlayer->units.end(), selectedUnit);
				if(it == (--selectedPlayer->units.end())) selectedUnit = *(selectedPlayer->units.begin());
				else selectedUnit = *std::next(it);
			}
			else selectedUnit = NULL;
			break;
		}break;
	case SDLK_UP:
		switch(_state) {
		case CTRL_SELECTING_PLAYER:
			if(model->settings.simulation_mode) {
				if(!selectedPlayer) SetPlayer();
				else {
					auto it = std::find(model->players.begin(), model->players.end(), selectedPlayer);
					if(it == model->players.begin()) selectedPlayer = *(--model->players.end());
					else selectedPlayer = *std::prev(it);
					newOrderList(selectedPlayer);
					SetUnit();
				}
			}
			break;
		case CTRL_ADDING_UNIT:
			newUnitType = (newUnitType - 1) % 4;
			break;
		}break;
	case SDLK_DOWN:
		switch(_state) {
		case CTRL_SELECTING_PLAYER:
			if(model->settings.simulation_mode) {
				if(!selectedPlayer) SetPlayer();
				else {
					auto it = std::find(model->players.begin(), model->players.end(), selectedPlayer);
					if(it == --(model->players.end())) selectedPlayer = *(model->players.begin());
					else selectedPlayer = *std::next(it);
					newOrderList(selectedPlayer);
					SetUnit();
				}
			}
			break;
		case CTRL_ADDING_UNIT:
			newUnitType = (newUnitType + 1) % 4;
			break;
		}break;
	case SDLK_LSHIFT:
		_shift = false;
		break;
	case SDLK_LCTRL:
		_ctrl = false;
		queueingOrders = false;
		std::cout << "...done.\n";
		break;
	case SDLK_PLUS:
		zoomSpeedIn = 0.;
		break;
	case SDLK_MINUS:
		zoomSpeedOut = 0.;
		break;
	case SDLK_a:
		switch(_state) {
		case CTRL_SELECTING_PLAYER:
			em->Post(new PlayerAddEvent()); break;
		case CTRL_SELECTING_UNIT:
			_state = CTRL_ADDING_UNIT; break;
		}
		break;
	case SDLK_d:
		switch(_state) {
		case CTRL_SELECTING_PLAYER:
			em->Post(new PlayerDeleteEvent()); break;
		case CTRL_SELECTING_UNIT:
			em->Post(new UnitDeleteEvent()); break;
		}
		break;
	case SDLK_h:
		if(!SDL_IsTextInputActive())
			_help = !_help; break;
	case SDLK_i:
		zoomSpeedUp = 0.;
		break;
	case SDLK_j:
		zoomSpeedLeft = 0.;
		break;
	case SDLK_k:
		zoomSpeedDown = 0.;
	case SDLK_l:
		if(_shift) _state = CTRL_LOADING;
		zoomSpeedRight = 0.;
		break;
	case SDLK_o:
		switch(_state) {
		case CTRL_IDLE:
			_state = CTRL_GIVING_ORDERS;
			break;
		}
		break;
	case SDLK_p:
		switch(_state) {
		case CTRL_IDLE:
			_state = CTRL_SELECTING_PLAYER;
			break;
		case CTRL_GIVING_ORDERS:
			passingThrough = !passingThrough;
			if(passingThrough) {std::cout << "moveType: passing through\n";}
			else {std::cout << "moveType: forming up\n";}
			break;
		}
		break;
	case SDLK_q:
		if(_shift) _state = CTRL_QUITTING;
		break;
	case SDLK_s:
		if(_shift) {}
		else {
			switch(_state) {
			case CTRL_IDLE: {
				if(model->state == MODEL_GAME_PAUSED) {
					bool allowedDeployment = true;
					/* check if deployment follows all rules
					* every unit that is not yet placed must
					*   have at least one order
					*   have its first order be a move order
					*   have its first order in the players deployment zone
					*/
					for(auto it = orderList.begin(); it != orderList.end(); it = std::next(it)) {
						int index = it - orderList.begin();
						if(!(selectedPlayer->units.at(index)->placed)) {
							if(orderList.at(index).size() == 0) {
								allowedDeployment = false; break;
							}
							if(orderList.at(index).at(0)->type != ORDER_MOVE) {
								allowedDeployment = false; break;
							}
							bool orderInDeploymentZone = false;
							for(auto dz : map->deploymentZones) {
								Point orderPos(orderList.at(index).at(0)->pos);
								if(dz->player_id == std::find(
									model->players.begin(), model->players.end(), selectedPlayer)
									- model->players.begin()) {
									if(PointRectangleCollision(&orderPos, dz)) {
										orderInDeploymentZone = true; break;
									}
								}
							}
							if(!orderInDeploymentZone) {
								allowedDeployment = false; break;
							}
						}
					}
					if(allowedDeployment) {
						GiveAllOrdersRequest gaor = GiveAllOrdersRequest(selectedPlayer, orderList);
						em->Post(&gaor);
						ContinueGameEvent cge;
						em->Post(&cge);
					}
				}
				break;}
			}
		}
		break;
	case SDLK_u:
		switch(_state) {
		case CTRL_IDLE:
			_state = CTRL_SELECTING_UNIT;
			if(selectedUnit == NULL)
				SetUnit();
			break;
		}
	}
	if(nev->type != GENERIC_EVENT) {
		em->Post(nev);
	}
}

void KeyboardAndMouseController::handleTextInputEvent(SDL_Event e, GameEventManager* gem, Model* model, GeneralView* view, Map* map) {
	_input += e.text.text;
}

void KeyboardAndMouseController::handleMouseMotionEvent(SDL_Event e, GameEventManager* gem, Model* model, GeneralView* view, Map* map) {
	Eigen::Vector2d mousePos;
	Eigen::Vector2d diag; diag << view->SCREEN_WIDTH / 2., view->SCREEN_HEIGHT / 2.;
	Eigen::Vector2d pos; pos << e.button.x, view->SCREEN_HEIGHT - e.button.y;
	mousePos = (pos + center - diag) / zoom;
	int x, y;
	x = mousePos.coeff(0);
	y = mousePos.coeff(1);
	if(firstPointSet) {
		p1 << x, y;
		double dx = p1.coeff(0) - p0.coeff(0);
		double dy = p1.coeff(1) - p0.coeff(1);
		double dp = sqrt(dx*dx + dy*dy);
		if(dp !=0 ) {
			double cos = dx / dp;
			double sin = dy / dp;
			rot << cos, -sin, sin, cos;
		}
	}
	else p0 << x, y;
	if(_state == CTRL_GIVING_ORDERS && selectedUnit) {
		if(!selectedUnit->placed) {
			selectedUnit->pos << x, y;
			selectedUnit->rot = rot;
		}
	}
}


enum EDITOR_STATES {
	EDITOR_IDLE,
	EDITOR_PLACING_RECTANGLE,
	EDITOR_ROTATING_RECTANGLE,
	EDITOR_ENTERING_REC_WIDTH,
	EDITOR_ENTERING_REC_HEIGHT,
	EDITOR_PLACING_DP_ZONE,
	EDITOR_ROTATING_DP_ZONE,
	EDITOR_ENTERING_DP_ZONE_WIDTH,
	EDITOR_ENTERING_DP_ZONE_HEIGHT,
	EDITOR_ENTERING_DP_ZONE_ID,
	EDITOR_PLACING_CIRCLE,
	EDITOR_ENTERING_CIRCLE_RAD,
	EDITOR_PLACING_WP,
	EDITOR_ENTERING_WP_RAD,
	EDITOR_NEWMAP,
	EDITOR_NEWMAP_WIDTH,
	EDITOR_NEWMAP_HEIGHT,
	EDITOR_NEWMAP_FINISH,
	EDITOR_CLOSING,
	EDITOR_SAVING,
	EDITOR_LOADING,
	EDITOR_SELECTING,
	EDITOR_MOVING,
	EDITOR_COPYING,
	EDITOR_PATHFINDING,
	EDITOR_ENTERING_AUTO_WP_RAD,
	EDITOR_CALCULATING_AUTO_WP
};

class MapEditorController : public ZoomableGUIController {
private:
	Eigen::Vector2d p0, p1;
public:
	Map* map;
	int state;
	int prevState;

	bool shift;
	bool help;
	Eigen::Vector2d mousePos;
	Eigen::Matrix2d rot;
	std::string input;
	bool input_confirmed;

	int lastCircleRad;
	int lastRecWidth;
	int lastRecHeight;
	Eigen::Matrix2d lastRot;

	MapObject* objToPlace;
	int selectedObject;
	MapObject* selectedObj;
	int new_SCREEN_WIDTH;
	int new_SCREEN_HEIGHT;

	MapEditorController(EventManager* em, int SCREEN_WIDTH, int SCREEN_HEIGHT, Map* map) : ZoomableGUIController(em, SCREEN_WIDTH, SCREEN_HEIGHT, map) {
		this->map = map;
		rot << 0, -1, 1, 0;
		state = EDITOR_IDLE;
		prevState = EDITOR_IDLE;
		shift = false;
		input_confirmed = false;
		input = "";
		objToPlace = NULL;
		lastCircleRad = 50;
		lastRecWidth = 100;
		lastRecHeight = 50;
		lastRot << 0, -1, 1, 0;
		selectedObject = 0;
		selectedObj = NULL;
	}

	void loadMap(Map* map) {
		this->map = map;
		zoom = std::min(Gem()->view->SCREEN_WIDTH / map->width, Gem()->view->SCREEN_HEIGHT / map->height);
		center << zoom * map->width / 2, zoom * map->height / 2;
	}

private:
	void handleSDLEvent(SDL_Event e, GeneralView* view);
	void handleMouseKeyUpEvent(SDL_Event e, GeneralView* view);
	void handleKeyDownEvent(SDL_Event e, GeneralView* view);
	void handleKeyUpEvent(SDL_Event e, GeneralView* view);
	void handleTextInputEvent(SDL_Event e, GeneralView* view);
	void handleMouseMotionEvent(SDL_Event e, GeneralView* view);

	void Notify(Event* ev) {
		GeneralView* view = Gem()->view;
		switch(ev->type) {
		case SDL_EVENT: {
			SDL_Event e = dynamic_cast<SDLEvent*>(ev)->event;
			handleSDLEvent(e, view); }
			break;
		case TICK_EVENT: {
			debug("ctrl : TickEvent");
			double oldZoom = zoom;
			zoom = zoom * std::pow(maxZoom, em->dt/1*(zoomSpeedIn - zoomSpeedOut));
			if(zoom < minZoom) zoom = minZoom;
			else if(zoom > maxZoom) zoom = maxZoom;
			Eigen::Vector2d newCenter;
			double x, y;
			double xmin = view->SCREEN_WIDTH/2. - 200;
			double xmax = zoom*map->width - view->SCREEN_WIDTH/2. + 200;
			double ymin = view->SCREEN_HEIGHT/2. - 200;
			double ymax = zoom*map->height - view->SCREEN_HEIGHT/2. + 200;
			x = center.coeff(0)*zoom/oldZoom + em->dt * (zoomSpeedRight - zoomSpeedLeft) * view->SCREEN_WIDTH;
			if(x < xmin) x = xmin;
			if(x > xmax) x = xmax;
			y = center.coeff(1)*zoom/oldZoom + em->dt * (zoomSpeedUp - zoomSpeedDown) * view->SCREEN_WIDTH;
			if(y < ymin) y = ymin;
			if(y > ymax) y = ymax;
			newCenter << x, y;
			center = newCenter;
			//std::cout << state << "\n";
			debug("ctrl : end TickEvent"); }
			break;
		}
	}
};

void MapEditorController::handleSDLEvent(SDL_Event e, GeneralView* view) {
	switch(e.type) {
	case SDL_KEYDOWN:
		handleKeyDownEvent(e, view); break;
	case SDL_KEYUP:
		handleKeyUpEvent(e, view); break;
	case SDL_TEXTINPUT:
		handleTextInputEvent(e, view); break;
	case SDL_MOUSEBUTTONUP:
		handleMouseKeyUpEvent(e, view); break;
	case SDL_MOUSEMOTION:
		handleMouseMotionEvent(e, view); break;
	}
}

void MapEditorController::handleKeyDownEvent(SDL_Event e, GeneralView* view) {
	switch(e.key.keysym.sym) {
	case SDLK_LCTRL:
		if(state == EDITOR_PLACING_RECTANGLE) {
			state = EDITOR_ROTATING_RECTANGLE;
		}
		if(state == EDITOR_PLACING_DP_ZONE) {
			state = EDITOR_ROTATING_DP_ZONE;
		}
		break;
	case SDLK_LSHIFT:
		shift = true;
		break;
	case SDLK_PLUS:
		if(!(shift || SDL_IsTextInputActive()))
		zoomSpeedIn = zoomSpeed;
		break;
	case SDLK_MINUS:
		if(!(shift || SDL_IsTextInputActive()))
		zoomSpeedOut = zoomSpeed;
		break;
	case SDLK_i:
		if(!(shift || SDL_IsTextInputActive()))
		zoomSpeedUp = zoomMoveSpeed;
		break;
	case SDLK_j:
		if(!(shift || SDL_IsTextInputActive()))
		zoomSpeedLeft = zoomMoveSpeed;
		break;
	case SDLK_k:
		if(!(shift || SDL_IsTextInputActive()))
		zoomSpeedDown = zoomMoveSpeed;
		break;
	case SDLK_l:
		if(!(shift || SDL_IsTextInputActive()))
		zoomSpeedRight = zoomMoveSpeed;
		break;
	}
}

void MapEditorController::handleKeyUpEvent(SDL_Event e, GeneralView* view) {
	switch(e.key.keysym.sym) {
	case SDLK_LSHIFT:
		shift = false;
		break;
	case SDLK_LCTRL:
		if(state == EDITOR_ROTATING_RECTANGLE) {
			state = EDITOR_PLACING_RECTANGLE;
		}
		if(state == EDITOR_ROTATING_DP_ZONE) {
			state = EDITOR_PLACING_DP_ZONE;
		}
		break;
	case SDLK_PLUS:
		zoomSpeedIn = 0.;
		break;
	case SDLK_MINUS:
		zoomSpeedOut = 0.;
		break;
	case SDLK_a: {
		if(shift) {}
		//if(state == EDITOR_IDLE && !SDL_IsTextInputActive()) {
		else {
			switch(state) {
			case EDITOR_IDLE:
				state = EDITOR_ENTERING_AUTO_WP_RAD;
				break;
			}
		}break;
	}
	case SDLK_b:
		if(shift) {}
		else {
			if(state == EDITOR_IDLE) map->toggelBorders();
		}
		break;
	case SDLK_c:
		if(shift) {}
		else {
			switch(state) {
			case EDITOR_IDLE:
				objToPlace = new MapCircle(mousePos, lastCircleRad);
				state = EDITOR_PLACING_CIRCLE;
				break;
			case EDITOR_SELECTING:
				if(selectedObj) {
					switch(selectedObj->type) {
					case MAP_CIRCLE: {
						MapCircle* ref = dynamic_cast<MapCircle*>(selectedObj);
						objToPlace = new MapCircle(ref->pos, ref->rad);
						}break;
					case MAP_BORDER:
					case MAP_RECTANGLE: {
						MapRectangle* ref = dynamic_cast<MapRectangle*>(selectedObj);
						objToPlace = new MapRectangle(ref->hl, ref->hw, ref->pos, ref->rot);
						}break;
					case MAP_DEPLOYMENT_ZONE: {
						DeploymentZone* ref = dynamic_cast<DeploymentZone*>(selectedObj);
						objToPlace = new DeploymentZone(ref->hl, ref->hw, ref->pos, ref->rot, ref->player_id);
						}break;
					case MAP_WAYPOINT: {
						MapWaypoint* ref = dynamic_cast<MapWaypoint*>(selectedObj);
						objToPlace = new MapWaypoint(ref->pos, ref->rad);
						}break;
					}
					state = EDITOR_COPYING;
					prevState = EDITOR_COPYING;
				}
				break;
			}
			break;
		}
		break;
	case SDLK_d:
		if(shift) {}
		else {
			switch(state) {
			case EDITOR_SELECTING:
				if(selectedObj) {
					map->RemoveMapObject(selectedObj);
					selectedObject++;
				}
				break;
			case EDITOR_IDLE:
				objToPlace = new DeploymentZone(lastRecWidth/2, lastRecHeight/2, mousePos, rot);
				state = EDITOR_PLACING_DP_ZONE;
				break;
			}
		}break;
	case SDLK_e:
		if(shift) {
		}
		else {
			switch(state) {
			case EDITOR_PLACING_RECTANGLE: state = EDITOR_ENTERING_REC_HEIGHT; break;
			case EDITOR_PLACING_DP_ZONE: state = EDITOR_ENTERING_DP_ZONE_HEIGHT; std::cout << "this line should not be called when pressing d\n"; break;
			}
		}
		break;
	case SDLK_h:
		help = !help;
		break;
	case SDLK_i:
		zoomSpeedUp = 0.;
		break;
	case SDLK_j:
		zoomSpeedLeft = 0.;
		break;
	case SDLK_k:
		zoomSpeedDown = 0.;
	case SDLK_l:
		if(shift) {
			state = EDITOR_LOADING;
		}
		else {
		}
		zoomSpeedRight = 0.;
		break;
	case SDLK_m:
		if(shift) {}
		else {
			switch(state) {
			case EDITOR_SELECTING:
				if(selectedObj) {
					switch(selectedObj->type) {
					case MAP_CIRCLE: {
						MapCircle* ref = dynamic_cast<MapCircle*>(selectedObj);
						objToPlace = new MapCircle(ref->pos, ref->rad);
						}break;
					case MAP_BORDER:
					case MAP_RECTANGLE: {
						MapRectangle* ref = dynamic_cast<MapRectangle*>(selectedObj);
						objToPlace = new MapRectangle(ref->hl, ref->hw, ref->pos, ref->rot);
						}break;
					case MAP_DEPLOYMENT_ZONE: {
						DeploymentZone* ref = dynamic_cast<DeploymentZone*>(selectedObj);
						objToPlace = new DeploymentZone(ref->hl, ref->hw, ref->pos, ref->rot, ref->player_id);
						}break;
					case MAP_WAYPOINT: {
						MapWaypoint* ref = dynamic_cast<MapWaypoint*>(selectedObj);
						objToPlace = new MapWaypoint(ref->pos, ref->rad);
						}break;
					}
					state = EDITOR_MOVING;
					prevState = EDITOR_MOVING;
				}
				break;
			}
			break;
		}
		break;
	case SDLK_n:
		if(shift) {
			state = EDITOR_NEWMAP;
		}
		break;
	case SDLK_p:
		if(shift) {}
		else {
			switch(state) {
			case EDITOR_IDLE:
				state = EDITOR_PATHFINDING;
				break;
			case EDITOR_PLACING_DP_ZONE:
				state = EDITOR_ENTERING_DP_ZONE_ID;
				break;
			}
		}
		break;
	case SDLK_q:
		if(shift) {
			state = EDITOR_CLOSING;
			break;
		}
		break;
	case SDLK_r:
		if(shift) {}
		else {
			switch(state) {
			case EDITOR_IDLE:
				objToPlace = new MapRectangle(lastRecWidth/2, lastRecHeight/2, mousePos, rot);
				state = EDITOR_PLACING_RECTANGLE;
				break;
			}
		}
		break;
	case SDLK_s:
		if(shift) {
			MapToJson(map);
			state = EDITOR_SAVING;
		}
		else {
			if(!SDL_IsTextInputActive()) {
				state = EDITOR_SELECTING;
			}
		}
		break;
	case SDLK_w:
		if(shift) {}
		else {
			switch(state) {
			case EDITOR_IDLE:
				objToPlace = new MapWaypoint(mousePos, lastCircleRad);
				state = EDITOR_PLACING_WP;
				break;
			case EDITOR_PLACING_CIRCLE: state = EDITOR_ENTERING_CIRCLE_RAD; break;
			case EDITOR_PLACING_RECTANGLE: state = EDITOR_ENTERING_REC_WIDTH; break;
			case EDITOR_PLACING_DP_ZONE: state = EDITOR_ENTERING_DP_ZONE_WIDTH; break;
			case EDITOR_PLACING_WP: state = EDITOR_ENTERING_WP_RAD; break;
			}
		}
		break;
	case SDLK_LEFT:
		switch(state) {
		case EDITOR_SELECTING:
			if(selectedObject <= 0) {
				selectedObject = std::max(0,(int) map->mapObjects.size() - 1);
			}
			else selectedObject--;
			break;
		}
		break;
	case SDLK_RIGHT:
		switch(state) {
		case EDITOR_SELECTING:
			if(selectedObject >= map->mapObjects.size() - 1) {
				selectedObject = 0;
			}
			else selectedObject++;
			break;
		}
		break;
	case SDLK_RETURN:
		switch(state) {
		case EDITOR_NEWMAP_WIDTH:
		case EDITOR_NEWMAP_HEIGHT:
		case EDITOR_ENTERING_CIRCLE_RAD:
		case EDITOR_ENTERING_REC_WIDTH:
		case EDITOR_ENTERING_REC_HEIGHT:
		case EDITOR_ENTERING_DP_ZONE_WIDTH:
		case EDITOR_ENTERING_DP_ZONE_HEIGHT:
		case EDITOR_ENTERING_DP_ZONE_ID:
		case EDITOR_ENTERING_WP_RAD:
		case EDITOR_ENTERING_AUTO_WP_RAD:
		case EDITOR_SAVING:
		case EDITOR_LOADING:
			input_confirmed = true;
			break;
		}
		break;
	case SDLK_ESCAPE:
		switch(state) {
		case EDITOR_ENTERING_CIRCLE_RAD: state = EDITOR_PLACING_CIRCLE; break;
		case EDITOR_ENTERING_REC_WIDTH:
		case EDITOR_ENTERING_REC_HEIGHT: state = EDITOR_PLACING_RECTANGLE; break;
		case EDITOR_ENTERING_DP_ZONE_WIDTH:
		case EDITOR_ENTERING_DP_ZONE_HEIGHT: 
		case EDITOR_ENTERING_DP_ZONE_ID:
			state = EDITOR_PLACING_DP_ZONE; break;
		case EDITOR_ENTERING_WP_RAD: state = EDITOR_PLACING_WP; break;
		case EDITOR_PLACING_CIRCLE:
		case EDITOR_PLACING_RECTANGLE:
		case EDITOR_PLACING_DP_ZONE:
		case EDITOR_PLACING_WP:
			switch(prevState) {
			case EDITOR_MOVING:
			case EDITOR_COPYING:
				state = EDITOR_SELECTING; break;
			default:
				state = EDITOR_IDLE; break;
			}
			break;
		case EDITOR_SELECTING:
		case EDITOR_NEWMAP_WIDTH:
		case EDITOR_NEWMAP_HEIGHT:
		case EDITOR_ENTERING_AUTO_WP_RAD:
		case EDITOR_SAVING:
		case EDITOR_LOADING:
			state = EDITOR_IDLE;
			break;
		}
		break;
	case SDLK_BACKSPACE:
		switch(SDL_IsTextInputActive()) {
		case true:
			if(input.size() > 0) input.pop_back();
		}
	}
}

void MapEditorController::handleMouseKeyUpEvent(SDL_Event e, GeneralView* view) {
	switch(state) {
	case EDITOR_PLACING_CIRCLE:
	case EDITOR_PLACING_RECTANGLE:
	case EDITOR_PLACING_DP_ZONE:
	case EDITOR_PLACING_WP:
		if(objToPlace) {
			switch(prevState) {
			case EDITOR_MOVING:
				map->RemoveMapObject(selectedObj);
			case EDITOR_COPYING:
				map->AddMapObject(objToPlace);
				objToPlace = NULL;
				selectedObject = map->mapObjects.size()-1;	
				state = EDITOR_SELECTING;
				break;
			default:
				map->AddMapObject(objToPlace);
				objToPlace = NULL;
				state = EDITOR_IDLE;
				break;
			}
		}
		break;
	case EDITOR_SELECTING: {
		selectedObject = 0;
		double dist;
		double minDist = 1000000;	//infinity
		for(int i = 0; i < map->mapObjects.size(); i++) {
			MapObject* currentObj = map->mapObjects.at(i);
			switch(currentObj->type) {
			case MAP_CIRCLE:
			case MAP_WAYPOINT: {
				Circle* circ = dynamic_cast<Circle*>(currentObj);
				dist = (circ->pos - mousePos).norm();
				break;}
			case MAP_BORDER:
			case MAP_RECTANGLE:
			case MAP_DEPLOYMENT_ZONE: {
				Rrectangle* rec = dynamic_cast<Rrectangle*>(currentObj);
				dist = (rec->pos - mousePos).norm();
				break;}
			}
			if(dist < minDist) {
				selectedObject = i;
				minDist = dist;
			}
		}
	}break;
	}
}

void MapEditorController::handleTextInputEvent(SDL_Event e, GeneralView* view) {
	input += e.text.text;
}

void MapEditorController::handleMouseMotionEvent(SDL_Event e, GeneralView* view) {
	Eigen::Vector2d diag; diag << view->SCREEN_WIDTH / 2., view->SCREEN_HEIGHT / 2.;
	Eigen::Vector2d pos; pos << e.button.x, view->SCREEN_HEIGHT - e.button.y;
	mousePos = (pos + center - diag) / zoom;
}

#endif