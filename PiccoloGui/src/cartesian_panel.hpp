#pragma once

#include "app_models.hpp"
#include <functional>

// Draws a cartesian coordinate panel with a draggable circle and Z slider.
// The supplied callback, if non-null, is called every frame with the current
// world coordinates (x,y,z) of the circle in the range [-1000,1000].
void drawCartesianPanel(std::function<void(float, float, float)> updateCallback = nullptr);
