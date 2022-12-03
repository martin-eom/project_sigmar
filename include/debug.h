#ifndef DEBUG
#define DEBUG

#include <iostream>
#include <string>

bool _showDebugMessages = false;

void debug(std::string text) {
	if(_showDebugMessages) {
		std::cout << "[DEBUG:] " << text << "\n";
	}
}

#endif