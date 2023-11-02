// screw onedrive

#ifndef TIMER
#define TIMER

class Timer {
private:
	int max;	// max number of ticks
	int current;// current tick

public:
	void reset() {
		current = max;
	}

	void unset() {
		current = 0;
	}

	void set_max(int max) {
		this->max = max;
		reset();
	}

	bool done() {
		return current < 0;
	}

	bool decrement() {
		if(done()) {
			return true;
		}
		current--;
		return false;
	}

	int get_max() {
		return max;
	}

	int get_current() {
		return current;
	}

	//Constructor
	Timer(int max) {
		set_max(max);
		reset();
	}

	Timer() : Timer(0) {}
};

#endif
