#ifndef VIEW
#define VIEW

#include <base.h>
#include <gui_base.h>
#include <shapes.h>
#include <player.h>
#include <model.h>
#include <input.h>
#include <textures.h>
#include <animations.h>

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
Color* colorOrange = new Color(0xff, 0xa5, 0x00, 0x99);


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
	std::string _oClick		= "m2           - choose location / orientation\n"
							  "m1           - attack target";
	std::string _octrl		= "hold control - append orders when creating and / or sending";
	std::string _op0		= "p            - toggle from form-up to passing-through";
	std::string _op1		= "p            - toggle from passing-through to form-up";
	std::string _submit		= "s            - submit orders and continue simulation";
	std::string _oconfirm	= "enter        - send orders to selected unit";
	std::string _escape     = "esc          - aboart";
	std::string _eescape	= "esc          - de-select unit";
	std::string _zoom		= "+/-          - zoom";
	std::string _zoomMove	= "i/j/k/l      - move zoomed area";

	std::string _p1Order = "Game paused:\nPlayer 1 can give orders!";
	std::string _p2Order = "Game paused:\nPlayer 2 can give orders!";
	std::string _running =               "Simulation running...    ";

private:
		Model* model;
		Map* map;

		SDL_Color colorText = {0xff, 0xff, 0xff};
		TTF_Font* font = TTF_OpenFont("VeraMono.ttf", 20);
		TTF_Font* fontLarge = TTF_OpenFont("VeraMono.ttf", 30);
		
		int textwidth;
		int textWindowGap = 50;

		StringTexture* textControls;	// displays control scheme for current situation
		std::string controls;
		StringTexture* textInputAdvice;	// displays text that tells people what to input into textbox
		std::string textbox;
		StringTexture* textInput;		// displays current text input
		StringTexture* objInformation;	// displays information about selected objects
		std::string info;
		StringTexture* gameInstructions; // displays game instructions (e.g. who's phase to give orders it is)
		std::string gameState;

		ImgTexture* objCircle;
		ImgTexture* token;
		ImgTexture* damage;
		ImgTexture* backgroundTexture;
		Animation* background;
		std::map<std::string, ImgTexture*> blueLegTextures;
		std::map<std::string, ImgTexture*> redLegTextures;
		std::vector<LegAnimation*> legs;
		std::map<std::string, ImgTexture*> blueBodyTextures;
		std::map<std::string, ImgTexture*> redBodyTextures;
		std::vector<SoldierAnimation*> bodies;
		std::map<std::string, ImgTexture*> blueMeleeTextures;
		std::map<std::string, ImgTexture*> redMeleeTextures;
		std::vector<MeleeAnimation*> melee;
		std::map<std::string, ImgTexture*> blueRangedTextures;
		std::map<std::string, ImgTexture*> redRangedTextures;
		std::vector<RangedAnimation*> ranged;
		std::vector<DamageAnimation*> damages;
		std::map<std::string, ImgTexture*> projectileTextures;
		std::vector<ProjectileAnimation*> projectiles;

	public:
		void loadBackground();
		void loadTextures();
		void loadDamageTexture();
		void createAnimations();
		void animateBackground(Animation* anime, ZoomableGUIController* ctrl);
		void animateSelectionCircle(SoldierAnimation* anime, ZoomableGUIController* ctrl);
		void animateSoldier(SoldierAnimation* anime, ZoomableGUIController* ctrl, bool hpAlpha = true);
		void animateProjectile(ProjectileAnimation* anime, ZoomableGUIController* ctrl);
		void drawMapObjects(KeyboardAndMouseController* ctrl, Model* model);
		void drawTileObjectCollision(KeyboardAndMouseController* ctrl);
		void drawProposedOrders1(KeyboardAndMouseController* ctrl, Model* model);
		void drawProposedOrders2(KeyboardAndMouseController* ctrl, Model* model, Unit* unit);
		void drawCurrentOrders(KeyboardAndMouseController* ctrl, Model* model, Unit* unit);
		void drawOrders(KeyboardAndMouseController* ctrl, Model* model);
		void drawGameObjects(KeyboardAndMouseController* ctrl);
		void drawUI(KeyboardAndMouseController* ctrl, Model* model);

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
			gameInstructions = new StringTexture(renderer);
			gameState = "";

			objCircle = new ImgTexture(renderer);
			objCircle->loadFromImage("textures/circle_purple.bmp");
			token = new ImgTexture(renderer);
			token->loadFromImage("textures/circles.bmp");

			GameEventManager* gem = Gem();
			model = gem->model;
			loadBackground();
			loadTextures();
			loadDamageTexture();
		}
	private:
		void Update() {
			GameEventManager* gem = Gem();
			auto ctrl = dynamic_cast<KeyboardAndMouseController*>(gem->ctrl);
			auto model = gem->model;
			SDL_SetRenderDrawColor(renderer, 0x00, 0x00, 0x00, 0xFF);
			SDL_RenderClear(renderer);

			animateBackground(background, ctrl);
			drawMapObjects(ctrl, model);
			drawTileObjectCollision(ctrl);
			drawProposedOrders1(ctrl, model);	
			drawOrders(ctrl, model);
			drawGameObjects(ctrl);
			drawUI(ctrl, model);

			SDL_RenderPresent(renderer);
		}

		void Notify(Event* ev) {
			if(ev->type == CHANGE_TEXTBOX_EVENT) {
				textbox = dynamic_cast<ChangeTextboxEvent*>(ev)->text;
			}
			else if(ev->type == UNIT_ROSTER_MODIFIED_EVENT) {
				debug("Unit roster was modified. Generating new animations...");
				legs.clear();
				melee.clear();
				ranged.clear();
				bodies.clear();
				damages.clear();
				projectiles.clear();
				debug("Cleared old animations.");
				createAnimations();
			}
			else if(ev->type == PROJECTILE_SPAWN_EVENT) {
				Projectile* projectile = dynamic_cast<ProjectileSpawnEvent*>(ev)->p;
				AnimationInformation info = model->SoldierTypes.at(projectile->soldierType).anime_projectile_information;
				ProjectileAnimation* anime = new ProjectileAnimation(projectile, info);
				anime->texture = projectileTextures.at(projectile->soldierType);
				projectiles.push_back(anime);
			}
			else if(ev->type == TICK_EVENT) {
				debug("TickEvent: view - begin");
				Update();
				debug("TickEvent: view - end");
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
	std::string _dpZone		= "d           - select deployment zone";
	std::string _dzPlayer	= "p           - set deployment zone player number";
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
	std::string _control_scheme = _circle + "\n" + _rec + "\n" + _dpZone + "\n" + _wp + "\n" + _select + "\n" + _pf + + "\n" + _awp + "\n" + _zoom + "\n" + _zoomMove + "\n" + _lshift;
	std::string _rplace = _rrot + "\n" + _rw + "\n" + _rh + "\n" + _zoom + "\n" + _zoomMove + "\n" + _pesc + "\n" + _lshift;
	std::string _dzplace = _rplace + "\n" + _dzPlayer;
	std::string _cplace = _crad + "\n" + _zoom + "\n" + _zoomMove + "\n" + _pesc + "\n" + _lshift;
	std::string _select_scheme = _sarrow + "\n" + _sclick + "\n" + _smove + "\n" + _scopy + "\n" + _sdel + "\n" + _zoom + "\n" + _zoomMove + "\n" + _sesc + "\n" + _lshift;
	std::string _move_scheme =  _zoom + "\n" + _zoomMove + "\n" + _smesc + "\n" + _lshift;
	std::string _copy_scheme =  _zoom + "\n" + _zoomMove + "\n" + _scesc + "\n" + _lshift;

	SDL_Color colorText = {0xff, 0xff, 0xff};
	TTF_Font* font = TTF_OpenFont("VeraMono.ttf", 20);
	int textWindowGap = 50;

private:
	MapEditorModel* model;

	int textwidth;
	StringTexture* textControls;
	StringTexture* textInputAdvice;
	StringTexture* textInput;
	StringTexture* objDimensions;
	ImgTexture* backgroundTexture;
	Animation* background;

	void drawPlacedObjects(MapEditorController* ctrl);
	void drawMenu(MapEditorController* ctrl);
	void drawInputTextbox(MapEditorController* ctrl);
	void drawPlacingObject(MapEditorController* ctrl);

public:
	Map* map;
	std::string textbox;

	void loadBackground();
	void animateBackground(Animation* anime, ZoomableGUIController* ctrl);

	MapEditorController* Ctrl() {
		return dynamic_cast<MapEditorController*>(Gem()->ctrl);
	}

	MapEditorView(EventManager* em, SDL_Window* window, SDL_Renderer* renderer, Map* map, MapEditorModel* model) : GeneralView(em, window, renderer) {
		this->map = map;
		textwidth = SCREEN_WIDTH - 2*textWindowGap;
		textbox = "";
		textControls = new StringTexture(renderer);
		textInputAdvice = new StringTexture(renderer);
		textInput = new StringTexture(renderer);
		objDimensions = new StringTexture(renderer);
		this->model = model;
		loadBackground();
	}
private:
	void Update() {
		MapEditorController* ctrl = Ctrl();
		SDL_SetRenderDrawColor(renderer, 0x00, 0x00, 0x00, 0xFF);
		SDL_RenderClear(renderer);
		animateBackground(background, ctrl);
		Eigen::Matrix2d rot; rot << 1, 0, 0, 1;
		Eigen::Vector2d map_center; map_center << map->width / 2, map->height / 2;
		Rrectangle map_bg(map->height / 2 - 1, map->width / 2 - 1, map_center, rot);
		DrawRectangle(&map_bg, renderer, darkGrey, SCREEN_WIDTH, SCREEN_HEIGHT, ctrl->zoom, ctrl->center);

		drawPlacedObjects(ctrl);
		drawMenu(ctrl);
		drawInputTextbox(ctrl);
		drawPlacingObject(ctrl);

		SDL_RenderPresent(renderer);
	}
	void Notify(Event* ev) {
		if(ev->type == TICK_EVENT) {
			Update();
		}
	}
};


void View::createAnimations() {
	for(auto player : Gem()->model->players) {
		debug("counted a player");
		for(auto unit : player->units) {
			for(auto row : unit->soldiers) {
				for(auto soldier : row) {
					LegAnimation* leg = new LegAnimation(soldier, model->SoldierTypes.at(soldier->tag).anime_legs_information);
					if(player->player1)
						leg->texture = blueLegTextures.at(soldier->tag);
					else
						leg->texture = redLegTextures.at(soldier->tag);
					legs.push_back(leg);
					if(soldier->melee) {
						MeleeAnimation* mele = new MeleeAnimation(soldier, model->SoldierTypes.at(soldier->tag).anime_melee_information);
						if(player->player1)
							mele->texture = blueMeleeTextures.at(soldier->tag);
						else
							mele->texture = redMeleeTextures.at(soldier->tag);
						melee.push_back(mele);
					}
					if(soldier->ranged) {
						RangedAnimation* range = new RangedAnimation(soldier, model->SoldierTypes.at(soldier->tag).anime_ranged_information);
						if(player->player1)
							range->texture = blueRangedTextures.at(soldier->tag);
						else
							range->texture = redRangedTextures.at(soldier->tag);
						ranged.push_back(range);
					}
					SoldierAnimation* body = new SoldierAnimation(soldier, model->SoldierTypes.at(soldier->tag).anime_body_information);
					if(player->player1)
						body->texture = blueBodyTextures.at(soldier->tag);
					else
						body->texture = redBodyTextures.at(soldier->tag);
					bodies.push_back(body);
					DamageAnimation* dmg = new DamageAnimation(soldier, model->settings.damageInfo);
					dmg->texture = damage;
					damages.push_back(dmg);
				}
			}
		}
	}
	debug("bodies now contains " + std::to_string(bodies.size()) + " animations!\n");
}

void View::drawMapObjects(KeyboardAndMouseController* ctrl, Model* model) {
	if(model->settings.show_map_object_outlines) {
		for(auto obj : map->mapObjects) {
			switch(obj->type) {
			case MAP_CIRCLE: {
				Circle* circ = dynamic_cast<Circle*>(obj);
				objCircle->renderZoomed(circ->pos.coeff(0), circ->pos.coeff(1), circ->rad, 32, 32, 0, 0,
					SCREEN_WIDTH, SCREEN_HEIGHT, ctrl->zoom, ctrl->center);
				break;}
			case MAP_BORDER:
			case MAP_RECTANGLE: {
				Rrectangle* rec = dynamic_cast<Rrectangle*>(obj);
				DrawRectangle(rec, renderer, colorPurple, SCREEN_WIDTH, SCREEN_HEIGHT, ctrl->zoom, ctrl->center);
				break;}
			}
		}
	}
	for(auto obj : map->deploymentZones) {
		Color* drawColor = colorRed;
		if(obj->player_id == 0) drawColor = colorBlue;
		Rrectangle* rec = dynamic_cast<Rrectangle*>(obj);
		DrawRectangle(rec, renderer, drawColor, SCREEN_WIDTH, SCREEN_HEIGHT, ctrl->zoom, ctrl->center);
	}
	if(ddebug::_showDebugGraphics) {
		for(auto obj : map->waypoints) {
			Circle* circ = dynamic_cast<Circle*>(obj);
			//DrawCircle(circ, renderer, colorGrey, SCREEN_WIDTH, SCREEN_HEIGHT, ctrl->zoom, ctrl->center);
		}
	}
}

void View::drawTileObjectCollision(KeyboardAndMouseController* ctrl) {
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
}

void View::drawProposedOrders1(KeyboardAndMouseController* ctrl, Model* model) {
	
	auto drawOrderList = [&](std::vector<Order*> orders, Unit* unit, Color* drawColor) {
		for(int n_order = 0; n_order < orders.size(); n_order++) {
			Order* o = orders.at(n_order);
			if(o->type == ORDER_MOVE) {
				MoveOrder* mo = dynamic_cast<MoveOrder*>(o);
				if(n_order > 0) {
					Order* prevo = orders.at(n_order-1);
					if(prevo->type == ORDER_MOVE) {
						MoveOrder* prevmo = dynamic_cast<MoveOrder*>(prevo);
						Point p1(mo->pos); Point p2(prevmo->pos);
						DrawLine(&p1, &p2, renderer, drawColor, SCREEN_WIDTH, SCREEN_HEIGHT, ctrl->zoom, ctrl->center);
					}
				}
			}
			Color* recColor = drawColor;
			Rrectangle rec = UnitRectangle(unit, n_order, orders);
			if(o->type == ORDER_ATTACK) {
				AttackOrder* ao = dynamic_cast<AttackOrder*>(o);
				rec = UnitRectangle(ao->target, ao->target->currentOrder);
				recColor = colorOrange;
			}
			DrawRectangle(&rec, renderer, recColor, SCREEN_WIDTH, SCREEN_HEIGHT, ctrl->zoom, ctrl->center);
		}
	};

	SDL_SetRenderDrawColor(renderer, 0x88, 0x88, 0x88, 0xFF);
	if(ctrl->state() == CTRL_GIVING_ORDERS && ctrl->selectedUnit) {
		drawOrderList(ctrl->orders, ctrl->selectedUnit, colorGrey);
	}
	if(model->state == MODEL_GAME_PAUSED) {
		for(int n_unit = 0; n_unit < ctrl->selectedPlayer->units.size(); n_unit++) {
			Color* drawColor;
			if(ctrl->selectedPlayer->units.at(n_unit) == ctrl->selectedUnit)
				drawColor = colorGreen;
			else {
				drawColor = colorRed;
				if(ctrl->selectedPlayer == model->player1)
					drawColor = colorBlue;
			}
			Unit* unit = ctrl->selectedPlayer->units.at(n_unit);
			std::vector<Order*> orders = ctrl->orderList.at(n_unit);
			drawOrderList(orders, unit, drawColor);
		}
	}			
	debug("view : drew orders when ordering");
}

void View::drawProposedOrders2(KeyboardAndMouseController* ctrl, Model* model, Unit* unit) {
	/* Draws rotating unit model when deciding unit rotation.
	*/
	if(unit == ctrl->selectedUnit) {
		std::vector<std::vector<Soldier*>>* soldiers = &(unit->soldiers);
		if(ctrl->state() == CTRL_GIVING_ORDERS) {
			std::vector<std::vector<Eigen::Vector2d>> posInUnit = unit->posInUnit;
			for(int i = 0; i < unit->nrows; i++) {
				for(int j = 0; j < unit->ncols; j++) {
					if(i*unit->ncols + j < unit->maxSoldiers) {
						Soldier* soldier = soldiers->at(i).at(j);
						if(soldier->alive) {
							Eigen::Vector2d pos = ctrl->rot * posInUnit.at(i).at(j) + ctrl->p0;
							DrawCircle(pos.coeff(0), pos.coeff(1), soldier->rad, renderer, colorGrey, SCREEN_WIDTH, SCREEN_HEIGHT, ctrl->zoom, ctrl->center);
							DrawFacingArrowhead(pos, ctrl->rot, soldier->rad, renderer, colorWhite, SCREEN_WIDTH, SCREEN_HEIGHT, ctrl->zoom, ctrl->center);
						}
					}
				}
				debug("but maybe this one is");
			}
			debug("nope");
		}
	}
}

void View::drawCurrentOrders(KeyboardAndMouseController* ctrl, Model* model, Unit* unit) {
	std::vector<std::vector<Soldier*>>* soldiers = &(unit->soldiers);
	GameEventManager* gem = Gem();
	if(unit->placed) {
		if(ddebug::_showDebugGraphics || true) {
			Circle circ = Circle(unit->pos, 30);
			//DrawCircle(&circ, renderer, colorGrey, SCREEN_WIDTH, SCREEN_HEIGHT, ctrl->zoom, ctrl->center);
			if(unit->rangedTarget && unit == ctrl->selectedUnit) {
				Rrectangle rec = OnSpotUnitRectangle(unit->rangedTarget);//, unit->rangedTarget->currentOrder);
				DrawRectangle(&rec, renderer, colorPurple, SCREEN_WIDTH, SCREEN_HEIGHT, ctrl->zoom, ctrl->center);
			}
		}
						
		if(ctrl->selectedUnit) {
			if(ctrl->selectedUnit == unit) {
				// draw unit order path
				DrawUnitArrow(unit->posTarget, unit->rotTarget, renderer, colorGreen, SCREEN_WIDTH, SCREEN_HEIGHT, ctrl->zoom, ctrl->center);
				debug(std::to_string(unit->orders.size()));
				for(int i = 0; i < unit->orders.size(); i++) {
					Order* o = unit->orders.at(i);
					if(o->type == ORDER_MOVE) {
						Rrectangle rec = UnitRectangle(unit, i);
						DrawRectangle(&rec, renderer, colorGreen, SCREEN_WIDTH, SCREEN_HEIGHT, ctrl->zoom, ctrl->center);
					}
					else if(o->type == ORDER_ATTACK) {
						AttackOrder* ao = dynamic_cast<AttackOrder*>(o);
						Rrectangle rec = UnitRectangle(ao->target, ao->target->currentOrder);
						DrawRectangle(&rec, renderer, colorOrange, SCREEN_WIDTH, SCREEN_HEIGHT, ctrl->zoom, ctrl->center);
					}
					else if(o->type == ORDER_TARGET) {
						TargetOrder* to = dynamic_cast<TargetOrder*>(o);
						Rrectangle rec = UnitRectangle(to->target, to->target->currentOrder);
						DrawRectangle(&rec, renderer, colorOrange, SCREEN_WIDTH, SCREEN_HEIGHT, ctrl->zoom, ctrl->center);
					}
					if(i > 0) {
						Order* prevo = unit->orders.at(i-1);
						Point p1(o->pos); Point p2(prevo->pos);
						DrawLine(&p1, &p2, renderer, colorGreen, SCREEN_WIDTH, SCREEN_HEIGHT, ctrl->zoom, ctrl->center);
					}
					debug("this loop might be infinite");
				}
				debug("or not");
			}
		}
						
	}
}

void View::drawOrders(KeyboardAndMouseController* ctrl, Model* model) {
	for(auto player : model->players) {
		for(auto unit : player->units) {
			drawCurrentOrders(ctrl, model, unit);
			drawProposedOrders2(ctrl, model, unit);
		}
	}
}

void View::drawGameObjects(KeyboardAndMouseController* ctrl) {
	// drawing green circles under soldiers of selected unit
	if(ctrl->selectedUnit) {
		for(auto anime : legs) {			
			if(ctrl->selectedUnit == anime->soldier->unit && anime->soldier->alive) {
				animateSelectionCircle(anime, ctrl);
			}
		}
	}
	for(auto anime : legs) {
		animateSoldier(anime, ctrl);
	}
	for(auto anime : melee) {
		animateSoldier(anime, ctrl);
	}
	for(auto anime : bodies) {
		animateSoldier(anime, ctrl);
	}
	for(auto anime : ranged) {
		animateSoldier(anime, ctrl);
	}
	for(auto anime : damages) {
		animateSoldier(anime, ctrl, false);
	}
	for(auto anime : projectiles) {
		animateProjectile(anime, ctrl);
		if(anime->projectile->dead) {
			ProjectileAnimation* tempProj = anime;
			std::erase(projectiles, anime);
			//delete tempProj;	///////// VERY IMPORTANT
		}
	}
	for(auto anime : bodies) {
		Soldier* soldier = anime->soldier;
		if(soldier->placed) {
			if(soldier->debugFlag1) {
				DrawCircle(soldier->pos.coeff(0), soldier->pos.coeff(1), soldier->rad, renderer, colorOrange, SCREEN_WIDTH, SCREEN_HEIGHT, ctrl->zoom, ctrl->center);
			}
			if(soldier->debugFlag2) {
				DrawCircle(soldier->pos.coeff(0), soldier->pos.coeff(1), soldier->rad, renderer, colorWhite, SCREEN_WIDTH, SCREEN_HEIGHT, ctrl->zoom, ctrl->center);
			}
			if(soldier->debugFlag3) {
				DrawCircle(soldier->pos.coeff(0), soldier->pos.coeff(1), soldier->rad, renderer, darkGreen, SCREEN_WIDTH, SCREEN_HEIGHT, ctrl->zoom, ctrl->center);
			}

		}
	}
}

void View::drawUI(KeyboardAndMouseController* ctrl, Model* model) {
	if(ctrl->shift()) {
		controls = _load_map + "\n" + _quit;
	}
	else {
		if(ctrl->help()) {
			switch(ctrl->state()) {
			case CTRL_IDLE:
				controls = _order + "\n" + _player + "\n" + _unit + "\n" + _submit + "\n" + _eescape + "\n" + _lshift; break;
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
		info = "Selecting player: " + std::to_string(std::find(model->players.begin(), model->players.end(), ctrl->selectedPlayer) - model->players.begin() + 1) + "/" + std::to_string(model->players.size());
		break;
	case CTRL_SELECTING_UNIT:
		info = "Player " + std::to_string(std::find(model->players.begin(), model->players.end(), ctrl->selectedPlayer) - model->players.begin() + 1) + " " + 
			"selecting unit: " + std::to_string(std::find(ctrl->selectedPlayer->units.begin(), ctrl->selectedPlayer->units.end(), ctrl->selectedUnit) - ctrl->selectedPlayer->units.begin() + 1)
					+ "/" + std::to_string(ctrl->selectedPlayer->units.size());
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
	switch(model->state) {
	//case MODEL_SIMULATION:
	//	gameState = ""; break;
	case MODEL_GAME_RUNNING:
		gameState = _running; break;
	case MODEL_GAME_PAUSED:
		if(ctrl->selectedPlayer == model->player1) gameState = _p1Order;
		else gameState = _p2Order;
		break;
	case MODEL_GAME_OVER:
		gameState = model->result; break;
	}
	switch(model->state) {
	case MODEL_GAME_RUNNING:
	case MODEL_GAME_PAUSED:
	case MODEL_GAME_OVER:
		gameInstructions->loadFromString(gameState, fontLarge, colorText, textwidth);
		gameInstructions->render(SCREEN_WIDTH - textWindowGap - 500, textWindowGap);
		gameInstructions->free();
		break;
	}
}



void View::loadBackground() {
	if(model->settings.custom_background) {
		background = new Animation(new Point(), model->settings.backgroundInfo);
		backgroundTexture = new ImgTexture(renderer);
		backgroundTexture->loadFromImage(("textures/" + model->settings.backgroundInfo.texture).c_str());
		background->texture = backgroundTexture;
		//std::cout << "################ " << ("textures/" + model->settings.backgroundInfo.texture).c_str();
	}
}

void MapEditorView::loadBackground() {
	if(model->settings.custom_background) {
		background = new Animation(new Point(), model->settings.backgroundInfo);
		backgroundTexture = new ImgTexture(renderer);
		backgroundTexture->loadFromImage(("textures/" + model->settings.backgroundInfo.texture).c_str());
		background->texture = backgroundTexture;
	}
}

void View::loadTextures() {
	std::map<std::string, SoldierInformation>::iterator it;
	for(it = model->SoldierTypes.begin(); it!=model->SoldierTypes.end(); it++) {
		ImgTexture* blueLeg = new ImgTexture(renderer);
		ImgTexture* redLeg = new ImgTexture(renderer);
		blueLeg->loadFromImage(("textures/" + it->second.anime_legs_information.textureBlue).c_str());
		redLeg->loadFromImage(("textures/" + it->second.anime_legs_information.textureRed).c_str());
		blueLegTextures.emplace(it->first, blueLeg);
		redLegTextures.emplace(it->first, redLeg);
		ImgTexture* blueMelee = new ImgTexture(renderer);
		ImgTexture* redMelee = new ImgTexture(renderer);
		blueMelee->loadFromImage(("textures/" + it->second.anime_melee_information.textureBlue).c_str());
		redMelee->loadFromImage(("textures/" + it->second.anime_melee_information.textureRed).c_str());
		blueMeleeTextures.emplace(it->first, blueMelee);
		redMeleeTextures.emplace(it->first, redMelee);
		if(it->second.ranged_ranged) {
			ImgTexture* blueRanged = new ImgTexture(renderer);
			ImgTexture* redRanged = new ImgTexture(renderer);
			blueRanged->loadFromImage(("textures/" + it->second.anime_ranged_information.textureBlue).c_str());
			redRanged->loadFromImage(("textures/" + it->second.anime_ranged_information.textureRed).c_str());
			blueRangedTextures.emplace(it->first, blueRanged);
			redRangedTextures.emplace(it->first, redRanged);
			ImgTexture* projectile = new ImgTexture(renderer);
			projectile->loadFromImage(("textures/" + it->second.anime_projectile_information.texture).c_str());
			projectileTextures.emplace(it->first, projectile);
		}
		ImgTexture* blueBody = new ImgTexture(renderer);
		ImgTexture* redBody = new ImgTexture(renderer);
		blueBody->loadFromImage(("textures/" + it->second.anime_body_information.textureBlue).c_str());
		redBody->loadFromImage(("textures/" + it->second.anime_body_information.textureRed).c_str());
		blueBodyTextures.emplace(it->first, blueBody);
		redBodyTextures.emplace(it->first, redBody);
	}
}

void View::loadDamageTexture() {
	damage = new ImgTexture(renderer);
	damage->loadFromImage(("textures/" + model->settings.damageInfo.texture).c_str());
}

void View::animateSelectionCircle(SoldierAnimation* anime, ZoomableGUIController* ctrl) {
	Soldier* soldier = anime->soldier;
	SDL_Rect segment;
	segment.x = 0; segment.y = 64; segment.w = 32; segment.h = 32;
	token->renderZoomed(soldier->pos.coeff(0), soldier->pos.coeff(1), soldier->rad, 32, 32, 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, ctrl->zoom, ctrl->center, 0, NULL, &segment);
}

void View::animateBackground(Animation* anime, ZoomableGUIController* ctrl) {
	if(model->settings.custom_background) {
		SDL_Rect clip;
		clip.x = 0 + anime->stage * anime->info.size_x;
		clip.y = 0;
		clip.w = anime->info.size_x;
		clip.h = anime->info.size_y;
		double rad = 0.5*map->width;
		double aspect = static_cast<double>(map->width) / map->height;
		double center_x = 0.5*map->width;
		double center_y = 0.5*map->height;
		anime->texture->renderZoomed(center_x, center_y, rad, 
			anime->info.frame_size_x, anime->info.frame_size_y, anime->info.frame_origin_x, anime->info.frame_origin_y,
			SCREEN_WIDTH, SCREEN_HEIGHT, ctrl->zoom, ctrl->center,
			0.0, NULL, &clip, SDL_FLIP_NONE, aspect);
		switch(model->state) {
		case MODEL_SIMULATION:
		case MODEL_GAME_RUNNING:
			anime->advance();
			break;
		}
	}
}

void MapEditorView::animateBackground(Animation* anime, ZoomableGUIController* ctrl) {
	if(model->settings.custom_background) {
		SDL_Rect clip;
		clip.x = 0 + anime->stage * anime->info.size_x;
		clip.y = 0;
		clip.w = anime->info.size_x;
		clip.h = anime->info.size_y;
		double rad = 0.5*map->width;
		double aspect = static_cast<double>(map->width) / map->height;
		double center_x = 0.5*map->width;
		double center_y = 0.5*map->height;
		anime->texture->renderZoomed(center_x, center_y, rad, 
			anime->info.frame_size_x, anime->info.frame_size_y, anime->info.frame_origin_x, anime->info.frame_origin_y,
			SCREEN_WIDTH, SCREEN_HEIGHT, ctrl->zoom, ctrl->center,
			0.0, NULL, &clip, SDL_FLIP_NONE, aspect);
		anime->advance();
	}
}

void View::animateSoldier(SoldierAnimation* anime, ZoomableGUIController* ctrl, bool hpAlpha) {
	Soldier* soldier = anime->soldier;
	if((soldier->alive && soldier->placed) || !hpAlpha) {
		double ang;
		if(hpAlpha) ang = soldier->angle * 180 / M_PI + 90;
		else ang = 0.;
		SDL_Rect clip;
		clip.x = 0 + anime->stage * anime->info.size_x;
		clip.y = 0;
		clip.w = anime->info.size_x;
		clip.h = anime->info.size_y;
		if(hpAlpha)
			SDL_SetTextureAlphaMod(anime->texture->texture, 255*std::min(double(soldier->hp) / soldier->maxHP, 1.));		
		anime->texture->renderZoomed(soldier->pos.coeff(0), soldier->pos.coeff(1), soldier->rad, 
			anime->info.frame_size_x, anime->info.frame_size_y, anime->info.frame_origin_x, anime->info.frame_origin_y,
			SCREEN_WIDTH, SCREEN_HEIGHT, ctrl->zoom, ctrl->center,
			ang, NULL, &clip);
		if(hpAlpha)
			SDL_SetTextureAlphaMod(anime->texture->texture, 255);
		switch(model->state) {
		case MODEL_SIMULATION:
		case MODEL_GAME_RUNNING:
			anime->advance();
			break;
		}
	}
}

void View::animateProjectile(ProjectileAnimation* anime, ZoomableGUIController* ctrl) {
	Projectile* projectile = anime->projectile;
	double ang = projectile->angle * 180 / M_PI + 90;
	SDL_Rect clip;
	clip.x = 0 + anime->stage * anime->info.size_x;
	clip.y = 0;
	clip.h = anime->info.size_y;
	clip.w = anime->info.size_x;
	anime->texture->renderZoomed(projectile->pos.coeff(0), projectile->pos.coeff(1), anime->info.length * 0.5, 
		anime->info.frame_size_x, anime->info.frame_size_y, anime->info.frame_origin_x, anime->info.frame_origin_y,
		SCREEN_WIDTH, SCREEN_HEIGHT, ctrl->zoom, ctrl->center,
		ang, NULL, &clip);
	switch(model->state) {
	case MODEL_SIMULATION:
	case MODEL_GAME_RUNNING:
		anime->advance();
		break;
	}
}


void MapEditorView::drawPlacedObjects(MapEditorController* ctrl) {
	for(auto object : map->mapObjects) {
		Color* objColor = colorPurple;
		if(object == ctrl->selectedObj) {
			switch(ctrl->prevState) {
			case EDITOR_MOVING:
				objColor = darkGreen; break;
			case EDITOR_COPYING:
				objColor = colorOrange; break;
			default:
				objColor = colorGreen; break;
			}
		}
		switch(object->type) {
		case MAP_WAYPOINT:
			if(object != ctrl->selectedObj) objColor = colorGrey;
		case MAP_CIRCLE: {
			Circle* circ = dynamic_cast<Circle*>(object);
			DrawCircle(circ, renderer, objColor, SCREEN_WIDTH, SCREEN_HEIGHT, ctrl->zoom, ctrl->center);
			break;}
		case MAP_DEPLOYMENT_ZONE:
			// here go the color selection rules
			if(object != ctrl->selectedObj) {
				if(dynamic_cast<DeploymentZone*>(object)->player_id == 0) objColor = colorBlue;
				else objColor = colorRed;
			}
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

}


void MapEditorView::drawMenu(MapEditorController* ctrl) {
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
		case EDITOR_PLACING_DP_ZONE:
			text = _dzplace; break;
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
}

void MapEditorView::drawInputTextbox(MapEditorController* ctrl) {
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
}

void MapEditorView::drawPlacingObject(MapEditorController* ctrl) {
	if(ctrl->objToPlace) {
		switch(ctrl->objToPlace->type) {
		case MAP_CIRCLE:
		case MAP_WAYPOINT: {
			Circle* circ = dynamic_cast<Circle*>(ctrl->objToPlace);
			DrawCircle(circ, renderer, colorGreen, SCREEN_WIDTH, SCREEN_HEIGHT, ctrl->zoom, ctrl->center);
			objDimensions->loadFromString(std::format("rectangle\nradius: {}", circ->rad), font, colorText, textwidth);
			break;}
		case MAP_BORDER:
		case MAP_RECTANGLE:
		case MAP_DEPLOYMENT_ZONE: {
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
}


#endif