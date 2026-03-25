#include "SignalProcessing.hpp"

namespace AppLayer
{

LowPassFilter::LowPassFilter(float alpha)
: alpha(alpha)
, output(0.0f)
, initialized(false)
{
    if (this->alpha < 0.0f) this->alpha = 0.0f;
    if (this->alpha > 1.0f) this->alpha = 1.0f;
}

void LowPassFilter::Reset(float initialValue)
{
    output = initialValue;
    initialized = true;
}

float LowPassFilter::Update(float input)
{
    if (!initialized)
    {
        output = input;
        initialized = true;
        return output;
    }

    output = alpha * input + (1.0f - alpha) * output;
    return output;
}

float LowPassFilter::Get() const
{
    return output;
}

float LowPassFilter::GetAlpha() const
{
    return alpha;
}

void LowPassFilter::SetAlpha(float alpha)
{
    if (alpha < 0.0f) alpha = 0.0f;
    if (alpha > 1.0f) alpha = 1.0f;
    this->alpha = alpha;
}

} // namespace AppLayer
