#include "CommandHelpers.h"

#include "RuntimeState.h"

namespace CommandHelpers {

String getP(const char* s, int idx) {
  int f = 0;
  int st = 0;
  int depth = 0;
  for (int i = 0;; i++) {
    if (s[i] == '(' || s[i] == '[') depth++;
    else if (s[i] == ')' || s[i] == ']') depth--;
    else if ((s[i] == ':' && depth == 0) || s[i] == '\0') {
      if (f == idx) return String(s).substring(st, i);
      f++;
      st = i + 1;
    }
    if (s[i] == '\0') break;
  }
  return "";
}

void doWait(float secs) {
  unsigned long t0 = millis();
  while (!RuntimeState::isAbortRequested() && (millis() - t0) < (unsigned long)(secs * 1000.0f)) {
    vTaskDelay(pdMS_TO_TICKS(5));
  }
}

}  // namespace CommandHelpers
