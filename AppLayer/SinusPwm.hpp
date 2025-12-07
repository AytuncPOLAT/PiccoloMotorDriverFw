#ifndef SINUS_PWM_HPP
#define SINUS_PWM_HPP

#include "SineLut.hpp"
#include <cstdint>

namespace AppLayer
{
	struct TriPhase
	{
		int a,b,c;
	};

	class SinusPwm
	{
	public:
		SinusPwm();
		TriPhase Update3P(int16_t amplitude, uint16_t angle);

	private:
		TriPhase phase;
	};
}

#endif //SINUS_PWM_HPP
