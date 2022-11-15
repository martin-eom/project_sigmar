#define BASE

#ifndef LINKEDLISTS
#include "linkedlists.h"
#endif
#ifndef EVENTS
#include "events.h"
#endif
#ifndef UNITS
#include "units.h"
#endif

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
		std::list<int> noPrint = {GENERIC_EVENT, TICK_EVENT, SDL_EVENT};
		std::list<Listener*> listeners;
		int fps = 30;
		double dt = 1./30.;
		
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
	for (std::list<Listener*>::iterator it = listeners.begin(); it != listeners.end(); it++) {
			(*it)->Notify(ev);
	}
}
