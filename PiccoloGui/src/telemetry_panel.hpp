#pragma once

#include "app_models.hpp"

class SerialManager;
struct ConnectionPanelState;

void drawTelemetryPanel(const TelemetryBuffer& telemetry, bool& armed, int status, bool& autoRefresh, int& refreshRateIndex, bool& loggingEnabled, SerialManager& serial, const ConnectionPanelState& connectionState);