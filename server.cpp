#define NOMINMAX
#include <Windows.h>	// this line is very dangerous, moving this statement to a different location causes all sorts of problems

#ifndef SERVER
#include "server.h"
#endif
#ifndef SHAPES
#include "shapes.h"
#endif
#ifndef VIEW
#include "view.h"
#endif
#ifndef INPUT
#include "input.h"
#endif

#include <SDL.h>
#include <stdio.h>


// Declaring variables
const int SCREEN_WIDTH = 1200;
const int SCREEN_HEIGHT = 750;
SDL_Window* window = NULL;
SDL_Renderer* renderer = NULL;
Uint32 CURRENT_TICK;
Uint32 T_OF_NEXT_TICK;
Uint32 T_OF_NEXT_FPS_UPDATE;
Uint32 T_OF_NEXT_REFORM;
int TICKS_SINCE_LAST_FPS_UPDATE;
int TICKS_SINCE_LAST_REFORM;


void close() {
	SDL_DestroyRenderer(renderer);
	SDL_DestroyWindow(window);
	window = NULL;
	renderer = NULL;
}

int main(int argc, char* argv[1]) {
	AllocConsole();
	freopen("CONOUT$", "w", stdout);
	freopen("CONOUT$", "w", stderr);

	// Setting up game objects
	EventManager* em = new EventManager(30);
	Map* map = new Map(SCREEN_WIDTH,SCREEN_HEIGHT,31);
	Model* model = new Model(em, map);
	Player* player1 = new Player();
	model->players.Append(new Node<Player*>(player1));
	player1->units.Append(new Node<Unit*>(new Infantry(player1)));
	player1->units.Append(new Node<Unit*>(new Infantry(player1)));
	player1->units.Append(new Node<Unit*>(new Infantry(player1)));
	player1->units.Append(new Node<Unit*>(new Cavalry(player1)));
	player1->units.Append(new Node<Unit*>(new MonsterUnit(player1)));
	Player* player2 = new Player();
	model->players.Append(new Node<Player*>(player2));
	player2->units.Append(new Node<Unit*>(new Infantry(player2)));
	player2->units.Append(new Node<Unit*>(new Infantry(player2)));
	player2->units.Append(new Node<Unit*>(new Infantry(player2)));
	player2->units.Append(new Node<Unit*>(new Cavalry(player2)));
	player2->units.Append(new Node<Unit*>(new MonsterUnit(player2)));
	model->SetPlayer();
	model->SetUnit();
	
	// Initializing SDL
	if(SDL_Init(SDL_INIT_EVERYTHING) != 0) {
		std::cout << "Failed to initialize SDL!\n";
		return -1;
	}
	
	// Creating Window and Renderer
	window = SDL_CreateWindow("Game",
			SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 
			SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_SHOWN);
	if(!window) {
		std::printf("Could not create window: %s", SDL_GetError());
		return -1;
	}
	renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
	if(!renderer) {
		std::printf("Could not create renderer: %s", SDL_GetError());
		return -1;
	}

	// Creating View object
	View* view = new View(em, map, model, window, renderer, SCREEN_HEIGHT);

	// Creating Input Controller object
	KeyboardAndMouseController* controller = new KeyboardAndMouseController(em, model, SCREEN_HEIGHT);

	// Extra Debug section
	/*Eigen::Matrix2d rec;
	Eigen::Matrix2d rot;
	Eigen::Vector2d pos;
	rec << -1., 0.5, 1., -0.5;
	rot << cos(0.78539816339), -sin(0.78539816339), cos(0.78539816339), sin(0.78539816339);
	rot << 1, 0, 0, 1;
	pos << 0.6, 0.6;
	if(PointInRectangle(pos, rec, rot)) {
		std::cout << "The point is in the thing!\n";
	}
	else {
		std::cout << "Disappointing...\n";
	}*/

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
			else if((e.type == SDL_MOUSEBUTTONUP) || (e.type == SDL_KEYUP) || (e.type == SDL_KEYDOWN)) {
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
	}

	// Cleanup
	close();
	SDL_Quit();
}