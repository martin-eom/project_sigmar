#ifndef CUSTOM_RNG
#define CUSTOM_RNG

#include <random>

class RNG {
public:
	static std::mt19937& get() {
		static std::mt19937 gen{std::random_device{}()};
		return gen;
	}

	static int uniformInt(int min, int max) {
		std::uniform_int_distribution<int> dist(min, max);
		return dist(get());
	}

	static double uniformDouble(double min, double max) {
		std::uniform_real_distribution<double> dist(min, max);
		return dist(get());
	}
};

#endif