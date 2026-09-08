/*
 * ESP32S3 - Robot controller (motors, continuous degree telemetry, and color)
 */

#include <Arduino.h>
#include <string.h>

#include "ColorSensor.h"
#include "Comms.h"
#include "Config.h"
#include "DistanceSensor.h"
#include "Motors.h"
#include "ProgramRunner.h"
#include "SensorsTask.h"

unsigned long lastRxMs = 0;
bool receiving = false;

void setup() {
  Serial.begin(115200);
  delay(200);

  SensorsTask::begin();
  Motors::begin();
  Comms::beginAccessPoint();
  ProgramRunner::begin();
}

void loop() {
  if (Comms::isWemosKnown()) {
    int r = 0;
    int g = 0;
    int b = 0;
    char colorName[32];
    if (ColorSensor::consumeNewReading(r, g, b, colorName, sizeof(colorName))) {
      char buf[64];
      snprintf(buf, sizeof(buf), "%d,%d,%d,%s", r, g, b, colorName);
      Comms::sendTelem("COLOR", buf);
    }
  }

  static unsigned long lastDistMs = 0;
  if (DistanceSensor::isOk() && Comms::isWemosKnown() && (millis() - lastDistMs > 200)) {
    lastDistMs = millis();
    char buf[16];
    snprintf(buf, sizeof(buf), "%d", DistanceSensor::getDistanceMM());
    Comms::sendTelem("DIST", buf);
  }

  static unsigned long lastEncMs = 0;
  if (Comms::isWemosKnown() && (millis() - lastEncMs > 200)) {
    lastEncMs = millis();
    int degA = 0;
    int degB = 0;
    Motors::getEncoderDegrees(degA, degB);
    char buf[32];
    snprintf(buf, sizeof(buf), "%d,%d", degA, degB);
    Comms::sendTelem("ENC", buf);
  }

  char line[Config::MAX_LINE_LEN];
  if (Comms::receiveLine(line, sizeof(line))) {
    if (strcmp(line, "STOP") == 0) {
      ProgramRunner::stopNow();
      receiving = false;
      vTaskDelay(pdMS_TO_TICKS(5));
      return;
    }

    if (!ProgramRunner::isDone()) {
      vTaskDelay(pdMS_TO_TICKS(5));
      return;
    }

    ProgramRunner::addLine(line, !receiving);
    receiving = true;
    lastRxMs = millis();
  }

  if (receiving && ProgramRunner::isDone() && (millis() - lastRxMs) > Config::RX_TIMEOUT_MS) {
    receiving = false;
    ProgramRunner::startBufferedProgram();
  }

  vTaskDelay(pdMS_TO_TICKS(5));
}
