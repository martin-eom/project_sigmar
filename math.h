#define MATH

#ifndef _USE_MATH_DEFINES
#define _USE_MATH_DEFINES
#endif

double Angle(double sin, double cos) {
	if(sin > 0) {
		return acos(cos);
	}
	else {
		return -acos(cos);
	}
}
