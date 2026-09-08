#include "Motors.h"

#include <math.h>
#include <stdlib.h>

#include "Comms.h"
#include "Config.h"
#include "RuntimeState.h"

namespace {
volatile long encA = 0;
volatile long encB = 0;

void setPWM_A(int v) {
  ledcWrite(Config::PWMA, constrain(v, 0, 255));
}

void setPWM_B(int v) {
  ledcWrite(Config::PWMB, constrain(v, 0, 255));
}

void IRAM_ATTR isrA() {
  encA += (digitalRead(Config::ENC_A_A) == digitalRead(Config::ENC_A_B)) ? 1 : -1;
}

void IRAM_ATTR isrB() {
  encB += (digitalRead(Config::ENC_B_A) == digitalRead(Config::ENC_B_B)) ? 1 : -1;
}

int scalePWM(int v) {
  return map(constrain(v, 1, 10), 1, 10, 55, 255);
}

void motorOn() {
  digitalWrite(Config::STBY, HIGH);
}

void dirA(bool f) {
  digitalWrite(Config::AIN1, f ? LOW : HIGH);
  digitalWrite(Config::AIN2, f ? HIGH : LOW);
}

void dirB(bool f) {
  digitalWrite(Config::BIN1, f ? HIGH : LOW);
  digitalWrite(Config::BIN2, f ? LOW : HIGH);
}
}

namespace Motors {

void begin() {
  pinMode(Config::STBY, OUTPUT);
  digitalWrite(Config::STBY, LOW);
  pinMode(Config::AIN1, OUTPUT);
  pinMode(Config::AIN2, OUTPUT);
  pinMode(Config::BIN1, OUTPUT);
  pinMode(Config::BIN2, OUTPUT);
  digitalWrite(Config::AIN1, LOW);
  digitalWrite(Config::AIN2, LOW);
  digitalWrite(Config::BIN1, LOW);
  digitalWrite(Config::BIN2, LOW);

  ledcAttach(Config::PWMA, Config::PWM_FREQ, Config::PWM_RES);
  ledcAttach(Config::PWMB, Config::PWM_FREQ, Config::PWM_RES);
  ledcWrite(Config::PWMA, 0);
  ledcWrite(Config::PWMB, 0);

  pinMode(Config::ENC_A_A, INPUT_PULLUP);
  pinMode(Config::ENC_A_B, INPUT_PULLUP);
  pinMode(Config::ENC_B_A, INPUT_PULLUP);
  pinMode(Config::ENC_B_B, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(Config::ENC_A_A), isrA, CHANGE);
  attachInterrupt(digitalPinToInterrupt(Config::ENC_B_A), isrB, CHANGE);
}

void motorOff() {
  setPWM_A(0);
  setPWM_B(0);
  digitalWrite(Config::STBY, LOW);
}

void moveSteps(long steps, bool fwd, int vel) {
  int pwm = scalePWM(vel);
  resetEncoders();
  motorOn();
  dirA(fwd);
  dirB(fwd);
  setPWM_A(pwm);
  setPWM_B(pwm);
  Comms::sendTelem("STATUS", fwd ? "FORWARD" : "BACKWARD");

  while (!RuntimeState::isAbortRequested()) {
    long dA = labs(encA);
    long dB = labs(encB);
    if (dA >= steps && dB >= steps) break;

    long diff = dA - dB;
    setPWM_A(dA < steps ? constrain(pwm - (int)(diff * 0.3f), 40, 255) : 0);
    setPWM_B(dB < steps ? constrain(pwm + (int)(diff * 0.3f), 40, 255) : 0);
    vTaskDelay(pdMS_TO_TICKS(5));
  }
  motorOff();
}

void turnSteps(long steps, bool cw, int vel) {
  int pwm = scalePWM(vel);
  resetEncoders();
  motorOn();
  dirA(!cw);
  dirB(cw);
  setPWM_A(pwm);
  setPWM_B(pwm);
  Comms::sendTelem("STATUS", cw ? "TURN_RIGHT" : "TURN_LEFT");

  while (!RuntimeState::isAbortRequested() && (labs(encA) < steps || labs(encB) < steps)) {
    vTaskDelay(pdMS_TO_TICKS(5));
  }
  motorOff();
}

void tankMove(float vA, float vB, float secs) {
  motorOn();
  dirA(vA >= 0);
  dirB(vB >= 0);
  setPWM_A(scalePWM(constrain((int)fabsf(vA), 1, 10)));
  setPWM_B(scalePWM(constrain((int)fabsf(vB), 1, 10)));
  Comms::sendTelem("STATUS", "TANK");

  unsigned long t0 = millis();
  while (!RuntimeState::isAbortRequested() && (millis() - t0) < (unsigned long)(secs * 1000.0f)) {
    vTaskDelay(pdMS_TO_TICKS(5));
  }
  motorOff();
}

void resetEncoders() {
  encA = 0;
  encB = 0;
}

long getEncoderA() {
  return encA;
}

long getEncoderB() {
  return encB;
}

void getEncoderDegrees(int& degA, int& degB) {
  degA = (int)(((float)getEncoderA() / Config::PASOS_X_VUELTA) * 360.0f);
  degB = (int)(((float)getEncoderB() / Config::PASOS_X_VUELTA) * 360.0f);
}

}  // namespace Motors
