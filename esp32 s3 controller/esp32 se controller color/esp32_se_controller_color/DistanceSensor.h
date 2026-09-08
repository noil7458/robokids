#pragma once

namespace DistanceSensor {

void begin();
bool isOk();
void update();
int getDistanceMM();

}  // namespace DistanceSensor
