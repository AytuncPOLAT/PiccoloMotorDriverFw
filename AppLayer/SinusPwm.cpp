#include "SinusPwm.hpp"
#include "SystemData.hpp"
#include "math.h"

using namespace AppLayer;

float phaseAngle_a;
float phaseAngle_b;
float phaseAngle_c;

namespace
{
	constexpr uint16_t COMMUTATION_360DEG = Common::COUNT_PER_REV / Common::MOTOR_POLES;
	constexpr uint16_t COMMUTATION_120DEG = (Common::COUNT_PER_REV / Common::MOTOR_POLES) / 3;
	constexpr float RAD_2PI = 2.0 * M_PI;
	constexpr float RAD_120 = RAD_2PI / 3.0;
}

SinusPwm::SinusPwm()
{

}

TriPhase SinusPwm::Update3P(int16_t amplitude, float angleInRad)
{
	float amp = amplitude;
	phaseAngle_a = angleInRad;

	phaseAngle_b = phaseAngle_a + RAD_120;
	if(phaseAngle_b > RAD_2PI) phaseAngle_b = phaseAngle_b - RAD_2PI;

	phaseAngle_c = phaseAngle_b + RAD_120;
	if(phaseAngle_c > RAD_2PI) phaseAngle_c = phaseAngle_c - RAD_2PI;

	uint16_t phaseA, phaseB, phaseC;

	phase.a = (511.0 + (amp * std::sin(phaseAngle_a)));
	phase.b = (511.0 + (amp * std::sin(phaseAngle_b)));
	phase.c = (511.0 + (amp * std::sin(phaseAngle_c)));

	return phase;
}
