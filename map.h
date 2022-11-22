#define MAP

#ifndef BASE
#include "base.h"
#endif
#ifndef SOLDIERS
#include "soldiers.h"
#endif

#include <vector>
#include <Dense>


class gridpiece{
	public:
		LinkedList<Soldier*> soldiers;
		std::vector<Rrectangle*> rectangles;
		std::vector<Circle*> circles;
		LinkedList<gridpiece*> neighbours;
		Rrectangle* rec;
};

class Map {
	public:
		int tilesize;
		int width;
		int heigth;
		int nrows;
		int ncols;
		std::vector<std::vector<gridpiece*>> tiles;
		std::vector<Rrectangle*> rectangles;
		std::vector<Circle*> circles;
		
		Map(int width, int heigth, int tilesize) {
			this->width = width;
			this->heigth = heigth;
			this->tilesize = tilesize;
			nrows = heigth / tilesize + bool(heigth % tilesize);
			ncols = width / tilesize + bool(width % tilesize);
			tiles = std::vector<std::vector<gridpiece*>>(nrows, std::vector<gridpiece*>(ncols, NULL));
			for(int i = 0; i < nrows; i++) {
				for(int j = 0; j < ncols; j++) {
					tiles.at(i).at(j) = new gridpiece();
					Eigen::Vector2d center;
					center << tilesize/2. + j*tilesize, tilesize/2. + i*tilesize;
					Eigen::Matrix2d rot;
					rot << 1., 0., 0., 1.;
					tiles.at(i).at(j)->rec = new Rrectangle(tilesize / 2., tilesize / 2., center, rot);
				}
			}
			for(int i = 0; i < nrows - 1; i++) {
				for(int j = 0; j < ncols - 1; j++) {
					tiles.at(i).at(j)->neighbours.Append(new Node<gridpiece*>(tiles.at(i+1).at(j)));
					tiles.at(i).at(j)->neighbours.Append(new Node<gridpiece*>(tiles.at(i).at(j+1)));
					tiles.at(i).at(j)->neighbours.Append(new Node<gridpiece*>(tiles.at(i+1).at(j+1)));
					if(i == 0) {
						tiles.at(nrows-1).at(j)->neighbours.Append(new Node<gridpiece*>(tiles.at(nrows-1).at(j+1)));
					}
				}
				tiles.at(i).at(ncols-1)->neighbours.Append(new Node<gridpiece*>(tiles.at(i+1).at(ncols-1)));
			}
		}
		
		void Cleangrid() {
			for(int i = 0; i < nrows; i++) {
				for(int j = 0; j < ncols; j++) {
					tiles.at(i).at(j)->soldiers.Empty();
				}
			}
		}
		
		void Assign(Soldier* soldier, int i, int j) {
			tiles.at(i).at(j)->soldiers.Append(new Node<Soldier*>(soldier));
		}

		void AddMapRectangle(Rrectangle* rec) {
			rectangles.push_back(rec);
			Rrectangle extended(rec->hl + 0.5*tilesize, rec->hw + 0.5*tilesize, rec->pos, rec->rot);
			for(int i = 0; i < this->nrows; i++) {
				for(int j = 0; j < this->ncols; j++) {
					if(RectangleRectangleCollision(&extended, tiles.at(i).at(j)->rec)) {
						tiles.at(i).at(j)->rectangles.push_back(rec);
					}
				}
			}
		}

		void AddMapCircle(Circle* circle) {
			Circle extended = Circle(circle->pos, circle->rad + 0.5*tilesize);
			circles.push_back(circle);
			int i = 0;
			for(auto row : tiles) {
				for(auto tile : row) {
					if(CircleRectangleCollision(&extended, tile->rec)) {
						tile->circles.push_back(circle);
						i++;
					}
				}
			}
			std::cout << "circle collides with " << i << " tiles\n";
		}
};

void SoldierCircleCollision(Soldier* soldier, Circle* circle) {
	double collisionStrength = 2.;
	Eigen::Vector2d posCorrection;
	Eigen::Vector2d knockVel;
	Eigen::Vector2d dist = soldier->pos - circle->pos;
	double d = dist.norm();
	if(d <= (soldier->rad() + circle->rad)) {		
		double sin = dist.coeff(1) / d;
		double cos = dist.coeff(0) / d;
		Eigen::Matrix2d rot;
		rot << cos, -sin, sin, cos;
		Eigen::Vector2d rotVel = rot.transpose() * soldier->vel;
		if(rotVel.coeff(0) < 0) {
			rotVel(0) *= -collisionStrength;
			rotVel(1) = 0;
			rotVel = rot * rotVel;
			knockVel = rotVel;
			posCorrection = dist/d*(soldier->rad() + circle->rad - d);
			soldier->knockVel += knockVel;
			soldier->pos += posCorrection;
		}
	}
}

void SoldierRectangleCollision(Soldier* soldier, Rrectangle* rec) {
	Eigen::Vector2d knockVel;
	knockVel << 0., 0.;
	Eigen::Vector2d normPos = soldier->pos - rec->pos;
	Eigen::Vector2d rotPos = rec->rot.transpose() * (soldier->pos - rec->pos);
	Eigen::Vector2d soldierPosCorrection;
	soldierPosCorrection << 0., 0.;
	bool _long, _wide;
	_long = (-rec->hw <= rotPos.coeff(0) && rotPos.coeff(0) <= rec->hw && -rec->hl - soldier->rad() <= rotPos.coeff(1) && rotPos.coeff(1) <= rec->hl + soldier->rad());
	_wide = (-rec->hw - soldier->rad() <= rotPos.coeff(0) && rotPos.coeff(0) <= rec->hw + soldier->rad() && -rec->hl <= rotPos.coeff(1) && rotPos.coeff(1) <= rec->hl);
	double dl;
	double dw;
	if(rotPos.coeff(1) < 0) {dl = -(rec->hl + soldier->rad()) - rotPos.coeff(1);}
	else {dl = rec->hl + soldier->rad() - rotPos.coeff(1);}
	if(rotPos.coeff(0) < 0) {dw = -(rec->hw + soldier->rad()) - rotPos.coeff(0);}
	else {dw = rec->hw + soldier->rad() - rotPos.coeff(0);}
	int edge;
	if(_long) {
		if(_wide) {
			if(dl < 0) {
				if(dw < 0) {
					if(dw < dl) {edge = 3;}
					else {edge = 4;}
				}
				else {
					if(-dl < dw) {edge = 3;}
					else {edge = 2;}
				}
			}
			else {
				if(dw < 0) {
					if(dl < -dw) {edge = 1;}
					else {edge = 4;}
				}
				else {
					if(dl < dw) {edge = 1;}
					else {edge = 2;}
				}
			}		
		}
		else {
			if(dl < 0) {edge = 3;}
			else {edge = 1;}
		}
	}
	else {
		if(_wide) {
			if(dw < 0) {edge = 4;}
			else {edge = 2;}
		}
		else {
			edge = 0;
		}
	}
	Eigen::Vector2d rotVel = rec->rot.transpose() * soldier->vel;
	double collisionStrength = 2.;	//2. is physical
	switch(edge) {
	case 1:
		if(rotVel.coeff(1) < 0) {
			rotVel(0) = 0;
			rotVel(1) *= -collisionStrength;
			rotVel = rec->rot * rotVel;
			knockVel = rotVel;
			soldierPosCorrection(1) += dl;
			soldierPosCorrection = rec->rot * soldierPosCorrection;
		}
		break;
	case 2:
		if(rotVel.coeff(0) < 0) {
			rotVel(0) *= -collisionStrength;
			rotVel(1) = 0;
			rotVel = rec->rot * rotVel;
			knockVel = rotVel;
			soldierPosCorrection(0) += dw;
			soldierPosCorrection = rec->rot * soldierPosCorrection;
		}
		break;
	case 3:
		if(rotVel.coeff(1) > 0) {
			rotVel(0) = 0;
			rotVel(1) *= -collisionStrength;
			rotVel = rec->rot * rotVel;
			knockVel = rotVel;
			soldierPosCorrection(1) += dl;
			soldierPosCorrection = rec->rot * soldierPosCorrection;
		}
		break;
	case 4:
		if(rotVel.coeff(0) > 0) {
			rotVel(0) *= -collisionStrength;
			rotVel(1) = 0;
			rotVel = rec->rot * rotVel;
			knockVel = rotVel;
			soldierPosCorrection(0) += dw;
			soldierPosCorrection = rec->rot * soldierPosCorrection;
		}
		break;
	case 0:
		for(int i = 0; i < 4; i++) {
			Corner* corner = rec->corners.at(i);
			Eigen::Vector2d dist = soldier->pos - corner->pos;
			double d = dist.norm();
			if(d <= soldier->rad()) {
				double sin = dist.coeff(1) / d;
				double cos = dist.coeff(0) / d;
				Eigen::Matrix2d rot;
				rot << cos, -sin, sin, cos;
				Eigen::Vector2d rotVel = rot.transpose() * soldier->vel;
				if(rotVel.coeff(0) < 0) {
					rotVel(0) *= -collisionStrength;
					rotVel(1) = 0;
					rotVel = rot * rotVel;
					knockVel = rotVel;
					soldierPosCorrection(0) += soldier->rad() - d;
					soldierPosCorrection = rot * soldierPosCorrection;
				}
			}
		}
	// missing case for collision with corners
	}
	//knockVel << 0., 0.;
	soldier->knockVel += knockVel;
	soldier->pos += soldierPosCorrection;
}
