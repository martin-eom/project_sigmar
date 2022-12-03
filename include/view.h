#ifndef VIEW
#define VIEW

#include <base.h>
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

//unsigned int colorCircle[4] = { 0x00, 0x00, 0x00, 0xff };
//unsigned int colorArrow[4] = {0xff, 0xff, 0xff, 0xff};
//unsigned int colorUnitArrow[4] = {40, 252, 3, 0xff};
//unsigned int colorPurple[4] = {0xff, 0x00, 0xff, 0xff};

Color* colorBlack = new Color(0x00, 0x00, 0x00, 0xff);
Color* colorWhite = new Color(0xff, 0xff, 0xff, 0xff);
Color* colorGreen = new Color(40, 252, 3, 0xff);
Color* colorPurple = new Color(0xff, 0x00, 0xff, 0xff);
Color* colorRed = new Color(0xff, 0x00, 0x00, 0xff);
Color* colorBlue = new Color(0x00, 0x00, 0xff, 0xff);
Color* colorGrey = new Color(0x88, 0x88, 0x88, 0xff);

class View : public Listener {
	private:
		int SCREEN_HEIGHT;
		SDL_Window* window;
		SDL_Renderer* renderer;
		Map* map;
		Model* model;
	public:
		View(EventManager* em, Map* map, Model* model, SDL_Window* window, SDL_Renderer* renderer, int SCREEN_HEIGHT) : Listener(em) {
			this->SCREEN_HEIGHT = SCREEN_HEIGHT;
			this->window = window;
			this->renderer = renderer;
			this->model = model;
			this->map = map;
		}
	private:
		void Update() {
			SDL_SetRenderDrawColor(renderer, 0x00, 0x00, 0x00, 0xFF);
			SDL_RenderClear(renderer);
			//drawing map objects
			for(auto object : map->mapObjects) {
				switch(object->type()) {
				case MAP_CIRCLE: {
					Circle* circ = dynamic_cast<Circle*>(object);
					DrawCircle(circ->pos.coeff(0), SCREEN_HEIGHT - circ->pos.coeff(1), circ->rad, renderer, colorPurple);
					break;}
				case MAP_BORDER:
				case MAP_RECTANGLE: {
					Rrectangle* rec = dynamic_cast<Rrectangle*>(object);
					DrawRectangle(rec, renderer, colorPurple, SCREEN_HEIGHT);
					break;}
				}
			}
			for(auto row : map->tiles) {
				for(auto tile : row) {
					//if(tile->circles.size() or tile->rectangles.size()) {
					if(tile->mapObjects.size()) {
						Rrectangle* rec = tile->rec;
						DrawRectangle(rec, renderer, colorGreen, SCREEN_HEIGHT);
					}
				}
			}
			//drawing soldiers and unit markers
			Node<Player*>* player = model->players.head;
			int nplayers = model->players.length();
			int nplayer = 0;
			while(player) {
				Color playerColor = *colorBlack;
				if(nplayers == 1) {
					//colorCircle[0] = 0x00;
					//colorCircle[2] = 0xff;
					playerColor.r = 0xff;
					playerColor.b = 0xff;
				}
				else {
					//colorCircle[0] = 0xff*nplayer/(nplayers-1);
					//colorCircle[2] = 0xff*((nplayers-1)-nplayer)/(nplayers-1);
					playerColor.r = 0xff * nplayer/(nplayers - 1);
					playerColor.b = 0xff * ((nplayers - 1) - nplayer)/(nplayers - 1);
					
				}
				Node<Unit*>* unitNode = player->data->units.head;
				while(unitNode) {
					Unit* unit = unitNode->data;
					std::vector<std::vector<Soldier*>>* soldiers = unit->soldiers();
					if(unit->placed) {
						if(model->selectedUnitNode) {
							if(model->selectedUnitNode->data == unit) {
								Eigen::Vector2d viewPos;
								viewPos << unit->posTarget.coeff(0), SCREEN_HEIGHT - unit->posTarget.coeff(1);
								Eigen::Matrix2d viewRot;
								viewRot << unit->rotTarget.coeff(0,0), -unit->rotTarget.coeff(0,1), 
									-unit->rotTarget.coeff(1,0), unit->rotTarget.coeff(1,1);
								DrawUnitArrow(viewPos, viewRot, renderer, colorGreen);
								for(int i = 0; i < unit->orders.size(); i++) {
									Order* o = unit->orders.at(i);
									if(o->type == ORDER_MOVE) {
										MoveOrder* mo = dynamic_cast<MoveOrder*>(o);
										Rrectangle rec = UnitRectangle(unit, i);
										DrawRectangle(&rec, renderer, colorGreen, SCREEN_HEIGHT);
										if(i > 0) {
											Order* prevo = unit->orders.at(i-1);
											if(prevo->type == ORDER_MOVE) {
												MoveOrder* prevmo = dynamic_cast<MoveOrder*>(prevo);
												SDL_RenderDrawLine(renderer, mo->pos.coeff(0), SCREEN_HEIGHT - mo->pos.coeff(1),
													prevmo->pos.coeff(0), SCREEN_HEIGHT - prevmo->pos.coeff(1));
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
									if(model->selectedUnitNode->data == unit) {
										DrawCircle(soldier->pos.coeff(0), SCREEN_HEIGHT - soldier->pos.coeff(1),
											soldier->rad() + 1, renderer, colorGreen);
										//SDL_RenderDrawLine(renderer, soldier->pos.coeff(0), SCREEN_HEIGHT - soldier->pos.coeff(1), 
										//	soldier->posTarget.coeff(0), SCREEN_HEIGHT - soldier->posTarget.coeff(1));
									}
									DrawCircle(soldier->pos.coeff(0), SCREEN_HEIGHT - soldier->pos.coeff(1),
										soldier->rad(), renderer, &playerColor);
									Eigen::Vector2d viewPos;
									viewPos << soldier->pos.coeff(0), SCREEN_HEIGHT - soldier->pos.coeff(1);
									Eigen::Matrix2d viewRot;
									viewRot << soldier->rot.coeff(0,0), -soldier->rot.coeff(0,1), -soldier->rot.coeff(1,0), soldier->rot.coeff(1,1);
									if(soldier->currentOrder == soldier->unit->currentOrder) {
										DrawFacingArrowhead(viewPos, viewRot, soldier->rad(), renderer, colorPurple);
									}
									else {
										DrawFacingArrowhead(viewPos, viewRot, soldier->rad(), renderer, colorWhite);
										/*Rrectangle rec = SoldierRectangle(soldier);
										DrawRectangle(&rec, renderer, colorPurple, SCREEN_HEIGHT);*/
									}
								}
							}
						}
					}
					unitNode = unitNode->next;
				}
				player = player->next;
				nplayer++;
			}
			SDL_RenderPresent(renderer);
		}
		void Notify(Event* ev) {
			if(ev->type == TICK_EVENT) {
				Update();
			}
		}
};


class MapEditorView : public Listener {
	std::string _new_map	= "shift n - new map";
	std::string _load_map	= "shift l - load map from file";
	std::string _save_map	= "shift s - save map";
	std::string _quit		= "shift q - quit";
	std::string _lshift		= "hold lshift - options menu";
	std::string _circle		= "c           - select circle";
	std::string _crad		= "w           - change radius";
	std::string _rec		= "r           - select rectangle";
	std::string _rrot		= "hold ctrl   - rotate rectangle";
	std::string _rw			= "w           - change width";
	std::string _rh			= "l           - change height";
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
	std::string _help_line	= "h - help";

	std::string _options_menu =	_new_map + "\n" + _save_map + "\n" + _load_map + "\n"+ _quit;
	std::string _control_scheme = _circle + "\n" + _rec + "\n" + _select + "\n" + _lshift;
	std::string _rplace = _rrot + "\n" + _rw + "\n" + _rh + "\n" + _pesc + "\n" + _lshift;
	std::string _cplace = _crad + "\n" + _pesc + "\n" + _lshift;
	std::string _select_scheme = _sarrow + "\n" + _sclick + "\n" + _smove + "\n" + _scopy + "\n" + _sdel + "\n" + _sesc + "\n" + _lshift;
	std::string _move_scheme = _smesc + "\n" + _lshift;
	std::string _copy_scheme = _scesc + "\n" + _lshift;

	SDL_Color colorText = {0xff, 0xff, 0xff};
	TTF_Font* font = TTF_OpenFont("VeraMono.ttf", 20);
	int textWindowGap = 50;

private:
	int SCREEN_WIDTH;
	int SCREEN_HEIGHT;
	int textwidth;
	SDL_Window* window;
	Map* map;
	MapEditorController* ctrl;
	StringTexture* textControls;
	StringTexture* textInputAdvice;
	StringTexture* textInput;
	StringTexture* objDimensions;

public:
	SDL_Renderer* renderer;
	std::string textbox;
	MapEditorView(EventManager* em, int width, int height, 
		SDL_Window* window, SDL_Renderer* renderer, Map* map, MapEditorController* ctrl) : Listener(em) {
		SCREEN_WIDTH = width;
		SCREEN_HEIGHT = height;
		this->window = window;
		this->renderer = renderer;
		this->map = map;
		this->ctrl = ctrl;
		textwidth = SCREEN_WIDTH - 2*textWindowGap;
		textbox = "";
		textControls = new StringTexture(renderer);
		textInputAdvice = new StringTexture(renderer);
		textInput = new StringTexture(renderer);
		objDimensions = new StringTexture(renderer);
	}
private:
	void Update() {
		SDL_SetRenderDrawColor(renderer, 0x00, 0x00, 0x00, 0xFF);
		SDL_RenderClear(renderer);
		//rendering placed objects
		for(auto object : map->mapObjects) {
			Color* objColor = colorPurple;
			if(object == ctrl->selectedObj) {
				switch(ctrl->prevState) {
				case EDITOR_MOVING:
					objColor = colorGrey; break;
				case EDITOR_COPYING:
					objColor = colorBlue; break;
				default:
					objColor = colorGreen; break;
				}
			}
			switch(object->type()) {
			case MAP_CIRCLE: {
				Circle* circ = dynamic_cast<Circle*>(object);
				DrawCircle(circ, renderer, objColor, SCREEN_HEIGHT);
				break;}
			case MAP_BORDER:
			case MAP_RECTANGLE: {
				Rrectangle* rec = dynamic_cast<Rrectangle*>(object);
				DrawRectangle(rec, renderer, objColor, SCREEN_HEIGHT);
				break;}
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
			case MAP_CIRCLE: {
				Circle* circ = dynamic_cast<Circle*>(ctrl->objToPlace);
				DrawCircle(circ, renderer, colorGreen, SCREEN_HEIGHT);
				objDimensions->loadFromString(std::format("rectangle\nradius: {}", circ->rad), font, colorText, textwidth);
				break;}
			case MAP_BORDER:
			case MAP_RECTANGLE: {
				Rrectangle* rec = dynamic_cast<Rrectangle*>(ctrl->objToPlace);
				DrawRectangle(rec, renderer, colorGreen, SCREEN_HEIGHT);
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