#ifndef GUI_BASE

#define GUI_BASE

#include <base.h>
#include <map.h>

class GeneralView : public Listener {
public:
	int SCREEN_WIDTH;
	int SCREEN_HEIGHT;
	SDL_Window* window;
	SDL_Renderer* renderer;

	GameEventManager* Gem() {
		return dynamic_cast<GameEventManager*>(em);
	}

	GeneralView(EventManager* em, SDL_Window* window, SDL_Renderer* renderer) : Listener(em) {
		SDL_GetWindowSize(window, &SCREEN_WIDTH, &SCREEN_HEIGHT);
		this->window = window;
		this->renderer = renderer;
	}
};


class ZoomableGUIController : public Listener {
public:
	double maxZoom;
	double minZoom;
	double zoomSpeed;
	double zoomMoveSpeed;

	double zoom;
	Eigen::Vector2d center;
	double zoomSpeedIn;
	double zoomSpeedOut;
	double zoomSpeedUp;
	double zoomSpeedRight;
	double zoomSpeedDown;
	double zoomSpeedLeft;

	GameEventManager* Gem() {
		return dynamic_cast<GameEventManager*>(em);
	}

	ZoomableGUIController(EventManager* em, int SCREEN_WIDTH, int SCREEN_HEIGHT, Map* map) : Listener(em) {
		zoom = std::min(SCREEN_WIDTH / double(map->width), SCREEN_HEIGHT / double(map->height));
		center << zoom * double(map->width) / 2., zoom * double(map->height) / 2.;

		maxZoom = 2.;
		minZoom = 0.1;
		zoomSpeed = 1;
		zoomMoveSpeed = 1.;

		zoomSpeedIn = 0.;
		zoomSpeedOut = 0.;
		zoomSpeedUp = 0.;
		zoomSpeedRight = 0.;
		zoomSpeedDown = 0.;
		zoomSpeedLeft = 0.;
	}
};


#endif