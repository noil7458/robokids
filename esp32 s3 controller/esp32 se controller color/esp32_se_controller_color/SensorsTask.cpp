#include "SensorsTask.h"

#include <Arduino.h>
#include <Wire.h>

#include "ColorSensor.h"
#include "Config.h"
#include "DistanceSensor.h"

namespace {
void sensorTaskFn(void*) {
  vTaskDelay(pdMS_TO_TICKS(1000));
  for (;;) {
    ColorSensor::update();
    DistanceSensor::update();
    vTaskDelay(pdMS_TO_TICKS(500));
  }
}
}

namespace SensorsTask {

void begin() {
  Wire.begin(Config::I2C_SDA, Config::I2C_SCL);
  delay(100);

  ColorSensor::begin();
  Serial.println(ColorSensor::isOk() ? "AS7341 OK" : "AS7341 FEHLER");

  DistanceSensor::begin();
  Serial.println(DistanceSensor::isOk() ? "VL53L1X OK" : "VL53L1X FEHLER");

  xTaskCreatePinnedToCore(sensorTaskFn, "ColorTask", 4096, NULL, 1, NULL, 0);
}
}
  // namespace SensorsTask
