#pragma once

#include <Arduino.h>

namespace Comms {

void beginAccessPoint();
bool isWemosKnown();
bool receiveLine(char* buffer, size_t bufferSize);
void sendTelem(const char* type, const char* value);

}  // namespace Comms
