#include "Comms.h"

#include <WiFi.h>
#include <WiFiUdp.h>
#include <string.h>

#include "Config.h"

namespace {
IPAddress wemosIP(0, 0, 0, 0);
bool wemosKnown = false;
WiFiUDP udp;
}

namespace Comms {

void beginAccessPoint() {
  WiFi.mode(WIFI_AP);
  WiFi.softAP(Config::AP_SSID, Config::AP_PASS);
  udp.begin(Config::UDP_LISTEN_PORT);
}

bool isWemosKnown() {
  return wemosKnown;
}

bool receiveLine(char* buffer, size_t bufferSize) {
  if (buffer == nullptr || bufferSize == 0) return false;

  int pktSize = udp.parsePacket();
  if (pktSize <= 0) return false;

  if (!wemosKnown) {
    wemosIP = udp.remoteIP();
    wemosKnown = true;
  }

  int len = udp.read(buffer, bufferSize - 1);
  if (len <= 0) return false;

  buffer[len] = '\0';
  String line = String(buffer);
  line.trim();
  strncpy(buffer, line.c_str(), bufferSize - 1);
  buffer[bufferSize - 1] = '\0';
  return true;
}

void sendTelem(const char* type, const char* value) {
  if (!wemosKnown) return;

  char buf[128];
  snprintf(buf, sizeof(buf), "%s:%s", type, value);
  udp.beginPacket(wemosIP, Config::UDP_TELEM_PORT);
  udp.write((uint8_t*)buf, strlen(buf));
  udp.endPacket();
}

}  // namespace Comms
