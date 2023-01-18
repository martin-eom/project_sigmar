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

void DrawLine(Point* p1, Point* p2, SDL_Renderer* renderer, Color* color, int SCREEN_WIDTH, int SCREEN_HEIGHT, double zoom, Eigen::Vector2d center) {
	Eigen::Vector2d diag; diag << SCREEN_WIDTH / 2., SCREEN_HEIGHT / 2.;
	Eigen::Vector2d pos1 = zoom * p1->pos - center + diag;
	pos1(1) = SCREEN_HEIGHT - pos1.coeff(1);
	Eigen::Vector2d pos2 = zoom * p2->pos - center + diag;
	pos2(1) = SCREEN_HEIGHT - pos2.coeff(1);
	SDL_SetRenderDrawColor(renderer, color->r, color->g, color->b, color->al);
	SDL_RenderDrawLine(renderer, pos1.coeff(0), pos1.coeff(1), pos2.coeff(0), pos2.coeff(1));
}

void DrawCircle(double x, double y, double rad, SDL_Renderer* renderer, Color* color, int SCREEN_WIDTH, int SCREEN_HEIGHT, double zoom, Eigen::Vector2d center) {
	Eigen::Vector2d diag; diag << SCREEN_WIDTH / 2., SCREEN_HEIGHT / 2.;
	double rrad = zoom * rad;
	double xx = zoom*x - center.coeff(0) + diag.coeff(0);
	double yy = zoom*y - center.coeff(1) + diag.coeff(1);
	yy = SCREEN_HEIGHT - yy;
	SDL_SetRenderDrawColor(renderer, color->r, color->g, color->b, color->al);
	int i_max = std::ceil(rrad);
	int j_max = std::ceil(rrad);
	int i = 0;
	int j = 0;
	int i_square = i * i;
	int j_square = j * j;
	double r_square = rrad * rrad;
	while(i <= i_max) {
		while(j <= j_max) {
			if(i_square + j_square < r_square) {
				SDL_RenderDrawPoint(renderer, xx+i, yy+j);
				SDL_RenderDrawPoint(renderer, xx+j, yy-i);
				SDL_RenderDrawPoint(renderer, xx-i, yy-j);
				SDL_RenderDrawPoint(renderer, xx-j, yy+i);
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

void DrawCircle(Circle* circ, SDL_Renderer* renderer, Color* color, int SCREEN_WIDTH, int SCREEN_HEIGHT, double zoom, Eigen::Vector2d center) {
	Eigen::Vector2d diag; diag << SCREEN_WIDTH / 2., SCREEN_HEIGHT / 2.;
	Circle ccirc(circ->pos*zoom - center + diag, zoom * circ->rad);
	ccirc.pos(1) = SCREEN_HEIGHT - ccirc.pos.coeff(1);
	SDL_SetRenderDrawColor(renderer, color->r, color->g, color->b, color->al);
	int rad = ccirc.rad;
	int x = ccirc.pos.coeff(0);
	int y = ccirc.pos.coeff(1); //SCREEN_HEIGHT - ccirc.pos.coeff(1);
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


void DrawFacingArrowhead(Eigen::Vector2d pos, Eigen::Matrix2d rot, double rad, SDL_Renderer* renderer, Color* color, int SCREEN_WIDTH, int SCREEN_HEIGHT, double zoom, Eigen::Vector2d center) {
	Eigen::Vector2d diag; diag << SCREEN_WIDTH / 2., SCREEN_HEIGHT / 2.;
	Eigen::Vector2d ppos = zoom*pos - center + diag;
	ppos(1) = SCREEN_HEIGHT - ppos.coeff(1);
	Eigen::Matrix2d rrot = rot.transpose();
	double rrad = zoom * rad;
	SDL_SetRenderDrawColor(renderer, color->r, color->g, color->b, color->al);
	Eigen::Vector2d p0, p1, p2;
	p0 << 0, -rrad;
	p1 << rrad, 0;
	p2 << 0, rrad;
	p0 = rrot*p0 + ppos;
	p1 = rrot*p1 + ppos;
	p2 = rrot*p2 + ppos;
	SDL_RenderDrawLine(renderer, p0.coeff(0), p0.coeff(1), p1.coeff(0), p1.coeff(1));
	SDL_RenderDrawLine(renderer, p1.coeff(0), p1.coeff(1), p2.coeff(0), p2.coeff(1));
}

static const double arrow_length = 40;
static const double arrowhead_depth = 10;
static const double arrowhead_width = 8;

void DrawUnitArrow(Eigen::Vector2d pos, Eigen::Matrix2d rot, SDL_Renderer* renderer, Color* color, int SCREEN_WIDTH, int SCREEN_HEIGHT, double zoom, Eigen::Vector2d center) {
	Eigen::Vector2d diag; diag << SCREEN_WIDTH / 2., SCREEN_HEIGHT / 2.;
	Eigen::Vector2d ppos = zoom*pos - center + diag;
	ppos(1) = SCREEN_HEIGHT - ppos.coeff(1);
	Eigen::Matrix2d rrot = rot.transpose();
	//rrot[0,1] *= -1; rrot[1,0] *= -1;
	SDL_SetRenderDrawColor(renderer, color->r, color->g, color->b, color->al);
	Eigen::Vector2d p0, p1, p2, p3;
	p0 << arrow_length - arrowhead_depth, -arrowhead_width/2;
	p1 << arrow_length, 0;
	p2 << arrow_length - arrowhead_depth, arrowhead_width/2;
	p0 = rrot*p0 + ppos;
	p1 = rrot*p1 + ppos;
	p2 = rrot*p2 + ppos;
	p3 = ppos;
	SDL_RenderDrawLine(renderer, p0.coeff(0), p0.coeff(1), p1.coeff(0), p1.coeff(1));
	SDL_RenderDrawLine(renderer, p1.coeff(0), p1.coeff(1), p2.coeff(0), p2.coeff(1));
	SDL_RenderDrawLine(renderer, p1.coeff(0), p1.coeff(1), p3.coeff(0), p3.coeff(1));
}

void DrawRectangle(Rrectangle* rec, SDL_Renderer* renderer, Color* color, int SCREEN_WIDTH, int SCREEN_HEIGHT, double zoom, Eigen::Vector2d center) {
	Eigen::Vector2d diag; diag << SCREEN_WIDTH / 2., SCREEN_HEIGHT / 2.;
	Rrectangle rrec(rec->hl*zoom, rec->hw*zoom, rec->pos*zoom - center + diag, rec->rot);
	SDL_SetRenderDrawColor(renderer, color->r, color->g, color->b, color->al);
	Eigen::Vector2d p0, p1, p2, p3;	//top left->top right->bottom right->bottom left
	p0 << rrec.corners.at(0)->pos.coeff(0), rrec.corners.at(0)->pos.coeff(1);
	p1 << rrec.corners.at(1)->pos.coeff(0), rrec.corners.at(1)->pos.coeff(1);
	p2 << rrec.corners.at(2)->pos.coeff(0), rrec.corners.at(2)->pos.coeff(1);
	p3 << rrec.corners.at(3)->pos.coeff(0), rrec.corners.at(3)->pos.coeff(1);
	SDL_RenderDrawLine(renderer, p0.coeff(0), SCREEN_HEIGHT - p0.coeff(1), p1.coeff(0), SCREEN_HEIGHT - p1.coeff(1));
	SDL_RenderDrawLine(renderer, p1.coeff(0), SCREEN_HEIGHT - p1.coeff(1), p2.coeff(0), SCREEN_HEIGHT - p2.coeff(1));
	SDL_RenderDrawLine(renderer, p2.coeff(0), SCREEN_HEIGHT - p2.coeff(1), p3.coeff(0), SCREEN_HEIGHT - p3.coeff(1));
	SDL_RenderDrawLine(renderer, p3.coeff(0), SCREEN_HEIGHT - p3.coeff(1), p0.coeff(0), SCREEN_HEIGHT - p0.coeff(1));
	if(ddebug::_showDebugGraphics) {
		SDL_RenderDrawLine(renderer, p0.coeff(0), SCREEN_HEIGHT - p0.coeff(1), p2.coeff(0), SCREEN_HEIGHT - p2.coeff(1));
		SDL_RenderDrawLine(renderer, p1.coeff(0), SCREEN_HEIGHT - p1.coeff(1), p3.coeff(0), SCREEN_HEIGHT - p3.coeff(1));	
	}

}

#endif