#ifndef APP_HPP
#define APP_HPP

#include "Hardware.hpp"
#include "SimpleLogger.hpp"

class App
{
public:
	App(Hardware &hardware);
	AppLayer::SimpleLogger simpleLogger;

	Hardware& hw;
private:
};

#endif // APP_HPP
