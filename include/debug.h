#ifndef DEBUG
#define DEBUG

#include <iostream>
#include <string>

namespace ddebug {
	bool _showDebugMessages = false;
	bool _showDebugGraphics = false;
}


void debug(std::string text) {
	if(ddebug::_showDebugMessages) {
		std::cout << "[DEBUG:] " << text << "\n";
	}
}

#endif