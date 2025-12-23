#include "PidControl.hpp"

using namespace AppLayer;

PidController::PidController()
{}

void PidController::SetParameters(float _kp, float _ki, float _kd, float _pidOutputLimit, float _windUpLimit)
{
	kp = _kp;
	ki = _ki;
	kd = _kd;
	pidLimit = _pidOutputLimit;
	integralWindUpLimit = _windUpLimit;
}

float PidController::Calculate(float input, float setPoint)
{
	error = setPoint - input;

	p = kp * error;

	errorDelta = error - errorOld;
	d = errorDelta * kd;

	dFilter = dFilter*0.99 + d*0.01;
			
	errorOld = error;		

	errorSum += error;
	i = errorSum * ki;
	
	if(errorSum > integralWindUpLimit) errorSum = integralWindUpLimit;		
	else if(errorSum < -integralWindUpLimit)errorSum = -integralWindUpLimit;

	pidOutput = p + i + dFilter;		

	if(pidOutput > pidLimit) pidOutput = pidLimit;
	else if(pidOutput < -pidLimit) pidOutput = -pidLimit;	  

	return pidOutput;
}




