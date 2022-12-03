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
	//Map* map = new Map(SCREEN_WIDTH,SCREEN_HEIGHT);
	Map* map = new Map("maps/testmap.json");
	int SCREEN_WIDTH = map->width;
	int SCREEN_HEIGHT = map->height;
	Eigen::Vector2d recPos;
	recPos << 400., 400.;
	Eigen::Matrix2d recRot;
	recRot << 1, 0, 0, 1;
	recRot << cos(M_PI/4.), -sin(M_PI/4.), sin(M_PI/4.), cos(M_PI/4.);
	MapRectangle* rec1 = new MapRectangle(200, 200, recPos, recRot);
	//map->AddMapObject(rec1);
	Eigen::Vector2d circPos; circPos << 800, 400;
	MapCircle* tht = new MapCircle(circPos, 100.);
	//map->AddMapObject(tht);
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
	/*Eigen::Vector2d rpos; rpos << 0, 0;
	Eigen::Vector2d rpos2; rpos2 << 2., 0;
	Eigen::Vector2d cpos; cpos << -1.5, 1.5;
	Eigen::Matrix2d rot; rot << 1, 0, 0, 1;
	Rrectangle rec(1., 1., rpos, rot);
	Rrectangle rec2(1.5, 0.5, rpos2, rot);
	Circle circ(cpos, 1.);
	std::cout << "-----------------------------------------------\n";
	std::cout << RectangleRectangleCollision(&rec2, &rec) << std::endl;
	std::cout << "-----------------------------------------------\n";*/

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