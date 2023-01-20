#ifndef VIEW
#define VIEW

#include <base.h>
#include <gui_base.h>
#include <shapes.h>
#include <player.h>
#include <model.h>
#include <input.h>
#include <textures.h>

#include <SDL.h>
#include <SDL_ttf.h>
#include <stdio.h>
#include <format>
#include <list>


Color* colorBlack = new Color(0x00, 0x00, 0x00, 0xff);
Color* colorWhite = new Color(0xff, 0xff, 0xff, 0xff);
Color* colorGreen = new Color(40, 252, 3, 0xff);
Color* darkGreen = new Color(0x2e, 0x43, 0x28, 0xff);
Color* colorPurple = new Color(0xff, 0x00, 0xff, 0xff);
Color* colorRed = new Color(0xff, 0x00, 0x00, 0xff);
Color* colorBlue = new Color(0x00, 0x00, 0xff, 0xff);
Color* colorGrey = new Color(0x88, 0x88, 0x88, 0xff);
Color* darkGrey = new Color(0x50, 0x50, 0x50, 0xff);


class View : public GeneralView {
	std::string _new_map	= "shift n - new map";
	std::string _load_map	= "shift l - load map";
	std::string _quit		= "shift q - quit";
	std::string _help_line	= "h - toggle help";
	std::string _lshift		= "hold lshift  - options menu";
	std::string _player		= "p            - activate player selection";
	std::string _pArrows	= "up/down      - select player";
	std::string _pAdd		= "a            - add new player";
	std::string _pDel		= "d            - delete selected player";
	std::string _unit		= "u            - activate unit selection";
	std::string _uArrows	= "left/right   - select unit";
	std::string _uClick		= "click        - select unit";
	std::string _uAdd		= "a            - add new unit";
	std::string _utArrows	= "up/down      - select unit type";
	std::string _utEnter	= "enter        - add unit to player";
	std::string _uDel		= "d            - delete selected unit";
	std::string _order		= "o            - enter order mode";
	std::string _oClick		= "click        - choose location / orientation";
	std::string _octrl		= "hold control - append orders when creating and / or sending";
	std::string _op0		= "p            - toggle from form-up to passing-through";
	std::string _op1		= "p            - toggle from passing-through to form-up";
	std::string _oconfirm	= "enter        - send orders to selected unit";
	std::string _escape     = "esc          - aboart";
	std::string _eescape	= "esc          - de-select unit";
	std::string _zoom		= "+/-          - zoom";
	std::string _zoomMove	= "i/j/k/l      - move zoomed area";

private:
		Map* map;
		Model* model;

		SDL_Color colorText = {0xff, 0xff, 0xff};
		TTF_Font* font = TTF_OpenFont("VeraMono.ttf", 20);
		int textwidth;
		int textWindowGap = 50;

		StringTexture* textControls;	// displays control scheme for current situation
		std::string controls;
		StringTexture* textInputAdvice;	// displays text that tells people what to input into textbox
		std::string textbox;
		StringTexture* textInput;		// displays current text input
		StringTexture* objInformation;	// displays information about selected objects
		std::string info;

	public:
		View(EventManager* em, Map* map, SDL_Window* window, SDL_Renderer* renderer) : GeneralView(em, window, renderer) {
			this->map = map;
			textwidth = SCREEN_WIDTH - 2 * textWindowGap;

			textControls = new StringTexture(renderer);
			controls = "";
			textInputAdvice = new StringTexture(renderer);
			textbox = "";
			textInput = new StringTexture(renderer);
			objInformation = new StringTexture(renderer);
			info = "";
		}
	private:
		void Update() {
			GameEventManager* gem = Gem();//dynamic_cast<GameEventManager*>(em);
			auto ctrl = dynamic_cast<KeyboardAndMouseController*>(gem->ctrl);
			auto model = gem->model;
			SDL_SetRenderDrawColor(renderer, 0x00, 0x00, 0x00, 0xFF);
			SDL_RenderClear(renderer);
			//drawing map objects
			for(auto obj : map->mapObjects) {
				switch(obj->type()) {
				case MAP_CIRCLE: {
					Circle* circ = dynamic_cast<Circle*>(obj);
					DrawCircle(circ, renderer, colorPurple, SCREEN_WIDTH, SCREEN_HEIGHT, ctrl->zoom, ctrl->center);
					break;}
				case MAP_BORDER:
				case MAP_RECTANGLE: {
					Rrectangle* rec = dynamic_cast<Rrectangle*>(obj);
					DrawRectangle(rec, renderer, colorPurple, SCREEN_WIDTH, SCREEN_HEIGHT, ctrl->zoom, ctrl->center);
					break;}
				}
			}
			//drawing waypoints
			if(ddebug::_showDebugGraphics) {
				for(auto obj : map->waypoints) {
					Circle* circ = dynamic_cast<Circle*>(obj);
					DrawCircle(circ, renderer, colorGrey, SCREEN_WIDTH, SCREEN_HEIGHT, ctrl->zoom, ctrl->center);
				}
			}
			//showing tiles that hold objects
			if(ddebug::_showDebugGraphics) {
				for(auto row : map->tiles) {
					for(auto tile : row) {
						if(!tile->mapObjects.empty()) {
							Rrectangle* rec = tile->rec;
							DrawRectangle(rec, renderer, colorGreen, SCREEN_WIDTH, SCREEN_HEIGHT, ctrl->zoom, ctrl->center);
						}
					}
				}
			}
			//drawing orders when ordering
			SDL_SetRenderDrawColor(renderer, 0x88, 0x88, 0x88, 0xFF);
			if(ctrl->state() == CTRL_GIVING_ORDERS) {
				for(int i = 0; i < ctrl->orders.size(); i++) {
					Order* o = ctrl->orders.at(i);
					if(o->type == ORDER_MOVE) {
						MoveOrder* mo = dynamic_cast<MoveOrder*>(o);
						if(i > 0) {
							Order* prevo = ctrl->orders.at(i-1);
							if(prevo->type == ORDER_MOVE) {
								MoveOrder* prevmo = dynamic_cast<MoveOrder*>(prevo);
								Point p1(mo->pos); Point p2(prevmo->pos);
								DrawLine(&p1, &p2, renderer, colorGreen, SCREEN_WIDTH, SCREEN_HEIGHT, ctrl->zoom, ctrl->center);
							}
						}
					}
					if(model->selectedUnit) {
						Rrectangle rec = UnitRectangle(model->selectedUnit, i, ctrl->orders);
						DrawRectangle(&rec, renderer, colorGrey, SCREEN_WIDTH, SCREEN_HEIGHT, ctrl->zoom, ctrl->center);
					}
				}
			}
			//drawing soldiers and unit markers
			int nplayers = model->players.size();
			int nplayer = 0;
			for(auto player : model->players) {
				Color playerColor = *colorBlack;
				if(nplayers == 1) {
					playerColor.r = 0xff;
					playerColor.b = 0xff;
				}
				else {
					playerColor.r = 0xff * nplayer/(nplayers - 1);
					playerColor.b = 0xff * ((nplayers - 1) - nplayer)/(nplayers - 1);
					
				}
				for(auto unit : player->units) {
					std::vector<std::vector<Soldier*>>* soldiers = unit->soldiers();
					if(unit->placed) {
						if(ddebug::_showDebugGraphics) {
							Circle circ = Circle(unit->pos, 30);
							DrawCircle(&circ, renderer, colorGrey, SCREEN_WIDTH, SCREEN_HEIGHT, ctrl->zoom, ctrl->center);
						}
						if(model->selectedUnit) {
							if(model->selectedUnit == unit) {
								DrawUnitArrow(unit->posTarget, unit->rotTarget, renderer, colorGreen, SCREEN_WIDTH, SCREEN_HEIGHT, ctrl->zoom, ctrl->center);
								for(int i = 0; i < unit->orders.size(); i++) {
									Order* o = unit->orders.at(i);
									if(o->type == ORDER_MOVE) {
										MoveOrder* mo = dynamic_cast<MoveOrder*>(o);
										Rrectangle rec = UnitRectangle(unit, i);
										DrawRectangle(&rec, renderer, colorGreen, SCREEN_WIDTH, SCREEN_HEIGHT, ctrl->zoom, ctrl->center);
										if(i > 0) {
											Order* prevo = unit->orders.at(i-1);
											if(prevo->type == ORDER_MOVE) {
												MoveOrder* prevmo = dynamic_cast<MoveOrder*>(prevo);
												Point p1(mo->pos); Point p2(prevmo->pos);
												DrawLine(&p1, &p2, renderer, colorGreen, SCREEN_WIDTH, SCREEN_HEIGHT, ctrl->zoom, ctrl->center);
											}
										}
									}
								}
							}
						}
						for(int i = 0; i < unit->nrows(); i++) {
							for(int j = 0; j < unit->width(); j++) {
								Soldier* soldier = (*soldiers)[i][j];
								if(soldier->placed) {
									if(model->selectedUnit == unit) {
										DrawCircle(soldier->pos.coeff(0), soldier->pos.coeff(1),
											soldier->rad() + 1, renderer, colorGreen, SCREEN_WIDTH, SCREEN_HEIGHT, ctrl->zoom, ctrl->center);
									}
									DrawCircle(soldier->pos.coeff(0), soldier->pos.coeff(1),
										soldier->rad(), renderer, &playerColor, SCREEN_WIDTH, SCREEN_HEIGHT, ctrl->zoom, ctrl->center);
									if(soldier->currentOrder == soldier->unit->currentOrder) {
										DrawFacingArrowhead(soldier->pos, soldier->rot, soldier->rad(), renderer, colorPurple, SCREEN_WIDTH, SCREEN_HEIGHT, ctrl->zoom, ctrl->center);
									}
									else {
										DrawFacingArrowhead(soldier->pos, soldier->rot, soldier->rad(), renderer, colorWhite, SCREEN_WIDTH, SCREEN_HEIGHT, ctrl->zoom, ctrl->center);
									}
								}
							}
						}
					}
					if(unit == model->selectedUnit) {
						if(ctrl->state() == CTRL_GIVING_ORDERS) {
							std::vector<std::vector<Eigen::Vector2d>> posInUnit = *unit->posInUnit();
							for(int i = 0; i < unit->nrows(); i++) {
								for(int j = 0; j < unit->width(); j++) {
									if(i*unit->width() + j < unit->maxSoldiers()) {
										Soldier* soldier = soldiers->at(i).at(j);
										Eigen::Vector2d pos = ctrl->rot * posInUnit.at(i).at(j) + ctrl->p0;
										DrawCircle(pos.coeff(0), pos.coeff(1), soldier->rad(), renderer, colorGrey, SCREEN_WIDTH, SCREEN_HEIGHT, ctrl->zoom, ctrl->center);
										DrawFacingArrowhead(pos, ctrl->rot, soldier->rad(), renderer, colorWhite, SCREEN_WIDTH, SCREEN_HEIGHT, ctrl->zoom, ctrl->center);
									}
								}
							}
						}
					}
				}
				nplayer++;
			}
			// writing text
			if(ctrl->shift()) {
				controls = _load_map + "\n" + _quit;
			}
			else {
				if(ctrl->help()) {
					switch(ctrl->state()) {
					case CTRL_IDLE:
						controls = _order + "\n" + _player + "\n" + _unit + "\n" + _eescape + "\n" + _lshift; break;
					case CTRL_SELECTING_PLAYER:
						controls = _pArrows + "\n" + _pAdd + "\n" + _pDel + "\n" + _escape + "\n" + _lshift; break;
					case CTRL_SELECTING_UNIT:
						controls = _uArrows + "\n" + _uClick + "\n" + _uAdd + "\n" + _uDel + "\n" + _escape + "\n" + _lshift; break;
					case CTRL_ADDING_UNIT:
						controls = _utArrows + "\n" + _utEnter + "\n" + _escape + "\n" + _lshift; break;
					case CTRL_GIVING_ORDERS:
						controls = _oClick + "\n" + _octrl + "\n";
						switch(ctrl->passingThrough) {
						case true: controls += _op1; break;
						case false: controls += _op0; break;
						}
						controls += "\n" + _oconfirm + "\n" + _escape + "\n" + _lshift;
						break;
					}
					controls += "\n" + _zoom + "\n" + _zoomMove;
				}
				else controls = _help_line;
			}
			switch(ctrl->state()) {
			case CTRL_SELECTING_PLAYER:
				info = "Selecting player: " + std::to_string(std::find(model->players.begin(), model->players.end(), model->selectedPlayer) - model->players.begin() + 1) + "/" + std::to_string(model->players.size());
				break;
			case CTRL_SELECTING_UNIT:
				info = "Player " + std::to_string(std::find(model->players.begin(), model->players.end(), model->selectedPlayer) - model->players.begin() + 1) + " " + 
					"selecting unit: " + std::to_string(std::find(model->selectedPlayer->units.begin(), model->selectedPlayer->units.end(), model->selectedUnit) - model->selectedPlayer->units.begin() + 1)
							+ "/" + std::to_string(model->selectedPlayer->units.size());
				break;
			case CTRL_ADDING_UNIT:
				info = "New unit type: ";
				switch(ctrl->newUnitType) {
				case 0: info += "Infantry"; break;
				case 1: case -3: info += "Cavalry"; break;
				case 2: case -2: info += "Monster"; break;
				case 3: case -1: info += "Lone Rider"; break;
				}break;
			default:
				info = "";
			}
			if(info.size() > 0) {
				objInformation->loadFromString(info, font, colorText, textwidth);
				objInformation->render(textWindowGap, textWindowGap);
				objInformation->free();
			}
			textControls->loadFromString(controls, font, colorText, textwidth);
			textControls->render(textWindowGap, SCREEN_HEIGHT - textWindowGap - textControls->length());
			if(textbox.length() > 0) {
				textInputAdvice->loadFromString(textbox, font, colorText, textwidth);
				textInputAdvice->render(textWindowGap, SCREEN_HEIGHT - textWindowGap - textControls->length() - textInputAdvice->length());
				std::string input = ctrl->input();
				if(input.length() > 0) {
					textInput->loadFromString(input, font, colorText, textwidth - textInputAdvice->width());
					textInput->render(textWindowGap + textInputAdvice->width(), SCREEN_HEIGHT - textWindowGap - textControls->length()
						- textInputAdvice->length());
				}
			}

			SDL_RenderPresent(renderer);
		}
		void Notify(Event* ev) {
			if(ev->type == CHANGE_TEXTBOX_EVENT) {
				textbox = dynamic_cast<ChangeTextboxEvent*>(ev)->text;
			}
			else if(ev->type == TICK_EVENT) {
				Update();
			}
		}
};


class MapEditorView : public GeneralView {
	std::string _new_map	= "shift n - new map";
	std::string _load_map	= "shift l - load map from file";
	std::string _save_map	= "shift s - save map";
	std::string _quit		= "shift q - quit";
	std::string _lshift		= "hold lshift - options menu";
	std::string _circle		= "c           - select circle";
	std::string _crad		= "w           - change radius";
	std::string _wp			= "w           - select waypoint";
	std::string _wprad		= "w           - change radius";
	std::string _pf			= "p           - update pathfinding";
	std::string _rec		= "r           - select rectangle";
	std::string _rrot		= "hold ctrl   - rotate rectangle";
	std::string _rw			= "w           - change width";
	std::string _rh			= "e           - change height";
	std::string _pesc		= "esc         - aboart placement";
	std::string _bon		= "b           - add map borders";
	std::string _boff		= "b           - remove map borders";
	std::string _select		= "s           - select placed object";
	std::string _sarrow		= "arrow keys  - select next/prev object";
	std::string _sclick     = "M1          - select nearest object";
	std::string _smove		= "m           - move object";
	std::string _smesc		= "esc         - aboart move";
	std::string _scopy		= "c           - copy object";
	std::string _scesc		= "esc         - aboart copying";
	std::string _sdel		= "d           - delete object";
	std::string _sesc		= "esc         - aboart selection";
	std::string _help_line	= "h - toggle help";
	std::string _zoom		= "+/-         - zoom";
	std::string _zoomMove	= "i/j/k/l     - move zoomed area";
	std::string _awp		= "a           - automatically place waypoints";


	std::string _options_menu =	_new_map + "\n" + _save_map + "\n" + _load_map + "\n"+ _quit;
	std::string _control_scheme = _circle + "\n" + _rec + "\n" + _wp + "\n" + _select + "\n" + _pf + + "\n" + _awp + "\n" + _zoom + "\n" + _zoomMove + "\n" + _lshift;
	std::string _rplace = _rrot + "\n" + _rw + "\n" + _rh + "\n" + _zoom + "\n" + _zoomMove + "\n" + _pesc + "\n" + _lshift;
	std::string _cplace = _crad + "\n" + _zoom + "\n" + _zoomMove + "\n" + _pesc + "\n" + _lshift;
	std::string _select_scheme = _sarrow + "\n" + _sclick + "\n" + _smove + "\n" + _scopy + "\n" + _sdel + "\n" + _zoom + "\n" + _zoomMove + "\n" + _sesc + "\n" + _lshift;
	std::string _move_scheme =  _zoom + "\n" + _zoomMove + "\n" + _smesc + "\n" + _lshift;
	std::string _copy_scheme =  _zoom + "\n" + _zoomMove + "\n" + _scesc + "\n" + _lshift;

	SDL_Color colorText = {0xff, 0xff, 0xff};
	TTF_Font* font = TTF_OpenFont("VeraMono.ttf", 20);
	int textWindowGap = 50;

private:
	int textwidth;
	StringTexture* textControls;
	StringTexture* textInputAdvice;
	StringTexture* textInput;
	StringTexture* objDimensions;

public:
	Map* map;
	std::string textbox;

	MapEditorController* Ctrl() {
		return dynamic_cast<MapEditorController*>(Gem()->ctrl);
	}

	MapEditorView(EventManager* em, SDL_Window* window, SDL_Renderer* renderer, Map* map) : GeneralView(em, window, renderer) {
		this->map = map;
		textwidth = SCREEN_WIDTH - 2*textWindowGap;
		textbox = "";
		textControls = new StringTexture(renderer);
		textInputAdvice = new StringTexture(renderer);
		textInput = new StringTexture(renderer);
		objDimensions = new StringTexture(renderer);
	}
private:
	void Update() {
		MapEditorController* ctrl = Ctrl();
		SDL_SetRenderDrawColor(renderer, 0x00, 0x00, 0x00, 0xFF);
		SDL_RenderClear(renderer);
		Eigen::Matrix2d rot; rot << 1, 0, 0, 1;
		Eigen::Vector2d map_center; map_center << map->width / 2, map->height / 2;
		Rrectangle map_bg(map->height / 2 - 1, map->width / 2 - 1, map_center, rot);
		DrawRectangle(&map_bg, renderer, darkGrey, SCREEN_WIDTH, SCREEN_HEIGHT, ctrl->zoom, ctrl->center);
		//rendering placed objects
		for(auto object : map->mapObjects) {
			Color* objColor = colorPurple;
			if(object == ctrl->selectedObj) {
				switch(ctrl->prevState) {
				case EDITOR_MOVING:
					objColor = darkGreen; break;
				case EDITOR_COPYING:
					objColor = colorBlue; break;
				default:
					objColor = colorGreen; break;
				}
			}
			switch(object->type()) {
			case MAP_WAYPOINT:
				if(object != ctrl->selectedObj) objColor = colorGrey;
			case MAP_CIRCLE: {
				Circle* circ = dynamic_cast<Circle*>(object);
				DrawCircle(circ, renderer, objColor, SCREEN_WIDTH, SCREEN_HEIGHT, ctrl->zoom, ctrl->center);
				break;}
			case MAP_BORDER:
			case MAP_RECTANGLE: {
				Rrectangle* rec = dynamic_cast<Rrectangle*>(object);
				DrawRectangle(rec, renderer, objColor, SCREEN_WIDTH, SCREEN_HEIGHT, ctrl->zoom, ctrl->center);
				break;}
			}
		}
		for(int i = 0; i < map->wp_path_next.size(); i++) {
			for(int j = i+1; j < map->wp_path_next.at(i).size(); j++) {
				if(i != j && map->wp_path_next.at(i).at(j) == j && map->waypoints.size() > std::max(i,j)){
					Point p1(map->waypoints.at(i)->pos); Point p2(map->waypoints.at(j)->pos);
					DrawLine(&p1, &p2, renderer, colorBlue, SCREEN_WIDTH, SCREEN_HEIGHT, ctrl->zoom, ctrl->center);

				}
			}
		}
		//rendering options menu
		std::string text;
		if(ctrl->shift) text = _options_menu;
		else if(ctrl->help) {
			text = _control_scheme;
			if(ctrl->map->Borders()) text = _boff + "\n" + _control_scheme;
			else text = _bon + "\n" + _control_scheme;
			switch(ctrl->state) {
			case EDITOR_PLACING_CIRCLE:
			case EDITOR_PLACING_WP:
				text = _cplace; break;
			case EDITOR_PLACING_RECTANGLE:
				text = _rplace; break;
			case EDITOR_SELECTING:
				text = _select_scheme; break;
			case EDITOR_MOVING:
				text = _move_scheme; break;
			case EDITOR_COPYING:
				text = _copy_scheme; break;
			}
		}
		else {
			text = _help_line;
		}
		textControls->loadFromString(text, font, colorText, textwidth);
		if(textControls->texture) {
			textControls->render(textWindowGap, SCREEN_HEIGHT - textWindowGap - textControls->length());
		}
		//rendering input textbox
		if(textbox.size() > 0) {
			textInputAdvice->loadFromString(textbox, font, colorText, textwidth);
			textInputAdvice->render(textWindowGap, SCREEN_HEIGHT - textWindowGap 
				- textControls->length() - textInputAdvice->length());
			if(ctrl->input.size() > 0) {
				textInput->loadFromString(ctrl->input, font, colorText, textwidth);
				textInput->render(textWindowGap + textInputAdvice->width(), SCREEN_HEIGHT - textWindowGap 
					- textControls->length() - textInput->length());
				textInput->free();
			}
			textInputAdvice->free();
		}
		textControls->free();
		//rendering object to be placed
		if(ctrl->objToPlace) {
			switch(ctrl->objToPlace->type()) {
			case MAP_CIRCLE:
			case MAP_WAYPOINT: {
				Circle* circ = dynamic_cast<Circle*>(ctrl->objToPlace);
				DrawCircle(circ, renderer, colorGreen, SCREEN_WIDTH, SCREEN_HEIGHT, ctrl->zoom, ctrl->center);
				objDimensions->loadFromString(std::format("rectangle\nradius: {}", circ->rad), font, colorText, textwidth);
				break;}
			case MAP_BORDER:
			case MAP_RECTANGLE: {
				Rrectangle* rec = dynamic_cast<Rrectangle*>(ctrl->objToPlace);
				DrawRectangle(rec, renderer, colorGreen, SCREEN_WIDTH, SCREEN_HEIGHT, ctrl->zoom, ctrl->center);
				objDimensions->loadFromString(std::format("rectangle\nwidth: {}\nheight: {}", rec->hl*2, rec->hw*2), font, colorText, textwidth);
				break;}
			}
			if(objDimensions->texture) {
				objDimensions->render(textWindowGap, textWindowGap);
				objDimensions->free();
			}
		}
		//updating screen
		SDL_RenderPresent(renderer);
	}
	void Notify(Event* ev) {
		if(ev->type == TICK_EVENT) {
			Update();
		}
	}
};

#endif