#pragma once

#include <Arduino.h>

namespace CommandHelpers {

String getP(const char* s, int idx);
void doWait(float secs);

}  // namespace CommandHelpers
