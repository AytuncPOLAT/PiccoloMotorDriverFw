#ifndef SIGNAL_PROCESSING_HPP
#define SIGNAL_PROCESSING_HPP

#include <cstdint>

namespace AppLayer
{

class LowPassFilter
{
public:
    // alpha should be in the range 0..1: 0 -> no update, 1 -> raw passthrough
    explicit LowPassFilter(float alpha = 0.1f);

    // Reset filter state (optional warm start)
    void Reset(float initialValue = 0.0f);

    // Process next sample and return filtered result
    float Update(float input);

    // Get current output without processing a new sample
    float Get() const;

    // Coefficient access
    float GetAlpha() const;
    void SetAlpha(float alpha);

private:
    float alpha;
    float output;
    bool initialized;
};

} // namespace AppLayer

#endif // SIGNAL_PROCESSING_HPP
