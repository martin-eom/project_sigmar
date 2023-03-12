#ifndef BASE
#define BASE


#include <events.h>
//#include <units.h>
#include <debug.h>

#ifndef _USE_MATH_DEFINES
#define _USE_MATH_DEFINES
#endif

#include <iostream>
#include <list>
#include <algorithm>
#include <string>
#include <cmath>




class Listener;


class EventManager {
	public:
		std::list<int> noPrint = {GENERIC_EVENT, TICK_EVENT, SDL_EVENT, CHANGE_TEXTBOX_EVENT};
		std::list<Listener*> listeners;
		int fps = 30;
		double dt = 1./30.;
		virtual void _polymorphism(){};	//necessary to make the class polymorphic
		
		EventManager() {};
		EventManager(int fps){
			this->fps = fps;
			dt = 1./fps;
		}
				
		void RegisterListener(Listener* listener) {
			listeners.push_back(listener);
		}
		
		void UnregisterListener(Listener* listener) {
			listeners.remove(listener);
		}
		
		void Post(Event* ev);

};


class Listener {
	public:
		EventManager* em;
		
		Listener(EventManager* em) {
			this->em = em;
			em->RegisterListener(this);
		}
		
		virtual void Notify(Event* ev){}
};


void EventManager::Post(Event* ev) {
	if(std::find(noPrint.begin(), noPrint.end(), ev->type)==noPrint.end()) {
		std::cout << " - posted " << ev->name << "\n";
	}
	for(auto listener : listeners) {
		listener->Notify(ev);
	}
}


class Map;
class Model;
class GeneralView;
class ZoomableGUIController;

class GameEventManager : public EventManager {
public:
	Map* map;
	ZoomableGUIController* ctrl;
	Model* model;
	GeneralView* view;

	GameEventManager(int fps) : EventManager(fps){
		map = NULL;
		ctrl = NULL;
		model = NULL;
		view = NULL;
	}
};


#endif