#ifndef SHAPES
#define SHAPES

#include <extra_math.h>

#include <SDL.h>
#include <stdio.h>
#include <cmath>
#include <Dense>


class Color {
public:
	unsigned int r;
	unsigned int g;
	unsigned int b;
	unsigned int al;

	Color(unsigned int r, unsigned int g, unsigned int b, unsigned int alpha) {
		this->r = r; this->g = g; this->b = b; this->al = alpha;
	}
};



void DrawCircle(double x, double y, double rad, SDL_Renderer* renderer, Color* color) {
	SDL_SetRenderDrawColor(renderer, color->r, color->g, color->b, color->al);
	int i_max = std::ceil(rad);
	int j_max = std::ceil(rad);
	int i = 0;
	int j = 0;
	int i_square = i * i;
	int j_square = j * j;
	double r_square = rad * rad;
	while(i <= i_max) {
		while(j <= j_max) {
			if(i_square + j_square < r_square) {
				SDL_RenderDrawPoint(renderer, x+i, y+j);
				SDL_RenderDrawPoint(renderer, x+j, y-i);
				SDL_RenderDrawPoint(renderer, x-i, y-j);
				SDL_RenderDrawPoint(renderer, x-j, y+i);
			}
			j_square += j + j + 1;
			j++;
		}
		i_square += i + i + 1;
		i++;
		j = 0;	
		j_square = 0;
	}
}

void DrawCircle(Circle* circ, SDL_Renderer* renderer, Color* color, int SCREEN_HEIGHT) {
	SDL_SetRenderDrawColor(renderer, color->r, color->g, color->b, color->al);
	int rad = circ->rad;
	int x = circ->pos.coeff(0);
	int y = SCREEN_HEIGHT - circ->pos.coeff(1);
	int i_max = std::ceil(rad);
	int j_max = std::ceil(rad);
	int i = 0;
	int j = 0;
	int i_square = i * i;
	int j_square = j * j;
	double r_square = rad * rad;
	while(i <= i_max) {
		while(j <= j_max) {
			if(i_square + j_square < r_square) {
				SDL_RenderDrawPoint(renderer, x+i, y+j);
				SDL_RenderDrawPoint(renderer, x+j, y-i);
				SDL_RenderDrawPoint(renderer, x-i, y-j);
				SDL_RenderDrawPoint(renderer, x-j, y+i);
			}
			j_square += j + j + 1;
			j++;
		}
		i_square += i + i + 1;
		i++;
		j = 0;	
		j_square = 0;
	}
}


void DrawFacingArrowhead(Eigen::Vector2d pos, Eigen::Matrix2d rot, double rad, SDL_Renderer* renderer, Color* color) {
	SDL_SetRenderDrawColor(renderer, color->r, color->g, color->b, color->al);
	Eigen::Vector2d p0, p1, p2;
	p0 << 0, -rad;
	p1 << rad, 0;
	p2 << 0, rad;
	p0 = rot*p0 + pos;
	p1 = rot*p1 + pos;
	p2 = rot*p2 + pos;
	SDL_RenderDrawLine(renderer, p0.coeff(0), p0.coeff(1), p1.coeff(0), p1.coeff(1));
	SDL_RenderDrawLine(renderer, p1.coeff(0), p1.coeff(1), p2.coeff(0), p2.coeff(1));
}

static const double arrow_length = 40;
static const double arrowhead_depth = 10;
static const double arrowhead_width = 8;

void DrawUnitArrow(Eigen::Vector2d pos, Eigen::Matrix2d rot, SDL_Renderer* renderer, Color* color) {
	SDL_SetRenderDrawColor(renderer, color->r, color->g, color->b, color->al);
	Eigen::Vector2d p0, p1, p2, p3;
	p0 << arrow_length - arrowhead_depth, -arrowhead_width/2;
	p1 << arrow_length, 0;
	p2 << arrow_length - arrowhead_depth, arrowhead_width/2;
	p0 = rot*p0 + pos;
	p1 = rot*p1 + pos;
	p2 = rot*p2 + pos;
	p3 = pos;
	SDL_RenderDrawLine(renderer, p0.coeff(0), p0.coeff(1), p1.coeff(0), p1.coeff(1));
	SDL_RenderDrawLine(renderer, p1.coeff(0), p1.coeff(1), p2.coeff(0), p2.coeff(1));
	SDL_RenderDrawLine(renderer, p1.coeff(0), p1.coeff(1), p3.coeff(0), p3.coeff(1));
}

void DrawRectangle(Rrectangle* rec, SDL_Renderer* renderer, Color* color, int SCREEN_HEIGHT) {
	SDL_SetRenderDrawColor(renderer, color->r, color->g, color->b, color->al);
	Eigen::Vector2d p0, p1, p2, p3;	//top left->top right->bottom right->bottom left
	p0 << rec->corners.at(0)->pos.coeff(0), rec->corners.at(0)->pos.coeff(1);
	p1 << rec->corners.at(1)->pos.coeff(0), rec->corners.at(1)->pos.coeff(1);
	p2 << rec->corners.at(2)->pos.coeff(0), rec->corners.at(2)->pos.coeff(1);
	p3 << rec->corners.at(3)->pos.coeff(0), rec->corners.at(3)->pos.coeff(1);
	SDL_RenderDrawLine(renderer, p0.coeff(0), SCREEN_HEIGHT - p0.coeff(1), p1.coeff(0), SCREEN_HEIGHT - p1.coeff(1));
	SDL_RenderDrawLine(renderer, p1.coeff(0), SCREEN_HEIGHT - p1.coeff(1), p2.coeff(0), SCREEN_HEIGHT - p2.coeff(1));
	SDL_RenderDrawLine(renderer, p2.coeff(0), SCREEN_HEIGHT - p2.coeff(1), p3.coeff(0), SCREEN_HEIGHT - p3.coeff(1));
	SDL_RenderDrawLine(renderer, p3.coeff(0), SCREEN_HEIGHT - p3.coeff(1), p0.coeff(0), SCREEN_HEIGHT - p0.coeff(1));

}

#endif