#pragma once

#include <Arduino.h>

namespace Motors {

void begin();
void motorOff();
void moveSteps(long steps, bool fwd, int vel);
void turnSteps(long steps, bool cw, int vel);
void tankMove(float vA, float vB, float secs);
void resetEncoders();
long getEncoderA();
long getEncoderB();
void getEncoderDegrees(int& degA, int& degB);

}  // namespace Motors
