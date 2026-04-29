#pragma once

#include <string>
#include <vector>

std::string timestampNow();
void addLog(std::vector<std::string>& logs, const std::string& line);
extern bool loggingEnabled;

bool parseTelemetryCsv(const std::string& line, double& encoder, double& speedRps, double& torque, double& busVoltage, double& pwmPercent, double& driverTemp, double& motorTemp);
