#define NOMINMAX
#include <Windows.h>	// this line is very dangerous, moving this statement to a different location causes all sorts of problems

#include <server.h>
#include <shapes.h>
#include <view.h>
#include <input.h>

#include <SDL.h>
#include <stdio.h>
#include <SDL_ttf.h>
//#include <omp.h>


// Declaring variables
SDL_Window* window = NULL;
SDL_Renderer* renderer = NULL;
Uint32 CURRENT_TICK;
Uint32 T_OF_NEXT_TICK;
Uint32 T_OF_NEXT_FPS_UPDATE;
Uint32 T_OF_NEXT_REFORM;
int TICKS_SINCE_LAST_FPS_UPDATE;
int TICKS_SINCE_LAST_REFORM;
int SCREEN_WIDTH = 1250;
int SCREEN_HEIGHT = 750;

GameEventManager* em;
KeyboardAndMouseController* ctrl;
View* view;
Map* map;
Model* model;

void close() {
	SDL_DestroyRenderer(renderer);
	SDL_DestroyWindow(window);
	window = NULL;
	renderer = NULL;
}

void OpenWindow(Map* map) {
	em = new GameEventManager(30);
	dynamic_cast<GameEventManager*>(em)->map = map;
	model = new Model(em, map);
	//model->loadSoldierTypes("config/templates/classes.json");
	//model->loadUnitTypes("config/templates/units.json");
	//model->loadDamageInfo();
	//model->loadSettings("config/game_settings.json");
	model->init();
	dynamic_cast<GameEventManager*>(em)->model = model;
	// #### set up players with units from armylist.json
	Player* player1 = new Player(true);
	model->players.push_back(player1);
	model->player1 = player1;
	Player* player2 = new Player(false);
	model->players.push_back(player2);
	model->player2 = player2;
	model->loadArmyLists("config/templates/armylist.json");
	model->SetPlayer();
	model->SetUnit();
	ctrl = new KeyboardAndMouseController(em, SCREEN_WIDTH, SCREEN_HEIGHT, map);
	dynamic_cast<GameEventManager*>(em)->ctrl = ctrl;

	// Creating Window and Renderer
	//	Creating window to get screen size
	window = SDL_CreateWindow("Game",
			SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 
			SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_SHOWN);
	SDL_DisplayMode mode;
	int displayIndex = SDL_GetWindowDisplayIndex(window);
	SDL_DestroyWindow(window);
	SDL_GetDesktopDisplayMode(displayIndex, &mode);
	//  Creating actual game window small enough so that console can be viewed
	window = SDL_CreateWindow("Game", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
	int((&mode)->w * 0.9), int((&mode)->h * 0.9), SDL_WINDOW_SHOWN);
	if(!window) {
		std::printf("Could not create window: %s", SDL_GetError());
	}
	renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
	if(!renderer) {
		std::printf("Could not create renderer: %s", SDL_GetError());
	}

	view = new View(em, map, window, renderer);
	dynamic_cast<GameEventManager*>(em)->view = view;
	UnitRosterModifiedEvent e;
	em->Post(&e);
}

void ResetTextbox(std::string text, bool input) {
	if(input) {
		if(!SDL_IsTextInputActive()) SDL_StartTextInput();
	}
	else {
		if(SDL_IsTextInputActive()) SDL_StopTextInput();
	}
	ChangeTextboxEvent cev(text);
	em->Post(&cev);
}


int main(int argc, char* argv[1]) {
	AllocConsole();
	freopen("CONOUT$", "w", stdout);
	freopen("CONOUT$", "w", stderr);

	// Initializing SDL
	if(SDL_Init(SDL_INIT_EVERYTHING) != 0) {
		std::cout << "Failed to initialize SDL!\n";
		return -1;
	}
	if(TTF_Init() != 0) {
		std::cout << "Failed to initialize TTF!\n";
		return -1;
	}
	SDL_StopTextInput();

	//map = new Map("maps/desert.json");
	map = new Map("maps/testmap.json");
	OpenWindow(map);

	// Extra Debug section
	Eigen::Vector2d start_pos;
	start_pos << 20, 90;
	Eigen::Vector2d vel;
	vel << 77*2/3, 77*2/3;
	//Projectile* proj = new Projectile(start_pos, vel, 225, *model->dt);
	//model->projectiles.push_back(proj);

	start_pos << 200, 200;
	Eigen::Matrix2d rot;
	rot << 1, 0, 0, 1;
	//Event ume = UnitRosterModifiedEvent();
	//em->Post(&ume);
	//UnitPlaceRequest placeTurret(turret, start_pos, rot);
	//em->Post(&placeTurret);

	// Main loop
	bool quit = false;
	SDL_Event e;
	int t = 0;
	CURRENT_TICK = SDL_GetTicks();
	T_OF_NEXT_TICK = CURRENT_TICK + em->dt * 1000;
	T_OF_NEXT_FPS_UPDATE = CURRENT_TICK + 2000;
	TICKS_SINCE_LAST_FPS_UPDATE = 0;
	T_OF_NEXT_REFORM = CURRENT_TICK + 2000;
	TICKS_SINCE_LAST_REFORM = 0;
	while (!quit) {
		while (SDL_PollEvent(&e)) {
			if (e.type == SDL_QUIT) {
				quit = true;
			}
			else if((e.type == SDL_MOUSEBUTTONUP) || (e.type == SDL_KEYUP) || (e.type == SDL_KEYDOWN) ||(e.type == SDL_TEXTINPUT) || (e.type == SDL_MOUSEMOTION)) {
				SDLEvent ev(e);
				em->Post(&ev);
			}
		}
		t = (t+1)%SCREEN_WIDTH;
		CURRENT_TICK = SDL_GetTicks();
		if (CURRENT_TICK >= T_OF_NEXT_TICK) {
			//debug("Tick");
			TickEvent ev;
			em->Post(&ev);
			T_OF_NEXT_TICK = CURRENT_TICK + em->dt * 1000;
			TICKS_SINCE_LAST_FPS_UPDATE++;
		}
		if (CURRENT_TICK >= T_OF_NEXT_FPS_UPDATE) {
			std::cout << 0.5 * TICKS_SINCE_LAST_FPS_UPDATE << " fps\n";
			TICKS_SINCE_LAST_FPS_UPDATE = 0;
			T_OF_NEXT_FPS_UPDATE = CURRENT_TICK + 2000;
		}
		if (CURRENT_TICK >= T_OF_NEXT_REFORM) {
			ReformEvent ev;
			em->Post(&ev);
			TICKS_SINCE_LAST_REFORM = 0;
			T_OF_NEXT_REFORM = CURRENT_TICK + 5000;
		}
		Event* ev = NULL;
		switch(ctrl->state()) {
		case CTRL_IDLE:
			ResetTextbox("", false);
			break;
		case CTRL_LOADING:
			if(!SDL_IsTextInputActive()) {
				ResetTextbox("Loading new map. Enter file name: ", true);
			}
			if(ctrl->inputConfirmed()) {
				map = new Map("maps/" + ctrl->input());
				close();
				OpenWindow(map);
			}
			break;
		case CTRL_QUITTING:
			quit = true;
			break;
		}
		if(ev) em->Post(ev);
	}

	// Cleanup
	close();
	SDL_Quit();
	return -1;
}