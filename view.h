#define VIEW

#ifndef BASE
#include "base.h"
#endif
#ifndef SHAPES
#include "shapes.h"
#endif
#ifndef SERVER
#include "server.h"
#endif

#include <SDL.h>
#include <stdio.h>


class View : public Listener {
	private:
		int SCREEN_HEIGHT;
		SDL_Window* window;
		SDL_Renderer* renderer;
		Map* map;
		Model* model;
	public:
		View(EventManager* em, Map* map, Model* model, SDL_Window* window, SDL_Renderer* renderer, int SCREEN_HEIGHT) : Listener(em) {
			this->SCREEN_HEIGHT = SCREEN_HEIGHT;
			this->window = window;
			this->renderer = renderer;
			this->model = model;
			this->map = map;
		}
	private:
		void Update() {
			SDL_SetRenderDrawColor(renderer, 0x00, 0x00, 0x00, 0xFF);
			SDL_RenderClear(renderer);
			unsigned int colorCircle[4] = { 0x00, 0x00, 0x00, 0xff };
			unsigned int colorArrow[4] = {0xff, 0xff, 0xff, 0xff};
			unsigned int colorUnitArrow[4] = {40, 252, 3, 0xff};
			unsigned int colorPurple[4] = {0xff, 0x00, 0xff, 0xff};
			Node<Player*>* player = model->players.head;
			int nplayers = model->players.length();
			int nplayer = 0;
			while(player) {
				if(nplayers == 1) {
					colorCircle[0] = 0x00;
					colorCircle[2] = 0xff;
				}
				else {
					colorCircle[0] = 0xff*nplayer/(nplayers-1);
					colorCircle[2] = 0xff*((nplayers-1)-nplayer)/(nplayers-1);
				}
				Node<Unit*>* unitNode = player->data->units.head;
				while(unitNode) {
					Unit* unit = unitNode->data;
					std::vector<std::vector<Soldier*>>* soldiers = unit->soldiers();
					if(unit->placed) {
						if(model->selectedUnitNode) {
							if(model->selectedUnitNode->data == unit) {
								Eigen::Vector2d viewPos;
								viewPos << unit->posTarget.coeff(0), SCREEN_HEIGHT - unit->posTarget.coeff(1);
								Eigen::Matrix2d viewRot;
								viewRot << unit->rotTarget.coeff(0,0), -unit->rotTarget.coeff(0,1), 
									-unit->rotTarget.coeff(1,0), unit->rotTarget.coeff(1,1);
								DrawUnitArrow(viewPos, viewRot, renderer, colorUnitArrow);
								for(int i = 0; i < unit->orders.size(); i++) {
									Order* o = unit->orders.at(i);
									if(o->type == ORDER_MOVE) {
										MoveOrder* mo = dynamic_cast<MoveOrder*>(o);
										std::vector<Eigen::Matrix2d> rectangle = Rectangle(unit, i);
										Eigen::Matrix2d rec = rectangle.at(0);
										Eigen::Matrix2d rot = rectangle.at(1);
										Eigen::Vector2d p0, p1, p2, p3;	//top left->top right->bottom right->bottom left
										p0 << rec.coeff(0,0), rec.coeff(0,1);
										p1 << rec.coeff(1,0), rec.coeff(0,1);
										p2 << rec.coeff(1,0), rec.coeff(1,1);
										p3 << rec.coeff(0,0), rec.coeff(1,1);
										p0 = mo->rot * p0 + mo->pos;
										p1 = mo->rot * p1 + mo->pos;
										p2 = mo->rot * p2 + mo->pos;
										p3 = mo->rot * p3 + mo->pos;
										SDL_RenderDrawLine(renderer, p0.coeff(0), SCREEN_HEIGHT - p0.coeff(1), 
											p1.coeff(0), SCREEN_HEIGHT - p1.coeff(1));
										SDL_RenderDrawLine(renderer, p1.coeff(0), SCREEN_HEIGHT - p1.coeff(1), 
											p2.coeff(0), SCREEN_HEIGHT - p2.coeff(1));
										SDL_RenderDrawLine(renderer, p2.coeff(0), SCREEN_HEIGHT - p2.coeff(1), 
											p3.coeff(0), SCREEN_HEIGHT - p3.coeff(1));
										SDL_RenderDrawLine(renderer, p3.coeff(0), SCREEN_HEIGHT - p3.coeff(1), 
											p0.coeff(0), SCREEN_HEIGHT - p0.coeff(1));
										if(i > 0) {
											Order* prevo = unit->orders.at(i-1);
											if(prevo->type == ORDER_MOVE) {
												MoveOrder* prevmo = dynamic_cast<MoveOrder*>(prevo);
												SDL_RenderDrawLine(renderer, mo->pos.coeff(0), SCREEN_HEIGHT - mo->pos.coeff(1),
													prevmo->pos.coeff(0), SCREEN_HEIGHT - prevmo->pos.coeff(1));
											}
										}
									}
								}
							}
						}
						for(int i = 0; i < unit->nrows(); i++) {
							for(int j = 0; j < unit->width(); j++) {
								Soldier* soldier = (*soldiers)[i][j];
								if(soldier->placed) {
									if(model->selectedUnitNode->data == unit) {
										DrawCircle(soldier->pos.coeff(0), SCREEN_HEIGHT - soldier->pos.coeff(1),
											soldier->rad() + 1, renderer, colorUnitArrow);
										//SDL_RenderDrawLine(renderer, soldier->pos.coeff(0), SCREEN_HEIGHT - soldier->pos.coeff(1), 
										//	soldier->posTarget.coeff(0), SCREEN_HEIGHT - soldier->posTarget.coeff(1));
									}
									DrawCircle(soldier->pos.coeff(0), SCREEN_HEIGHT - soldier->pos.coeff(1),
										soldier->rad(), renderer, colorCircle);
									Eigen::Vector2d viewPos;
									viewPos << soldier->pos.coeff(0), SCREEN_HEIGHT - soldier->pos.coeff(1);
									Eigen::Matrix2d viewRot;
									viewRot << soldier->rot.coeff(0,0), -soldier->rot.coeff(0,1), -soldier->rot.coeff(1,0), soldier->rot.coeff(1,1);
									if(soldier->currentOrder == soldier->unit->currentOrder) {
										DrawFacingArrowhead(viewPos, viewRot, soldier->rad(), renderer, colorPurple);
									}
									else {
										DrawFacingArrowhead(viewPos, viewRot, soldier->rad(), renderer, colorArrow);									
									}
								}
							}
						}
					}
					unitNode = unitNode->next;
				}
				player = player->next;
				nplayer++;
			}
			SDL_RenderPresent(renderer);
		}
		void Notify(Event* ev) {
			if(ev->type == TICK_EVENT) {
				Update();
			}
		}
};