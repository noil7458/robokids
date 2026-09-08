#pragma once

#include <Arduino.h>

namespace Config {

extern const char* const AP_SSID;
extern const char* const AP_PASS;
extern const uint16_t UDP_LISTEN_PORT;
extern const uint16_t UDP_TELEM_PORT;

constexpr int I2C_SDA = 15;
constexpr int I2C_SCL = 5;

constexpr int AIN1 = 26;
constexpr int AIN2 = 27;
constexpr int PWMA = 25;
constexpr int BIN1 = 32;
constexpr int BIN2 = 33;
constexpr int PWMB = 14;
constexpr int STBY = 13;
constexpr int ENC_A_A = 18;
constexpr int ENC_A_B = 19;
constexpr int ENC_B_A = 20;
constexpr int ENC_B_B = 21;

constexpr int PWM_FREQ = 5000;
constexpr int PWM_RES = 8;

constexpr float PASOS_X_VUELTA = 2956.0f;
constexpr float PASOS_X_GRADO = (PASOS_X_VUELTA * 1.19f) / 180.0f;

constexpr int MAX_LINES = 512;
constexpr int MAX_LINE_LEN = 64;
constexpr int MAX_VARS = 16;
constexpr int STACK_SIZE = 32;
constexpr int MAX_DEFS = 8;
constexpr unsigned long RX_TIMEOUT_MS = 300;

}  // namespace Config
