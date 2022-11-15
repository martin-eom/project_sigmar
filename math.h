#define MATH

#ifndef _USE_MATH_DEFINES
#define _USE_MATH_DEFINES
#endif

#include <Dense>

double Angle(double sin, double cos) {
	if(sin > 0) {
		return acos(cos);
	}
	else {
		return -acos(cos);
	}
}

bool PointInRectangle(Eigen::Vector2d point, Eigen::Matrix2d rec, Eigen::Matrix2d rot) {	// rotation of the rectangle, point must be translated so that rectangle center is in 0 0
	Eigen::Vector2d p0, p1;
	p0 << rec.coeff(0,0), rec.coeff(0,1);
	p1 << rec.coeff(1,0), rec.coeff(1,1);
	Eigen::Vector2d rotPoint = rot.transpose() * point;
	return rec.coeff(0,0) <= rotPoint.coeff(0) && rotPoint.coeff(0) <= rec.coeff(1,0)
		&& rec.coeff(0,1) >= rotPoint.coeff(1) && rotPoint.coeff(1) >= rec.coeff(1,1);
}