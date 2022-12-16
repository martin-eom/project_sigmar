#ifndef INPUT
#define INPUT

#include <base.h>
#include <events.h>
#include <player.h>
#include <fileio.h>

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

class KeyboardAndMouseController : public Listener {
	private:
		int SCREEN_HEIGHT;
		bool firstPointSet;
		bool queueingOrders;
		bool _inputConfirmed;
		Model* model;
		std::string _input;
		int _state;
		bool _help;
		bool _shift;
		bool _ctrl;
	public:
		std::vector<Order*> orders;
		Eigen::Vector2d p0, p1;
		Eigen::Matrix2d rot;
		bool passingThrough;
		bool help() {return _help;}
		int state() {return _state;}
		bool shift() {return _shift;}
		bool ctrl() {return _ctrl;}
		bool inputConfirmed() {return _inputConfirmed;}
		std::string input() {return _input;}
		int newUnitType;

		KeyboardAndMouseController(EventManager* em, Model* model,int SCREEN_HEIGHT) : Listener(em) {
			this->SCREEN_HEIGHT = SCREEN_HEIGHT;
			this->model = model;

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
		};
	private:
		void Notify(Event* ev) {
			GameEventManager* gem = dynamic_cast<GameEventManager*>(em);
			if(ev->type == CTRL_STATE_EVENT) {
				CtrlStateEvent* cev = dynamic_cast<CtrlStateEvent*>(ev);
				_state = cev->state;
			}
			else if(ev->type == INPUT_RECEIVED_EVENT) {
				_inputConfirmed = false;
				_input = "";
			}
			else if(ev->type == REMEMBER_ORDERS) {
				debug("Remembered Orders!");
				RememberOrders* rem = dynamic_cast<RememberOrders*>(ev);
				orders = rem->orders;
			}
			else if(ev->type == SDL_EVENT) {
				SDL_Event e = dynamic_cast<SDLEvent*>(ev)->event;
				if(e.type == SDL_MOUSEBUTTONUP) {
					int x, y;
					x = e.button.x;
					y = SCREEN_HEIGHT - e.button.y;
					switch(_state) {
					case CTRL_GIVING_ORDERS:
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
								if(passingThrough) {mo->moveType = MOVE_PASSINGTHROUGH;}
								if(!queueingOrders) {orders.clear();}
								orders.push_back(mo);
								std::cout << orders.size() << " orders queued\n";
							}
							else {
								std::cout << "First and second point are identical, can't get angel.\n";
							}
						}
						else {
							p0 << x, y;
						}
						firstPointSet = !firstPointSet;
						break;
					case CTRL_SELECTING_UNIT:
						double dist;
						double minDist = 1000000;	//infinity
						Eigen::Vector2d mouse; mouse << x, y;
						for(auto unit : model->selectedPlayer->units) {
							if(unit->placed) {
								dist = (unit->pos - mouse).norm();
								if(dist < minDist) {
									gem->model->selectedUnit = unit;
									em->Post(new RememberOrders(unit->orders));
									minDist = dist;
								}
							}
						}
					}
				}

				else if(e.type == SDL_KEYDOWN) {
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
					}
				}
				else if(e.type == SDL_KEYUP) {
					Event* nev = new Event();
					switch(e.key.keysym.sym) {
					case SDLK_RETURN:
						debug(std::to_string(_state));
						switch(_state) {
						case CTRL_IDLE:
							break;
						case CTRL_GIVING_ORDERS:
							if(model->selectedUnit) {
								if(orders.size() > 0) {
									if(queueingOrders) {
										nev = new AppendOrdersRequest(model->selectedUnit, orders);
									}
									else {
										nev = new GiveOrdersRequest(model->selectedUnit, orders);
									}
									debug(std::to_string(_state));
								}
								else {std::cout << "No orders sent, order list is empty.\n";}
							}
							else {
								std::cout << "No unit was selected. Attempting to set unit..\n";
								model->SetUnit();
							}
							debug(std::to_string(_state));
							break;
						case CTRL_LOADING:
							_inputConfirmed = true;
							break;
						case CTRL_ADDING_UNIT:
							switch(newUnitType) {
							case 0:
								em->Post(new UnitAddEvent(UNIT_INFANTRY)); break;
							case 1:
								em->Post(new UnitAddEvent(UNIT_CAVALRY)); break;
							case 2:
								em->Post(new UnitAddEvent(UNIT_MONSTER)); break;
							}
							_state = CTRL_SELECTING_UNIT;
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
							gem->model->selectedUnit = NULL;
							break;
						}break;
					case SDLK_LEFT:
						switch(_state) {
						case CTRL_SELECTING_UNIT:
							nev = new UnitSelectEvent(-1);
							break;
						}break;
					case SDLK_RIGHT:
						switch(_state) {
						case CTRL_SELECTING_UNIT:
							nev = new UnitSelectEvent(1);
							break;
						}break;
					case SDLK_UP:
						switch(_state) {
						case CTRL_SELECTING_PLAYER:
							nev = new PlayerSelectEvent(-1);
							break;
						case CTRL_ADDING_UNIT:
							newUnitType = (newUnitType - 1) % 3;
							break;
						}break;
					case SDLK_DOWN:
						switch(_state) {
						case CTRL_SELECTING_PLAYER:
							nev = new PlayerSelectEvent(1);
							break;
						case CTRL_ADDING_UNIT:
							newUnitType = (newUnitType + 1) % 3;
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
					case SDLK_l:
						if(_shift) _state = CTRL_LOADING;
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
					case SDLK_u:
						switch(_state) {
						case CTRL_IDLE:
							_state = CTRL_SELECTING_UNIT;
							break;
						}
					}
					if(nev->type != GENERIC_EVENT) {
						em->Post(nev);
					}
				}
				if(e.type == SDL_TEXTINPUT) {
					_input += e.text.text;
				}
				if(e.type == SDL_MOUSEMOTION) {
					int x, y;
					x = e.motion.x;
					y = SCREEN_HEIGHT - e.motion.y;
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
					if(_state == CTRL_GIVING_ORDERS && gem->model->selectedUnit) {
						if(!gem->model->selectedUnit->placed) {
							gem->model->selectedUnit->pos << x, y;
							gem->model->selectedUnit->rot = rot;
						}
					}
				}
			}
		}

};

enum EDITOR_STATES {
	EDITOR_IDLE,
	EDITOR_PLACING_RECTANGLE,
	EDITOR_ROTATING_RECTANGLE,
	EDITOR_ENTERING_REC_WIDTH,
	EDITOR_ENTERING_REC_HEIGHT,
	EDITOR_PLACING_CIRCLE,
	EDITOR_ENTERING_CIRCLE_RAD,
	EDITOR_NEWMAP,
	EDITOR_NEWMAP_WIDTH,
	EDITOR_NEWMAP_HEIGHT,
	EDITOR_NEWMAP_FINISH,
	EDITOR_CLOSING,
	EDITOR_SAVING,
	EDITOR_LOADING,
	EDITOR_SELECTING,
	EDITOR_MOVING,
	EDITOR_COPYING
};

class MapEditorController : public Listener {
private:
	Eigen::Vector2d p0, p1;
public:
	Map* map;
	int* SCREEN_HEIGHT;
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

	MapEditorController(EventManager* em, int* SCREEN_HEIGHT, Map* map) : Listener(em) {
		this->map = map;
		this->SCREEN_HEIGHT = SCREEN_HEIGHT;
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
		SCREEN_HEIGHT = &map->height;
	}

private:
	void Notify(Event* ev) {
		if(ev->type == SDL_EVENT) {
			SDL_Event e = dynamic_cast<SDLEvent*>(ev)->event;

			if(e.type == SDL_KEYDOWN) {
				switch(e.key.keysym.sym) {
				case SDLK_LCTRL:
					if(state == EDITOR_PLACING_RECTANGLE) {
						state = EDITOR_ROTATING_RECTANGLE;
					}
					break;
				case SDLK_LSHIFT:
					shift = true;
					break;
				case SDLK_h:
					help = true;
					break;
				}
			}
			
			if(e.type == SDL_KEYUP) {
				switch(e.key.keysym.sym) {
				case SDLK_LSHIFT:
					shift = false;
					break;
				case SDLK_LCTRL:
					if(state == EDITOR_ROTATING_RECTANGLE) {
						state = EDITOR_PLACING_RECTANGLE;
					}
					break;
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
								switch(selectedObj->type()) {
								case MAP_CIRCLE: {
									MapCircle* ref = dynamic_cast<MapCircle*>(selectedObj);
									objToPlace = new MapCircle(ref->pos, ref->rad);
									}break;
								case MAP_BORDER:
								case MAP_RECTANGLE: {
									MapRectangle* ref = dynamic_cast<MapRectangle*>(selectedObj);
									objToPlace = new MapRectangle(ref->hl, ref->hw, ref->pos, ref->rot);
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
						}
					}
				case SDLK_h:
					help = false;
					break;
				case SDLK_l:
					if(shift) {
						state = EDITOR_LOADING;
					}
					else {
						switch(state) {
						case EDITOR_PLACING_RECTANGLE: state = EDITOR_ENTERING_REC_HEIGHT; break;
						}
					}
					break;
				case SDLK_m:
					if(shift) {}
					else {
						switch(state) {
						case EDITOR_SELECTING:
							if(selectedObj) {
								switch(selectedObj->type()) {
								case MAP_CIRCLE: {
									MapCircle* ref = dynamic_cast<MapCircle*>(selectedObj);
									objToPlace = new MapCircle(ref->pos, ref->rad);
									}break;
								case MAP_BORDER:
								case MAP_RECTANGLE: {
									MapRectangle* ref = dynamic_cast<MapRectangle*>(selectedObj);
									objToPlace = new MapRectangle(ref->hl, ref->hw, ref->pos, ref->rot);
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
				case SDLK_q:
					if(shift) {
						state = EDITOR_CLOSING;
						break;
					}
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
						case EDITOR_PLACING_CIRCLE: state = EDITOR_ENTERING_CIRCLE_RAD; break;
						case EDITOR_PLACING_RECTANGLE: state = EDITOR_ENTERING_REC_WIDTH; break;
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
					case EDITOR_PLACING_CIRCLE:
					case EDITOR_PLACING_RECTANGLE:
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
			
			if(e.type == SDL_TEXTINPUT) {
				input += e.text.text;
			}
			
			if(e.type == SDL_MOUSEBUTTONUP) {
				switch(state) {
				case EDITOR_PLACING_CIRCLE:
				case EDITOR_ENTERING_CIRCLE_RAD:
				case EDITOR_PLACING_RECTANGLE:
				case EDITOR_ENTERING_REC_WIDTH:
				case EDITOR_ENTERING_REC_HEIGHT:
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
						switch(currentObj->type()) {
						case MAP_CIRCLE: {
							Circle* circ = dynamic_cast<Circle*>(currentObj);
							dist = (circ->pos - mousePos).norm();
							break;}
						case MAP_BORDER:
						case MAP_RECTANGLE: {
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
			if(e.type == SDL_MOUSEMOTION) {
				mousePos << e.button.x, *SCREEN_HEIGHT-e.button.y;
			}

		}
	}
};

#endif