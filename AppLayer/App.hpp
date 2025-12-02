#ifndef APP_HPP
#define APP_HPP

#include "Hardware.hpp"

class App
{
public:
	App(Hardware &hardwareRef);
private:
	Hardware &hardware;
};

#endif // APP_HPP
