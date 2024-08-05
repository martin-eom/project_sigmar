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
#include <chrono>




class Listener;


class EventManager {
	public:
		std::list<int> noPrint = {GENERIC_EVENT, TICK_EVENT, SDL_EVENT, CHANGE_TEXTBOX_EVENT};
		std::list<Listener*> listeners;
		std::vector<double> times;
		int fps = 30;
		double dt = 1./30.;
		virtual void _polymorphism(){};	//necessary to make the class polymorphic

		bool measureTime = false;
		bool showTimes = false;
		
		EventManager() {};
		EventManager(int fps){
			this->fps = fps;
			dt = 1./fps;
		}
				
		void RegisterListener(Listener* listener) {
			listeners.push_back(listener);
			times.push_back(0);
		}
		
		void UnregisterListener(Listener* listener) {
			int n_listener = std::distance(listeners.begin(), std::find(listeners.begin(), listeners.end(), listener));
			listeners.remove(listener);
			times.erase(std::next(times.begin(), n_listener));
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
		if(measureTime) {
			auto start = std::chrono::system_clock::now();
			listener->Notify(ev);
			auto end = std::chrono::system_clock::now();
			times.at(std::distance(listeners.begin(), std::find(listeners.begin(), listeners.end(), listener))) += std::chrono::duration<double>(end - start).count();		
		}
		else
			listener->Notify(ev);

		if(showTimes) {
			std::cout << "######### EM TIMING ###########\n";
			for(auto time : times) {
				std::cout << time << "\n";
			}
			std::cout << "###############################\n";
			showTimes = false;
			measureTime = false;
			//while(true) {}
		}
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