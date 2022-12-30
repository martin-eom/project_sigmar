#define NOMINMAX
#include <Windows.h>	// this line is very dangerous, moving this statement to a different location causes all sorts of problems

#include <server.h>
#include <shapes.h>
#include <view.h>
#include <input.h>

#include <SDL.h>
#include <stdio.h>
#include <SDL_ttf.h>


// Declaring variables
SDL_Window* window = NULL;
SDL_Renderer* renderer = NULL;
Uint32 CURRENT_TICK;
Uint32 T_OF_NEXT_TICK;
Uint32 T_OF_NEXT_FPS_UPDATE;
Uint32 T_OF_NEXT_REFORM;
int TICKS_SINCE_LAST_FPS_UPDATE;
int TICKS_SINCE_LAST_REFORM;
int SCREEN_WIDTH;
int SCREEN_HEIGHT;

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
	SCREEN_WIDTH = map->width;
	SCREEN_HEIGHT = map->height;
	model = new Model(em, map);
	Player* player1 = new Player();
	model->players.push_back(player1);
	player1->units.push_back(new Infantry(player1));
	player1->units.push_back(new Cavalry(player1));
	player1->units.push_back(new MonsterUnit(player1));
	Player* player2 = new Player();
	model->players.push_back(player2);
	player2->units.push_back(new Infantry(player2));
	player2->units.push_back(new Cavalry(player2));
	player2->units.push_back(new MonsterUnit(player2));
	model->SetPlayer();
	model->SetUnit();
	ctrl = new KeyboardAndMouseController(em, model, SCREEN_HEIGHT);
	em->ctrl = ctrl;
	em->model = model;

	// Creating Window and Renderer
	window = SDL_CreateWindow("Game",
			SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 
			SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_SHOWN);
	if(!window) {
		std::printf("Could not create window: %s", SDL_GetError());
	}
	renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
	if(!renderer) {
		std::printf("Could not create renderer: %s", SDL_GetError());
	}

	view = new View(em, map, model, window, renderer, SCREEN_HEIGHT);

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
	//em->Post(new ChangeTextboxEvent(text));
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

	map = new Map("maps/testmap.json");
	OpenWindow(map);

	// Extra Debug section

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