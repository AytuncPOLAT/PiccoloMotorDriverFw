#include "PidControl.hpp"

using namespace AppLayer;

PidController::PidController()
: dFilter(0.1f)
{}

void PidController::SetParameters(float _kp, float _ki, float _kd, float _windUpLimit, float _pidOutputLimit)
{
	kp = _kp;
	ki = _ki;
	kd = _kd;
	integralWindUpLimit = _windUpLimit;
	pidLimit = _pidOutputLimit;
}

float PidController::Calculate(float input, float setPoint)
{
	error = setPoint - input;

	p = kp * error;

	errorDelta = error - errorOld;
	d = errorDelta * kd;

	float dFiltered = dFilter.Update(d);


	errorSum += error;
	i = errorSum * ki;
	
	if(errorSum > integralWindUpLimit)
		errorSum = integralWindUpLimit;
	else if(errorSum < -integralWindUpLimit)
		errorSum = -integralWindUpLimit;

	pidOutput = p + i + dFiltered;

	if(pidOutput > pidLimit)
		pidOutput = pidLimit;
	else if(pidOutput < -pidLimit)
		pidOutput = -pidLimit;

	errorOld = error;

	return pidOutput;
}

