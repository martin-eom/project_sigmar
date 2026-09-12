#ifndef LOGGER
#define LOGGER

#include <fstream>
#include <string>
#include <iostream>

namespace Log {
	inline std::ofstream _file;

	inline void Init(const std::string& filename = "game.log") {
		std::ifstream check(filename);
		bool existed = check.good();
		check.close();
		_file.open(filename, std::ios::app);
		if(!_file.is_open()) {
			std::cout << "[Log] Could not open log file " << filename << ", falling back to terminal.\n";
			return;
		}
		_file << (existed ? "--- log resumed ---" : "--- log created ---") << std::endl;
	}

	inline void Write(const std::string& text) {
		if(_file.is_open()) {
			_file << text << std::endl; // flushes so the line survives an abrupt crash
		}
		else {
			std::cout << text << "\n";
		}
	}
}

#endif
