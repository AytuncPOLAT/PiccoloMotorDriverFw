#include "SinusPwm.hpp"
#include "SystemData.hpp"

using namespace AppLayer;

namespace
{
	constexpr uint16_t COMMUTATION_360DEG = Common::COUNT_PER_REV / Common::MOTOR_POLES;
	constexpr uint16_t COMMUTATION_120DEG = (Common::COUNT_PER_REV / Common::MOTOR_POLES) / 3;
}

SinusPwm::SinusPwm()
{

}

TriPhase SinusPwm::Update3P(int16_t amplitude, uint16_t angle)
{
	uint16_t phaseA, phaseB, phaseC;

	phaseA = (angle % COMMUTATION_360DEG);
	phaseB = (phaseA + COMMUTATION_120DEG) % COMMUTATION_360DEG;
	phaseC = (phaseB + COMMUTATION_120DEG) % COMMUTATION_360DEG;

	if(amplitude > 1024)amplitude = 1024;
    if(amplitude < -1024)amplitude = -1024;

	int offsetA = (LUT::sineLut585[phaseA] - 511) * amplitude;
	int offsetB = (LUT::sineLut585[phaseB] - 511) * amplitude;
	int offsetC = (LUT::sineLut585[phaseC] - 511) * amplitude;

	offsetA = offsetA / 1024;
	offsetB = offsetB / 1024;
	offsetC = offsetC / 1024;

	phase.a = (511 + offsetC);
	phase.b = (511 + offsetB);
	phase.c = (511 + offsetA);

	return phase;
}
