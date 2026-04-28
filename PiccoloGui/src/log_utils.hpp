#pragma once

#include <string>
#include <vector>

std::string timestampNow();
void addLog(std::vector<std::string>& logs, const std::string& line);

bool parseTelemetryCsv(const std::string& line, double& speedRps, double& currentA, double& positionDeg, double& busVoltage, double& pwmPercent, double& driverTemp, double& motorTemp);
