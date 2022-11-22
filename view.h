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
			//drawing map objects
			for(auto circ : map->circles) {
				DrawCircle(circ->pos.coeff(0), SCREEN_HEIGHT - circ->pos.coeff(1), circ->rad, renderer, colorPurple);
			}
			for(auto rec : map->rectangles) {
				DrawRectangle(rec, renderer, colorPurple, SCREEN_HEIGHT);
			}
			for(auto row : map->tiles) {
				for(auto tile : row) {
					if(tile->circles.size() or tile->rectangles.size()) {
						Rrectangle* rec = tile->rec;
						DrawRectangle(rec, renderer, colorUnitArrow, SCREEN_HEIGHT);
					}
				}
			}
			//drawing soldiers and unit markers
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
										Rrectangle rec = UnitRectangle(unit, i);
										DrawRectangle(&rec, renderer, colorUnitArrow, SCREEN_HEIGHT);
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
										/*Rrectangle rec = SoldierRectangle(soldier);
										DrawRectangle(&rec, renderer, colorPurple, SCREEN_HEIGHT);*/
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