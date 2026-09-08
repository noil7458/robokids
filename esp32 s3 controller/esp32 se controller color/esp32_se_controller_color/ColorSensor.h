#pragma once

#include <Arduino.h>

namespace ColorSensor {

void begin();
bool isOk();
void update();
bool consumeNewReading(int& r, int& g, int& b, char* colorName, size_t colorNameSize);
int red();
int green();
int blue();
bool isColor(const char* protocolColor);
const char* name();

}  // namespace ColorSensor
