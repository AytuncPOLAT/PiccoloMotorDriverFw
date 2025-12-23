#ifndef PID_CONTROL_HPP
#define PID_CONTROL_HPP

namespace AppLayer
{
	class PidController
	{
	public:
		PidController();
		void SetParameters(float _kp, float _ki, float _kd, float _pidOutputLimit, float _windUpLimit);
		float Calculate(float input, float setPoint);
		float error;

		float kp;
		float ki;
		float kd;
		float p;
		float i;
		float d;
		
		float dFilter;
		float errorDelta;
		float errorOld;
		float errorSum;
		float integralWindUpLimit;
		float pidLimit;
		float pidSetPoint;
		float pidInput;
		float pidOutput;
	};
}
#endif //PID_CONTROL_HPP
