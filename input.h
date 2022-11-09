#define INPUT

#ifndef BASE
#include "base.h"
#endif
#ifndef EVENTS
#include "events.h"
#endif
#ifndef SERVER
#include "server.h"
#endif

#include<stdio.h>
#include<iostream>
#include<SDL.h>
#include<Dense>
#include<cmath>



class KeyboardAndMouseController : public Listener {
	private:
		int SCREEN_HEIGHT;
		Eigen::Vector2d p0, p1;
		Eigen::Matrix2d rot;
		bool firstPointSet;
		Model* model;
	public:
		KeyboardAndMouseController(EventManager* em, Model* model,int SCREEN_HEIGHT) : Listener(em) {
			this->SCREEN_HEIGHT = SCREEN_HEIGHT;
			this->model = model;
			firstPointSet = false;
		};
	private:
		void Notify(Event* ev) {
			if(ev->type == SDL_EVENT) {
				SDL_Event e = dynamic_cast<SDLEvent*>(ev)->event;
				if(e.type == SDL_MOUSEBUTTONUP) {
					int x, y;
					x = e.button.x;
					y = SCREEN_HEIGHT - e.button.y;
					if(firstPointSet) {
						p1 << x, y;
						double dx = p1.coeff(0) - p0.coeff(0);
						double dy = p1.coeff(1) - p0.coeff(1);
						double dp = sqrt(dx*dx + dy*dy);
						if(dp !=0 ) {
							double cos = dx / dp;
							double sin = dy / dp;
							rot << cos, -sin, sin, cos;
							if(model->selectedUnitNode) {
								Event* ev;
								if(model->selectedUnitNode->data->placed) {
									ev = new UnitMoveRequest(model->selectedUnitNode->data, p0, rot);
								}
								else {
									ev = new UnitPlaceRequest(model->selectedUnitNode->data, p0, rot);
								}
								em->Post(ev);
							}
							else {
								std::cout << "No unit was selected. Attempting to set unit..\n";
								model->SetUnit();
							}
						}
						else {
							std::cout << "First and second point are identical, can't get angel.\n";
						}
					}
					else {
						p0 << x, y;
					}
					firstPointSet = !firstPointSet;
				}
				else if(e.type == SDL_KEYUP) {
					Event* nev = new Event();
					if(e.key.keysym.sym == SDLK_LEFT) {
						nev = new UnitSelectEvent(-1);
					}
					else if(e.key.keysym.sym == SDLK_RIGHT) {
						nev = new UnitSelectEvent(1);
					}
					else if(e.key.keysym.sym == SDLK_UP) {
						nev = new PlayerSelectEvent(-1);
					}
					else if(e.key.keysym.sym == SDLK_DOWN) {
						nev = new PlayerSelectEvent(1);
					}
					if(nev->type != GENERIC_EVENT) {
						em->Post(nev);
					}
				}
			}
		}

};