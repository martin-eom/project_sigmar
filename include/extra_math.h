#ifndef EXTRA_MATH
#define EXTRA_MATH

#ifndef _USE_MATH_DEFINES
#define _USE_MATH_DEFINES
#endif

#include <Dense>

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
};

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

class Rrectangle;

class Corner : public Point {
public:
	Rrectangle* rec;

	Corner(Eigen::Vector2d pos, Rrectangle* rec) : Point(pos) {
		this->pos = pos;
		this->rec = rec;
	}
};

class Rrectangle {
public:
	double hl;	//half length
	double hw;	//half width
	Eigen::Vector2d pos;	//center
	Eigen::Matrix2d rot;
	std::vector<Corner*> corners;

	Rrectangle(double hl, double hw, Eigen::Vector2d pos, Eigen::Matrix2d rot);

	void Reposition(Eigen::Vector2d pos);
	void Rotate(Eigen::Matrix2d rot);
	void Reshape(double hl, double hw);
};

enum REC_AXIS {
	REC_AXIS_WIDTH,
	REC_AXIS_HEIGHT
};

Rrectangle::Rrectangle(double hl, double hw, Eigen::Vector2d pos, Eigen::Matrix2d rot) {
	this->hl = hl;
	this->hw = hw;
	this->pos = pos;
	this->rot = rot;
	Eigen::Vector2d cornerPos;
	cornerPos << -hw, hl; cornerPos = rot*cornerPos + pos;
	corners.push_back(new Corner(cornerPos, this));
	cornerPos << hw, hl; cornerPos = rot*cornerPos + pos;
	corners.push_back(new Corner(cornerPos, this));
	cornerPos << hw, -hl; cornerPos = rot*cornerPos + pos;
	corners.push_back(new Corner(cornerPos, this));
	cornerPos << -hw, -hl; cornerPos = rot*cornerPos + pos;
	corners.push_back(new Corner(cornerPos, this));
}

void Rrectangle::Reposition(Eigen::Vector2d pos) {
	Eigen::Vector2d dist = pos - this->pos;
	for(auto corner : corners) {
		corner->pos += dist;
	}
	this->pos += dist;
}

void Rrectangle::Rotate(Eigen::Matrix2d rot) {
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


// Collision detection functions
bool LineLineCollision(Point* p00, Point* p01, Point* p10, Point* p11) {
	//Bezier parameters
	double t_numerator = (p00->pos.coeff(0) - p10->pos.coeff(0))*(p10->pos.coeff(1) - p11->pos.coeff(1))
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
	return t_intersect && u_intersect;
}

bool LineCircleCollision(Point* l1, Point* l2, Circle* circ) {
	if(l1->pos == circ->pos) return true; // prevents division by 0
	else {
		Eigen::Vector2d va = l2->pos - l1->pos;
		Eigen::Vector2d vb = circ->pos - l1->pos;
		double van = va.norm();
		double crossProd = std::abs(va.coeff(0)*vb.coeff(1) - va.coeff(1)*vb.coeff(0));
		double dotProd = va.dot(vb);
		if(circ->rad >= crossProd / van && dotProd >= 0 && dotProd <= 1)
			return true;
		return circ->rad >= vb.norm() || circ->rad >= (circ->pos - l2->pos).norm();
	}
}

bool LineRectangleCollison(Point* l1, Point* l2, Rrectangle* rec) {
	if(LineLineCollision(l1, l2, rec->corners.at(0), rec->corners.at(2))) return true;
	if(LineLineCollision(l1, l2, rec->corners.at(1), rec->corners.at(3))) return true;
	return false;
}

bool PointRectangleCollision(Point* p, Rrectangle* rec) {
	Eigen::Vector2d rotPos = rec->rot.transpose() * (p->pos - rec->pos);
	return -rec->hw <= rotPos.coeff(0) && rotPos.coeff(0) <= rec->hw 
		&& -rec->hl <= rotPos.coeff(1) && rotPos.coeff(1) <= rec->hl;
}

bool CircleRectangleCollision(Circle* circle, Rrectangle* rec) {
	Eigen::Vector2d rotCPos = rec->rot.transpose() * (circle->pos - rec->pos);
	if(-(rec->hw + circle->rad) <= rotCPos.coeff(0) && rotCPos.coeff(0) <= (rec->hw + circle->rad) 
		&& -(rec->hl) <= rotCPos.coeff(1) && rotCPos.coeff(1) <= (rec->hl)) {
		return true;
	}
	if(-(rec->hw) <= rotCPos.coeff(0) && rotCPos.coeff(0) <= (rec->hw) 
		&& -(rec->hl + circle->rad) <= rotCPos.coeff(1) && rotCPos.coeff(1) <= (rec->hl + circle->rad)) {
		return true;
	}
	for(auto corner : rec->corners) {
		if((circle->pos - corner->pos).norm() <= (circle->rad)) {
			return true;
		}
	}
	return false;
}

bool RectangleRectangleCollision(Rrectangle* rec1, Rrectangle* rec2) {
	for(auto corner : rec1->corners) {
		if(PointRectangleCollision(corner, rec2)) {return true;}
	}
	for(auto corner : rec2->corners) {
		if(PointRectangleCollision(corner, rec1)) {return true;}
	}
	if(LineLineCollision(rec1->corners.at(3), rec1->corners.at(1), rec2->corners.at(3), rec2->corners.at(1))) {
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