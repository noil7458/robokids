#include "DistanceSensor.h"

#include "VL53L1X.h"

namespace {
VL53L1X distanceSensor;
bool sensorDistanzOk = false;
volatile int sDistanzMM = 0;
}

namespace DistanceSensor {

void begin() {
  distanceSensor.setTimeout(500);
  if (distanceSensor.init()) {
    distanceSensor.setDistanceMode(VL53L1X::Short);
    distanceSensor.setMeasurementTimingBudget(50000);
    distanceSensor.startContinuous(50);
    sensorDistanzOk = true;
  }
}

bool isOk() {
  return sensorDistanzOk;
}

void update() {
  if (!sensorDistanzOk) return;

  distanceSensor.read();
  if (!distanceSensor.timeoutOccurred()) {
    sDistanzMM = distanceSensor.ranging_data.range_mm;
  }
}

int getDistanceMM() {
  return sDistanzMM;
}

}  // namespace DistanceSensor
