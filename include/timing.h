#include <chrono>

//typedef void (* TimeableFunctionCall)();
//using TimeableFunctionCall = void (*)();

//auto TimeFunction(TimeableFunctionCall func) {
auto TimeFunction(const std::function<void()>& func) {
	auto start = std::chrono::system_clock::now();
	func();
	auto end = std::chrono::system_clock::now();
	return std::chrono::duration<double>(end - start).count();
}

/*template <typename Func>
void MeasureTimeFunction(Func&& func) {
	auto start = std::chrono::system_clock::now();
	func();
	auto end = std::chrono::system_clock::now();
	return std::chrono::duration<double>(end - start).count();
}*/