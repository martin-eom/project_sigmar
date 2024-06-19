#define _CRT_SECURE_NO_WARNINGS
#define NOMINMAX
#include <Windows.h>	// this line is very dangerous, moving this statement to a different location causes all sorts of problems

#include <map.h>
#include <view.h>
#include <input.h>

#include <SDL.h>
#include <stdio.h>
#include <iostream>
#include <SDL_ttf.h>
#include <string.h>
#include <fileio.h>
#include <pathfinding.h>

Uint32 CURRENT_TICK;
Uint32 T_OF_NEXT_TICK;

int initial_SCREEN_WIDTH = 1200;
int initial_SCREEN_HEIGHT = 750;
SDL_Window* window = NULL;
SDL_Renderer* renderer = NULL;

EventManager* em;
MapEditorModel* model;
Map* map;
MapEditorController* ctrl;
MapEditorView* view;

void OpenWindow(Map* new_map) {
	window = SDL_CreateWindow("Game",
	SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 
	initial_SCREEN_WIDTH, initial_SCREEN_HEIGHT, SDL_WINDOW_SHOWN);
	SDL_DisplayMode mode;
	int displayIndex = SDL_GetWindowDisplayIndex(window);
	SDL_DestroyWindow(window);
	SDL_GetDesktopDisplayMode(displayIndex, &mode);
	window = SDL_CreateWindow("Game", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
	int((&mode)->w * 0.9), int((&mode)->h * 0.9), SDL_WINDOW_SHOWN);
	if(!window) {
		std::cout << "Could not create window: " << SDL_GetError() << "\n";
	}
	renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
	if(!renderer) {
		std::cout << "Could not create renderer: " << SDL_GetError() << "\n";
	}
	em = new GameEventManager(30);
	debug("Created GameEventManager");
	model = new MapEditorModel();
	debug("Created MapEditorModel");
	model->init();
	debug("Initialized MapEditorModel");
	ctrl = new MapEditorController(em, initial_SCREEN_WIDTH, initial_SCREEN_HEIGHT, new_map);
	debug("Created MapEditorController");
	debug("zoom: " + std::to_string(ctrl->zoom));
	dynamic_cast<GameEventManager*>(em)->ctrl = ctrl;
	debug("Assigned MapEditorController to GameEventManager.");
	std::cout << ctrl << "\n" << dynamic_cast<GameEventManager*>(em)->ctrl << "\n";
	view = new MapEditorView(em, window, renderer, new_map, model);
	debug("Created MapEditorView");
	dynamic_cast<GameEventManager*>(em)->view = view;
	debug("Assigned MapEditorView to GameEventManager.");
	std::cout << view << "\n" << dynamic_cast<GameEventManager*>(em)->view << "\n";
}

void OpenWindow(int SCREEN_WIDTH, int SCREEN_HEIGHT) {
	map = new Map(SCREEN_WIDTH, SCREEN_HEIGHT);
	OpenWindow(map);
}

void CloseWindow() {
	SDL_DestroyRenderer(renderer);
	SDL_DestroyWindow(window);
	window = NULL;
	renderer = NULL;
}

void ResetTextbox(std::string text, bool input, MapEditorController* a_ctrl, MapEditorView* a_view) {
	if(input) {
		if(!SDL_IsTextInputActive()) SDL_StartTextInput();
	}
	else {
		if(SDL_IsTextInputActive()) SDL_StopTextInput();
	}
	a_ctrl->input = "";
	a_view->textbox = text;
}

bool Isint(std::string text) {
	for(auto symbol : text) {
		if(!isdigit(symbol)) return false;
	}
	return true;
}

bool Isdouble(std::string text) {
	int ndots = 0;
	for(auto symbol : text) {
		if(!isdigit(symbol)) {
			if(symbol == '.') ndots++;
			else return false;
		}
	}
	if(ndots > 1) return false;
	else return true;
}

int main(int argc, char* argv[1]) {
	AllocConsole();
	freopen("CONOUT$", "w", stdout);
	freopen("CONOUT$", "w", stderr);

	// Initializing SDL
	if(SDL_Init(SDL_INIT_VIDEO) != 0) {
		std::cout << "Failed to initialize SDL!\n";
		return -1;
	}
	if(TTF_Init() != 0) {
		std::cout << "Failed to initialize TTF!\n";
		return -1;
	}
	SDL_StopTextInput();
	
	OpenWindow(initial_SCREEN_WIDTH, initial_SCREEN_HEIGHT);
	CURRENT_TICK = SDL_GetTicks();
	T_OF_NEXT_TICK = CURRENT_TICK + em->dt * 1000;
	while (ctrl->state != EDITOR_CLOSING) {
		SDL_Event e;
		while (1) {
			CURRENT_TICK = SDL_GetTicks();
			if (CURRENT_TICK >= T_OF_NEXT_TICK) {
				while (SDL_PollEvent(&e)) {
					if (e.type == SDL_QUIT) {
						ctrl->state = EDITOR_CLOSING;
					}
					else if(e.type == SDL_KEYUP || e.type == SDL_KEYDOWN || e.type == SDL_TEXTINPUT ||e.type == SDL_MOUSEBUTTONUP || e.type == SDL_MOUSEMOTION) {
						Event* ev = new SDLEvent(e);
						em->Post(ev);
					}
				}
				TickEvent ev;
				em->Post(&ev);
				T_OF_NEXT_TICK = CURRENT_TICK + em->dt * 1000;
			}
			switch(ctrl->state) {
			case EDITOR_IDLE:
				ResetTextbox("", false, ctrl, view);
				ctrl->objToPlace = NULL;
				ctrl->selectedObj = NULL;
				break;
			case EDITOR_NEWMAP:
				ResetTextbox("Enter new map width: ", true, ctrl, view);
				ctrl->state = EDITOR_NEWMAP_WIDTH;
				break;
			case EDITOR_NEWMAP_WIDTH:
				if(ctrl->input_confirmed) {
					ctrl->input_confirmed = false;
					if(Isint(ctrl->input)) {
						ctrl->new_SCREEN_WIDTH = stoi(ctrl->input);
						ctrl->state = EDITOR_NEWMAP_HEIGHT;
						ResetTextbox("Enter new map height: ", true, ctrl, view);
					}
					else ctrl->state = EDITOR_NEWMAP;
				}
				break;
			case EDITOR_NEWMAP_HEIGHT:
				if(ctrl->input_confirmed) {
					ctrl->input_confirmed = false;
					if(Isint(ctrl->input)) {
						ctrl->new_SCREEN_HEIGHT = stoi(ctrl->input);					
						ctrl->state = EDITOR_NEWMAP_FINISH;
						ResetTextbox("", false, ctrl, view);
					}
					else ctrl->state = EDITOR_NEWMAP_WIDTH;
				}
				break;
			case EDITOR_NEWMAP_FINISH:
				ctrl->state = EDITOR_IDLE;
				CloseWindow();
				OpenWindow(ctrl->new_SCREEN_WIDTH, ctrl->new_SCREEN_HEIGHT);
				break;
			case EDITOR_PLACING_CIRCLE:
			case EDITOR_PLACING_WP: {
				ResetTextbox("", false, ctrl, view);
				dynamic_cast<Circle*>(ctrl->objToPlace)->Reposition(ctrl->mousePos);
				break;}
			case EDITOR_ENTERING_CIRCLE_RAD:
				if(!SDL_IsTextInputActive()) {
					ResetTextbox("enter new circle radius: ", true, ctrl, view);
				}
				if(ctrl->input_confirmed) {
					ctrl->input_confirmed = false;
					if(Isdouble(ctrl->input)) {
						ctrl->lastCircleRad = stod(ctrl->input);
						dynamic_cast<Circle*>(ctrl->objToPlace)->rad = stod(ctrl->input);
					}
					ctrl->state = EDITOR_PLACING_CIRCLE;
				}
				break;
			case EDITOR_ENTERING_AUTO_WP_RAD:
				if(!SDL_IsTextInputActive()) {
					ResetTextbox("enter waypoint radius: ", true, ctrl, view);
				}
				if(ctrl->input_confirmed) {
					ctrl->input_confirmed = false;
					if(Isdouble(ctrl->input)) {
						int wp_rad = stod(ctrl->input);
						AutoWaypoints(wp_rad, map);
						ctrl->state = EDITOR_IDLE;
					}
				}
				break;
			case EDITOR_ENTERING_WP_RAD:
				if(!SDL_IsTextInputActive()) {
					ResetTextbox("enter new waypoint radius: ", true, ctrl, view);
				}
				if(ctrl->input_confirmed) {
					ctrl->input_confirmed = false;
					if(Isdouble(ctrl->input)) {
						ctrl->lastCircleRad = stod(ctrl->input);
						dynamic_cast<Circle*>(ctrl->objToPlace)->rad = stod(ctrl->input);
					}
					ctrl->state = EDITOR_PLACING_WP;
				}
				break;
			case EDITOR_PLACING_RECTANGLE: {
				ResetTextbox("", false, ctrl, view);
				dynamic_cast<Rrectangle*>(ctrl->objToPlace)->Reposition(ctrl->mousePos);
				break;}
			case EDITOR_ROTATING_RECTANGLE: {
				ResetTextbox("", false, ctrl, view);
				Rrectangle* rec = dynamic_cast<Rrectangle*>(ctrl->objToPlace);
				Eigen::Vector2d diff = ctrl->mousePos - rec->pos;
				if(diff.coeff(0) != 0 || diff.coeff(1) != 0) {
					double cos = diff.coeff(0) / diff.norm();
					double sin = diff.coeff(1) / diff.norm();
					Eigen::Matrix2d rot; rot << cos, -sin, sin, cos;
					rec->Rotate(rot);
				}
				break;}
			case EDITOR_ENTERING_REC_WIDTH:
				if(!SDL_IsTextInputActive()) {
					ResetTextbox("enter new rectangle width: ", true, ctrl, view);
				}
				if(ctrl->input_confirmed) {
					ctrl->input_confirmed = false;
					if(Isdouble(ctrl->input)) {
						ctrl->lastRecWidth = stod(ctrl->input);
						Rrectangle* rec = dynamic_cast<Rrectangle*>(ctrl->objToPlace);
						rec->Reshape(stod(ctrl->input)/2, rec->hw);
					}
					ctrl->state = EDITOR_PLACING_RECTANGLE;
				}
				break;
			case EDITOR_ENTERING_REC_HEIGHT:
				if(!SDL_IsTextInputActive()) {
					ResetTextbox("enter new rectangle height: ", true, ctrl, view);
				}
				if(ctrl->input_confirmed) {
					ctrl->input_confirmed = false;
					if(Isdouble(ctrl->input)) {
						ctrl->lastRecHeight = stod(ctrl->input);
						Rrectangle* rec = dynamic_cast<Rrectangle*>(ctrl->objToPlace);
						rec->Reshape(rec->hl, stod(ctrl->input)/2);
					}
					ctrl->state = EDITOR_PLACING_RECTANGLE;
				}
				break;
			case EDITOR_SELECTING:
				ctrl->objToPlace = NULL;
				ctrl->prevState = 0;
				if(ctrl->selectedObject >= ctrl->map->mapObjects.size()) ctrl->selectedObject = 0;
				if(ctrl->map->mapObjects.size() > 0) ctrl->selectedObj = ctrl->map->mapObjects.at(ctrl->selectedObject);
				else ctrl->selectedObj = NULL;
				break;
			case EDITOR_MOVING:
				if(ctrl->objToPlace) {
					switch(ctrl->objToPlace->type()) {
					case MAP_CIRCLE:
						ctrl->state = EDITOR_PLACING_CIRCLE; break;
					case MAP_RECTANGLE:
						ctrl->state = EDITOR_PLACING_RECTANGLE; break;
					case MAP_WAYPOINT:
						ctrl->state = EDITOR_PLACING_WP; break;
					default:
						ctrl->state = EDITOR_SELECTING;
						ctrl->prevState = 0;
					}
				}
				break;
			case EDITOR_COPYING:
				if(ctrl->objToPlace) {
					switch(ctrl->objToPlace->type()) {
					case MAP_CIRCLE:
						ctrl->state = EDITOR_PLACING_CIRCLE; break;
					case MAP_RECTANGLE:
						ctrl->state = EDITOR_PLACING_RECTANGLE; break;
					case MAP_WAYPOINT:
						ctrl->state = EDITOR_PLACING_WP; break;
					default:
						ctrl->state = EDITOR_SELECTING;
						ctrl->prevState = 0;
					}
				}
				break;
			case EDITOR_PATHFINDING:
				FloydWarshallWithPathReconstruction(map);
				ctrl->state = EDITOR_IDLE;
				break;
			case EDITOR_SAVING:
				if(!SDL_IsTextInputActive()) {
					ResetTextbox("Saving map to json. Enter file name: ", true, ctrl, view);
				}
				if(ctrl->input_confirmed) {
					ctrl->input_confirmed = false;
					FloydWarshallWithPathReconstruction(map);
					std::ofstream out("maps/" + ctrl->input);
					out << std::setw(4) << MapToJson(ctrl->map) << "\n";
					ctrl->state = EDITOR_IDLE;
				}
				break;
			case EDITOR_LOADING:
				if(!SDL_IsTextInputActive()) {
					ResetTextbox("Loading map. Enter file name: ", true, ctrl, view);
				}
				if(ctrl->input_confirmed) {
					ctrl->input_confirmed = false;
					map = new Map("maps/" + ctrl->input);
					ctrl->state = EDITOR_IDLE;
					CloseWindow();
					OpenWindow(map);
				}
				break;
			case EDITOR_CLOSING:
				CloseWindow();
				return true;
			}
		}
		CloseWindow();
		return true;

	}
	SDL_StopTextInput();
	SDL_Quit();
}
