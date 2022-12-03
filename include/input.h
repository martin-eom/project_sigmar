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



class KeyboardAndMouseController : public Listener {
	private:
		int SCREEN_HEIGHT;
		Eigen::Vector2d p0, p1;
		Eigen::Matrix2d rot;
		bool firstPointSet;
		bool queueingOrders;
		bool passingThrough;
		Model* model;
		std::vector<Order*> orders;
	public:
		KeyboardAndMouseController(EventManager* em, Model* model,int SCREEN_HEIGHT) : Listener(em) {
			this->SCREEN_HEIGHT = SCREEN_HEIGHT;
			this->model = model;
			firstPointSet = false;
			queueingOrders = false;
			passingThrough = false;
		};
	private:
		void Notify(Event* ev) {
			if(ev->type == SDL_EVENT) {
				SDL_Event e = dynamic_cast<SDLEvent*>(ev)->event;
				if(e.type == SDL_MOUSEBUTTONUP) {
					int x, y;
					x = e.button.x;
					y = SCREEN_HEIGHT - e.button.y;
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
				}
				else if(e.type == SDL_KEYDOWN) {
					if(e.key.keysym.sym == SDLK_LSHIFT && !e.key.repeat) {
						queueingOrders = true;
						std::cout << "Queueing orders...\n";
					}
				}
				else if(e.type == SDL_KEYUP) {
					Event* nev = new Event();
					if(e.key.keysym.sym == SDLK_RETURN) {
						if(model->selectedUnitNode) {
							if(orders.size() > 0) {
								if(queueingOrders) {
									nev = new AppendOrdersRequest(model->selectedUnitNode->data, orders);
								}
								else {
									nev = new GiveOrdersRequest(model->selectedUnitNode->data, orders);
								}
							}
							else {std::cout << "No orders sent, order list is empty.\n";}
						}
						else {
							std::cout << "No unit was selected. Attempting to set unit..\n";
							model->SetUnit();
						}
					}
					else if(e.key.keysym.sym == SDLK_LEFT) {
						nev = new UnitSelectEvent(-1);
					}
					else if(e.key.keysym.sym == SDLK_RIGHT) {
						nev = new UnitSelectEvent(1);
					}
					else if(e.key.keysym.sym == SDLK_UP) {
						nev = new PlayerSelectEvent(-1);
					}
					else if(e.key.keysym.sym == SDLK_DOWN) {
						nev = new PlayerSelectEvent(1);
					}
					else if(e.key.keysym.sym == SDLK_LSHIFT) {
						queueingOrders = false;
						std::cout << "...done.\n";
					}
					else if(e.key.keysym.sym == SDLK_p) {
						passingThrough = !passingThrough;
						if(passingThrough) {std::cout << "moveType: passing through\n";}
						else {std::cout << "moveType: forming up\n";}
					}
					if(nev->type != GENERIC_EVENT) {
						em->Post(nev);
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