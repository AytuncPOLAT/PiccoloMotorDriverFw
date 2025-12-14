#ifndef APP_HPP
#define APP_HPP

#include "Hardware.hpp"
#include "SimpleLogger.hpp"
#include "Communication.hpp"

class App
{
public:
	App(Hardware &hardware);
	AppLayer::SimpleLogger simpleLogger;
	AppLayer::Communication communication;

	Hardware& hw;
private:
};

#endif // APP_HPP
