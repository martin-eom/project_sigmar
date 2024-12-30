#ifndef EXTRA_MATH
#define EXTRA_MATH

#ifndef _USE_MATH_DEFINES
#define _USE_MATH_DEFINES
#endif

#include <Dense>
#include <iostream>

// Trigonometric functions
double Angle(double sin, double cos) {
	if(sin > 0) {
		return acos(cos);
	}
	else {
		return -acos(cos);
	}
}

double Tans(double sin, double cos) {
	return sin / cos;
}

Eigen::Matrix2d Rotation(double angle) {
	double _cos = cos(angle);
	double _sin = sin(angle);
	Eigen::Matrix2d rot; rot << _cos, -_sin, _sin, _cos;
	return rot;
}


//Geometric objects
class Point {
public:
	virtual void ConversionEnabler() {}

	Eigen::Vector2d pos;

	Point() {};

	Point(Eigen::Vector2d pos) {
		this->pos = pos;
	}

	double x() {return pos.coeff(0);}
	double y() {return pos.coeff(1);}
};

class Straight {
public:
	bool vertical = false;
	double vertical_x = false;
	double a = 1;
	double b = 0;

	Straight(Point* p1, Point* p2) {
		if(p2->x() == p1->x()) {
			vertical = true;
			vertical_x = p1->x();
		}
		else {
			a = (p2->y() - p1->y()) / (p2->x() - p1->x());
			b = p1->y() - a*p1->x();
		}
	}
};

Eigen::Vector2d StraightIntersect(Straight l1, Straight l2) {
	// ONLY WORKS if intersect EXISTS
	Eigen::Vector2d intersect;
	double x,y;
	if(l1.vertical) {
		x = l1.vertical_x;
		y = l2.a*x + l2.b;
	}
	else if(l2.vertical) {
		x = l2.vertical_x;
		y = l1.a*x + l1.b;
	}
	else {
		x = (l2.b - l1.b) / (l1.a - l2.a);
		y = l1.a*x + l1.b;
	}
	intersect << x, y;
	return intersect;
}

class Circle : public Point {
public:
	double rad;

	Circle () {}
	Circle(Eigen::Vector2d pos, double rad) : Point(pos) {
		this->rad = rad;
	}

	void Reposition(Eigen::Vector2d pos) {
		this->pos = pos;
	}
};

class Ppolygon;
//class Rrectangle;

class Corner : public Point { // point that holds reference to the polygon it is a part of
public:
	Ppolygon* pol;

	Corner(Eigen::Vector2d pos, Ppolygon* pol) : Point(pos) {
		//this->pos = pos;
		this->pol = pol;
	}
};

class Ppolygon : public Point {
public:
	//Eigen::Vector2d pos;
	Eigen::Matrix2d rot;
	std::vector<Corner*> corners;

	void Reposition(Eigen::Vector2d pos);
	void Rotate(Eigen::Matrix2d rot);

	Ppolygon(Eigen::Vector2d pos, Eigen::Matrix2d rot) : Point(pos) {
		//this->pos = pos;
		this->rot = rot;
	}
};

class Triangle : public Ppolygon {
public:
	double a;
	double b;
	double gamma; // in degree
	Eigen::Matrix2d gamma_rot;
	//void Reshape(double a, double b, double gamma);

	Triangle(double a, double b, double gamma, Eigen::Vector2d pos, Eigen::Matrix2d rot) : Ppolygon(pos, rot) {
		if(a == 0) this->a = 1;
		else this->a = a;
		if(b == 0) this->b = 1;
		else this->b = b;
		if(gamma <= 0 || gamma >= 180) this->gamma = 90;
		else this->gamma = gamma;
		//M_PI
		gamma_rot = Rotation(M_PI/180*this->gamma);
		corners.push_back(new Corner(pos, this));
		Eigen::Vector2d p3(this->b, 0);
		p3 = (rot * (gamma_rot * p3)) + pos;
		corners.push_back(new Corner(p3, this));
		Eigen::Vector2d p2(this->a, 0);
		p2 = (rot * p2) + pos;
		corners.push_back(new Corner(p2, this));
	}

	void Reshape(double a, double b, double gamma) {
		if(a == 0) this->a = 1;
		else this->a = a;
		if(b == 0) this->b = 1;
		else this->b = b;
		if(gamma <= 0 || gamma >= 180) this->gamma = 90;
		else this->gamma = gamma;
		gamma_rot = Rotation(M_PI/180*this->gamma);
		Eigen::Vector2d p3(this->b, 0);
		p3 = (rot * (gamma_rot * p3)) + pos;
		corners.at(1)->pos = p3;
		Eigen::Vector2d p2(this->a, 0);
		p2 = (rot * p2) + pos;
		corners.at(2)->pos = p2;
	}
};

class Rrectangle : public Ppolygon {
public:
	double hl;	//half length
	double hw;	//half width
	double hdiag;
	//Eigen::Vector2d pos;	//center
	//Eigen::Matrix2d rot;
	//std::vector<Corner> corners;

	Rrectangle(double hl, double hw, Eigen::Vector2d pos, Eigen::Matrix2d rot);

	//void Reposition(Eigen::Vector2d pos);
	//void Rotate(Eigen::Matrix2d rot);
	void Reshape(double hl, double hw);
};

enum TRI_AXIS {
	TRI_A,
	TRI_B,
	TRI_GAMMA
};

enum REC_AXIS {
	REC_AXIS_WIDTH,
	REC_AXIS_HEIGHT
};

Rrectangle::Rrectangle(double hl, double hw, Eigen::Vector2d pos, Eigen::Matrix2d rot) : Ppolygon(pos, rot) {
	this->hl = hl;
	this->hw = hw;
	this->hdiag = pow(hw*hw + hl*hl, 0.5);
	//this->pos = pos;
	//this->rot = rot;
	Eigen::Vector2d cornerPos;
	cornerPos << -hw, hl; cornerPos = rot*cornerPos + pos;
	corners.push_back(new Corner(cornerPos, dynamic_cast<Ppolygon*>(this)));
	cornerPos << hw, hl; cornerPos = rot*cornerPos + pos;
	corners.push_back(new Corner(cornerPos, this));
	cornerPos << hw, -hl; cornerPos = rot*cornerPos + pos;
	corners.push_back(new Corner(cornerPos, this));
	cornerPos << -hw, -hl; cornerPos = rot*cornerPos + pos;
	corners.push_back(new Corner(cornerPos, this));
}

void Ppolygon::Reposition(Eigen::Vector2d pos) {
	Eigen::Vector2d dist = pos - this->pos;
	for(auto corner : corners) {
		corner->pos += dist;
	}
	this->pos += dist;
}

void Ppolygon::Rotate(Eigen::Matrix2d rot) {
	Eigen::Matrix2d rotDiff = rot * this->rot.transpose();
	for(auto corner : corners) {
		corner->pos = rotDiff * (corner->pos - pos) + pos;
	}
	this->rot = rot;
}

void Rrectangle::Reshape(double hl, double hw) {
	Eigen::Vector2d cornerPos;
	cornerPos << -hw, hl; cornerPos = rot*cornerPos + pos;
	corners.at(0)->pos = cornerPos;
	cornerPos << hw, hl; cornerPos = rot*cornerPos + pos;
	corners.at(1)->pos = cornerPos;
	cornerPos << hw, -hl; cornerPos = rot*cornerPos + pos;
	corners.at(2)->pos = cornerPos;
	cornerPos << -hw, -hl; cornerPos = rot*cornerPos + pos;
	corners.at(3)->pos = cornerPos;
	this->hl = hl;
	this->hw = hw;
}

Eigen::Vector2d LineLineIntersect(Point* p00, Point* p01, Point* p10, Point* p11) {
	Eigen::Vector2d intersect;

	return intersect;
}


// Collision detection functions
bool LineLineCollision(Point* p00, Point* p01, Point* p10, Point* p11) {
	//Bezier parameters
	/*double t_numerator = (p00->pos.coeff(0) - p10->pos.coeff(0))*(p10->pos.coeff(1) - p11->pos.coeff(1))
						- (p00->pos.coeff(1) - p10->pos.coeff(1))*(p10->pos.coeff(0) - p11->pos.coeff(0));
	double t_denominator = (p00->pos.coeff(0) - p01->pos.coeff(0))*(p10->pos.coeff(1) - p11->pos.coeff(1))
						- (p00->pos.coeff(1) - p01->pos.coeff(1))*(p10->pos.coeff(0) - p11->pos.coeff(0));
	double u_numerator = (p00->pos.coeff(0) - p10->pos.coeff(0))*(p00->pos.coeff(1) - p01->pos.coeff(1))
						- (p00->pos.coeff(1) - p10->pos.coeff(1))*(p00->pos.coeff(0) - p01->pos.coeff(0));
	double u_denominator = (p00->pos.coeff(0) - p01->pos.coeff(0))*(p10->pos.coeff(1) - p11->pos.coeff(1))
						- (p00->pos.coeff(1) - p01->pos.coeff(1))*(p10->pos.coeff(0) - p11->pos.coeff(0));
	bool t_intersect = false;
	bool u_intersect = false;
	if((t_numerator < 0 && t_denominator < 0) || (0 <= t_numerator && 0 <= t_denominator)) {
		if(std::abs(t_denominator) >= std::abs(t_numerator)) t_intersect = true;
	}
	if((u_numerator < 0 && u_denominator < 0) || (0 <= u_numerator && 0 <= u_denominator)) {
		if(std::abs(u_denominator) >= std::abs(u_numerator)) u_intersect = true;
	}
	return t_intersect && u_intersect;*/
	Eigen::Vector2d p2 = p01->pos - p00->pos;
	Eigen::Vector2d p3 = p10->pos - p00->pos;
	Eigen::Vector2d p4 = p11->pos - p00->pos;
	Eigen::Matrix2d rot = Rotation(-Angle(p2.coeff(1) / p2.norm(), p2.coeff(0) / p2.norm()));
	p2 = rot * p2;
	p3 = rot * p3;
	p4 = rot * p4;
	if(p3.coeff(1) == p4.coeff(1)) {
		// special case, all ends should return
		if(p3.coeff(1) == 0) {
			if(std::min(p3.coeff(0), p4.coeff(0)) <= 0 && 
				0 <= std::max(p3.coeff(0), p4.coeff(0)))
				return true;
			else if(std::min(p3.coeff(0), p4.coeff(0)) <= p2.coeff(0) && 
				p2.coeff(0) <= std::max(p3.coeff(0), p4.coeff(0)))
				return true;
			else return false;
		}
		else return false;
	}
	double xIntersect = p3.coeff(0);
	if(p3.coeff(0) != p4.coeff(0)) {
		xIntersect -= p3.coeff(1) * (p4.coeff(0) - p3.coeff(0)) / (p4.coeff(1) - p3.coeff(1));
	}
	return xIntersect >= 0 && xIntersect <= p2.coeff(0)
		&& xIntersect >= std::min(p3.coeff(0),p4.coeff(0))
		&& xIntersect <= std::max(p3.coeff(0), p4.coeff(0));
}

bool LineCircleCollision(Point* l1, Point* l2, Circle* circ) {
	//if(l1->pos == circ->pos) return true; // prevents division by 0
	/*else {
		Eigen::Vector2d va = l2->pos - l1->pos;
		Eigen::Vector2d vb = circ->pos - l1->pos;
		double van = va.norm();
		double crossProd = std::abs(va.coeff(0)*vb.coeff(1) - va.coeff(1)*vb.coeff(0));
		double dotProd = va.dot(vb);
		if(circ->rad >= crossProd / van && dotProd >= 0 && dotProd <= 1)
			return true;
		return circ->rad >= vb.norm() || circ->rad >= (circ->pos - l2->pos).norm();
	}*/
	Eigen::Matrix2d rot;
	Eigen::Vector2d l = l2->pos - l1->pos;
	Eigen::Vector2d lc = circ->pos - l1->pos;
	double L = l.norm();
	double sin = l.coeff(1) / L;
	double cos = l.coeff(0) / L;
	rot << cos, -sin, sin, cos;
	//Eigen::Vector2d lrot = rot.transpose()*l;
	Eigen::Vector2d lcrot = rot.transpose()*lc;
	if(0 < lcrot.coeff(0) && lcrot.coeff(0) < L)
		return abs(lcrot.coeff(1)) < circ->rad;
	else {
		return lc.norm() < circ->rad || (circ->pos - l2->pos).norm() < circ->rad;
	}
}


bool PointBelowLine(Point* p, Point* l1, Point* l2) {	// checks the direction of the cross product (l2-l1) x (p-l1)
	double Sine = (l2->pos.coeff(0) - l1->pos.coeff(0))*(p->pos.coeff(1) - l1->pos.coeff(1))
		- (l2->pos.coeff(1) - l1->pos.coeff(1))*(p->pos.coeff(0) - l1->pos.coeff(0));
	return Sine < 0;
}

bool PointPolygonCollision(Point* p, Ppolygon* pol) {
	for(int nCorner = 0; nCorner < pol->corners.size(); nCorner++) {
		if(!PointBelowLine(p, pol->corners.at(nCorner), pol->corners.at((nCorner+1)%pol->corners.size())))
			return false;
	}
	return true;
}

bool PointRectangleCollision(Point* p, Rrectangle* rec) {
	return PointPolygonCollision(p, rec);
	/*Eigen::Vector2d rotPos = rec->rot.transpose() * (p->pos - rec->pos);
	return -rec->hw <= rotPos.coeff(0) && rotPos.coeff(0) <= rec->hw 
		&& -rec->hl <= rotPos.coeff(1) && rotPos.coeff(1) <= rec->hl;*/
}

bool LinePolygonCollision(Point* l1, Point* l2, Ppolygon* pol) {
	if(PointPolygonCollision(l1, pol)) return true;
	if(PointPolygonCollision(l2, pol)) return true;
	for(int nCorner = 0; nCorner < pol->corners.size() * 0.5; nCorner++) {
		if(LineLineCollision(l1, l2, 
			pol->corners.at(nCorner), pol->corners.at((nCorner + pol->corners.size()/2)%pol->corners.size())))
			return true;
	}
	return false;
}

bool LineRectangleCollison(Point* l1, Point* l2, Rrectangle* rec) {
	// either one of the points is in the rectangle
	if(PointPolygonCollision(l1, rec)) return true;
	if(PointPolygonCollision(l2, rec)) return true;
	// check one diagonal from every point for crossing the line
	if(LineLineCollision(l1, l2, (rec->corners.at(0)), (rec->corners.at(2)))) return true;
	if(LineLineCollision(l1, l2, (rec->corners.at(1)), (rec->corners.at(3)))) return true;
	return false;
}

bool CirclePolygonCollision(Circle* circ, Ppolygon* pol) {
	// Either circle center is in polygon
	if(PointPolygonCollision(circ, pol)) {
		return true;
	}
	/*for(int nCorner = 0; nCorner < pol->corners.size(); nCorner++) {
		if(!PointBelowLine(circ, pol->corners.at(nCorner), pol->corners.at((nCorner+1)%pol->corners.size()))) {
			//std::cout << "line-circle-collision\n";
			return false;
		}
	}*/
	// or circle must touch a line of the polygon
	for(int nCorner = 0; nCorner < pol->corners.size(); nCorner++) {
		if(LineCircleCollision(pol->corners.at(nCorner), pol->corners.at((nCorner+1)%pol->corners.size()), circ))
			return true;
	}
	return false;
}

bool CircleRectangleCollision(Circle* circle, Rrectangle* rec) {
	return CirclePolygonCollision(circle, rec);
	/*Eigen::Vector2d rotCPos = rec->rot.transpose() * (circle->pos - rec->pos);
	if(-(rec->hw + circle->rad) <= rotCPos.coeff(0) && rotCPos.coeff(0) <= (rec->hw + circle->rad) 
		&& -(rec->hl) <= rotCPos.coeff(1) && rotCPos.coeff(1) <= (rec->hl)) {
		return true;
	}
	if(-(rec->hw) <= rotCPos.coeff(0) && rotCPos.coeff(0) <= (rec->hw) 
		&& -(rec->hl + circle->rad) <= rotCPos.coeff(1) && rotCPos.coeff(1) <= (rec->hl + circle->rad)) {
		return true;
	}
	for(auto corner : rec->corners) {
		if((circle->pos - corner.pos).norm() <= (circle->rad)) {
			return true;
		}
	}*/

	/*if(LineCircleCollision(&rec->corners.at(0), &rec->corners.at(1), circle)) {
		std::cout << "edge 01\n";
		return true;
	}
	if(LineCircleCollision(&rec->corners.at(1), &rec->corners.at(2), circle)) {
		std::cout << "edge 12\n";
		return true;
	}
	if(LineCircleCollision(&rec->corners.at(2), &rec->corners.at(3), circle)) {
		std::cout << "edge 23\n";
		return true;
	}
	if(LineCircleCollision(&rec->corners.at(3), &rec->corners.at(0), circle)) {
		std::cout << "edge 30\n";
		return true;
	}
	if(PointPolygonCollision(circle, rec)) {
		std::cout << "inside\n";	
		return true;
	}
	return false;*/
}

bool PolygonPolygonCollision(Ppolygon* pol1, Ppolygon* pol2) {
	for(auto corner : pol1->corners) {
		if(PointPolygonCollision(corner, pol2)) return true;
	}
	for(auto corner : pol2->corners) {
		if(PointPolygonCollision(corner, pol1)) return true;
	}
	// check one diagonal connected to each corner
	for(int nCorner1 = 0; nCorner1 < pol1->corners.size() * 0.5; nCorner1++) {
		for(int nCorner2 = 0; nCorner2 < pol2->corners.size(); nCorner2++) {
			if(LineLineCollision(
				pol1->corners.at(nCorner1), pol1->corners.at((nCorner1 + pol1->corners.size()/2)%pol1->corners.size()),
				pol2->corners.at(nCorner2), pol2->corners.at((nCorner2+1)%pol2->corners.size())
				))
				return true;
		}
	}
	// or check every edge with every edge of the other polygon (more expensive)
	/*for(int nCorner1 = 0; nCorner1 < pol1->corners.size(); nCorner1++) {
		for(int nCorner2 = 0; nCorner2 < pol2->corners.size(); nCorner2++) {
			if(LineLineCollision(
			pol1->corners.at(nCorner1), pol1->corners.at((nCorner1 + 1)%pol1->corners.size()),
			pol2->corners.at(nCorner2), pol2->corners.at((nCorner2 + 1)%pol2->corners.size())
			)) {
				return true;
			}
		}
	}*/
	return false;
}

bool RectangleRectangleCollision(Rrectangle* rec1, Rrectangle* rec2) {
	return PolygonPolygonCollision(rec1, rec2);
	// below is wrong
	for(auto corner : rec1->corners) {
		if(PointRectangleCollision(corner, rec2)) {return true;}
	}
	for(auto corner : rec2->corners) {
		if(PointRectangleCollision(corner, rec1)) {return true;}
	}
	if(LineLineCollision((rec1->corners.at(3)), (rec1->corners.at(1)), (rec2->corners.at(3)), (rec2->corners.at(1)))) {
		return true;
	}
	return false;
}

bool CircleCircleCollision(Circle* circ1, Circle* circ2) {
	return (circ1->pos - circ2->pos).norm() < circ1->rad + circ2->rad;
}

bool LenientCircleCircleCollision(Circle* circ1, Circle* circ2) {
	return (circ1->pos - circ2->pos).norm() < (circ1->rad + circ2->rad)*0.99;
}

bool LenientToughCircleRectangleCollision(Circle* circ, Rrectangle* rec) {
	bool collision = CircleRectangleCollision(circ, rec);
	if(collision) return true;
	else {
		double diag = std::sqrt(2 * circ->rad * circ->rad);
		for(auto corner : rec->corners) {
			if((circ->pos - corner->pos).norm() < diag * 0.95) return true;
		}
		return false;
	}
}

bool ConeCircleCollision(Eigen::Vector2d conePos, Eigen::Matrix2d coneRot, Eigen::Matrix2d coneAngle, double coneRad, Circle* circ) {
	Eigen::Vector2d base; base << 1, 0;
	Eigen::Vector2d relPL = coneAngle*coneRot * base;
	Eigen::Vector2d relPR = coneAngle.transpose()*coneRot * base;
	Eigen::Vector2d relCPos = circ->pos - conePos;
	if(-relPL.coeff(1)*relCPos.coeff(0) + relPL.coeff(0)*relCPos.coeff(1) < 0
		&&
		-relPR.coeff(1)*relCPos.coeff(0) + relPR.coeff(0)*relCPos.coeff(1) > 0
		)
		return true;

	Point p0(conePos);
	Point p1(conePos + coneRad*relPL);
	Point p2(conePos + coneRad*relPR);
	return LineCircleCollision(&p0, &p1, circ) || LineCircleCollision(&p0, &p2, circ);
	return false;
}

#endif