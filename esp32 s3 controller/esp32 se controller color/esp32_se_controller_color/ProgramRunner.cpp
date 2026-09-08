#include "ProgramRunner.h"

#include <Arduino.h>
#include <esp_system.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

#include "ColorSensor.h"
#include "CommandHelpers.h"
#include "Comms.h"
#include "Config.h"
#include "DistanceSensor.h"
#include "Motors.h"
#include "RuntimeState.h"

namespace {
char prog[Config::MAX_LINES][Config::MAX_LINE_LEN];
int progLen = 0;
volatile bool progDone = true;

struct Var {
  char name[16];
  float val;
};
Var vars[Config::MAX_VARS];
int varCount = 0;

struct StackFrame {
  char type;
  int loopStart;
  int loopCount;
};
StackFrame stk[Config::STACK_SIZE];
int stkTop = 0;

struct Def {
  char name[32];
  int start;
  int end;
};
Def defs[Config::MAX_DEFS];
int defCount = 0;

TaskHandle_t runTask = NULL;

float getVar(const char* name) {
  for (int i = 0; i < varCount; i++) {
    if (strcmp(vars[i].name, name) == 0) return vars[i].val;
  }
  return 0;
}

void setVar(const char* name, float val) {
  for (int i = 0; i < varCount; i++) {
    if (strcmp(vars[i].name, name) == 0) {
      vars[i].val = val;
      return;
    }
  }
  if (varCount < Config::MAX_VARS) {
    strncpy(vars[varCount].name, name, 15);
    vars[varCount].name[15] = '\0';
    vars[varCount].val = val;
    varCount++;
  }
}

int findEnd(int from, const char* endToken, const char* startToken) {
  int depth = 1;
  for (int i = from + 1; i < progLen; i++) {
    if (strncmp(prog[i], startToken, strlen(startToken)) == 0) depth++;
    else if (strcmp(prog[i], endToken) == 0) {
      depth--;
      if (depth == 0) return i;
    }
  }
  return progLen;
}

void prescanDefs() {
  defCount = 0;
  for (int i = 0; i < progLen; i++) {
    if (strncmp(prog[i], "DEF:", 4) == 0) {
      if (defCount >= Config::MAX_DEFS) continue;
      strncpy(defs[defCount].name, prog[i] + 4, 31);
      defs[defCount].name[31] = '\0';
      defs[defCount].start = i;
      defs[defCount].end = findEnd(i, "END_DEF", "DEF:");
      defCount++;
    }
  }
}

void execLine(int& pc);

void runProgram() {
  stkTop = 0;
  prescanDefs();
  Motors::resetEncoders();
  Comms::sendTelem("STATUS", "RUNNING");

  int pc = 0;
  while (pc < progLen && !RuntimeState::isAbortRequested()) {
    execLine(pc);
  }

  if (!RuntimeState::isAbortRequested()) Comms::sendTelem("STATUS", "DONE");
  else Comms::sendTelem("STATUS", "IDLE");
  progDone = true;
}

void execLine(int& pc) {
  const char* line = prog[pc];
  if (line[0] == '\0') {
    pc++;
    return;
  }

  if (stkTop > 0) {
    char top = stk[stkTop - 1].type;
    if (top == 'I' || top == 'D') {
      if (top == 'I' && strcmp(line, "ELSE") == 0) {
        stk[stkTop - 1].type = 'X';
        pc++;
        return;
      }
      if (strcmp(line, "END_IF") == 0 || strcmp(line, "END_REPEAT") == 0 ||
          strcmp(line, "END_FOREVER") == 0 || (top == 'D' && strcmp(line, "END_DEF") == 0)) {
        stkTop--;
        pc++;
        return;
      }
      if (strncmp(line, "IF:", 3) == 0) {
        pc = findEnd(pc, "END_IF", "IF:") + 1;
        return;
      }
      if (strncmp(line, "REPEAT:", 7) == 0) {
        pc = findEnd(pc, "END_REPEAT", "REPEAT:") + 1;
        return;
      }
      if (strcmp(line, "FOREVER") == 0) {
        pc = findEnd(pc, "END_FOREVER", "FOREVER") + 1;
        return;
      }
      if (strncmp(line, "DEF:", 4) == 0) {
        pc = findEnd(pc, "END_DEF", "DEF:") + 1;
        return;
      }
      pc++;
      return;
    }
    if (top == 'X') {
      if (strcmp(line, "END_IF") == 0) {
        stkTop--;
        pc++;
        return;
      }
      if (strncmp(line, "IF:", 3) == 0) {
        pc = findEnd(pc, "END_IF", "IF:") + 1;
        return;
      }
    }
  }

  if (strncmp(line, "REPEAT:", 7) == 0) {
    int times = (int)ProgramRunner::evalExpr(line + 7);
    if (times <= 0) {
      pc = findEnd(pc, "END_REPEAT", "REPEAT:") + 1;
      return;
    }
    if (stkTop < Config::STACK_SIZE) {
      stk[stkTop++] = {'R', pc, times - 1};
    }
    pc++;
    return;
  }

  if (strcmp(line, "END_REPEAT") == 0) {
    if (stkTop > 0 && stk[stkTop - 1].type == 'R') {
      if (stk[stkTop - 1].loopCount > 0) {
        stk[stkTop - 1].loopCount--;
        pc = stk[stkTop - 1].loopStart + 1;
        return;
      } else {
        stkTop--;
      }
    }
    pc++;
    return;
  }

  if (strcmp(line, "FOREVER") == 0) {
    if (stkTop < Config::STACK_SIZE) {
      stk[stkTop++] = {'F', pc, 0};
    }
    pc++;
    return;
  }

  if (strcmp(line, "END_FOREVER") == 0) {
    if (stkTop > 0 && stk[stkTop - 1].type == 'F') {
      pc = stk[stkTop - 1].loopStart + 1;
      return;
    }
    pc++;
    return;
  }

  if (strncmp(line, "IF:", 3) == 0) {
    float cond = ProgramRunner::evalExpr(line + 3);
    if (cond != 0) {
      if (stkTop < Config::STACK_SIZE) {
        stk[stkTop++] = {'T', pc, 0};
      }
    } else {
      if (stkTop < Config::STACK_SIZE) {
        stk[stkTop++] = {'I', pc, 0};
      }
    }
    pc++;
    return;
  }

  if (strcmp(line, "ELSE") == 0) {
    if (stkTop > 0 && stk[stkTop - 1].type == 'T') {
      stk[stkTop - 1].type = 'X';
    }
    pc++;
    return;
  }

  if (strcmp(line, "END_IF") == 0) {
    if (stkTop > 0) {
      char t = stk[stkTop - 1].type;
      if (t == 'T' || t == 'I' || t == 'X') stkTop--;
    }
    pc++;
    return;
  }

  if (strncmp(line, "DEF:", 4) == 0) {
    pc = findEnd(pc, "END_DEF", "DEF:") + 1;
    return;
  }
  if (strcmp(line, "END_DEF") == 0) {
    pc++;
    return;
  }

  if (strncmp(line, "CALL:", 5) == 0) {
    const char* name = line + 5;
    for (int i = 0; i < defCount; i++) {
      if (strcmp(defs[i].name, name) == 0) {
        int sub = defs[i].start + 1;
        while (sub < defs[i].end && !RuntimeState::isAbortRequested()) execLine(sub);
        break;
      }
    }
    pc++;
    return;
  }

  if (strncmp(line, "SET:", 4) == 0) {
    String l = String(line);
    int c1 = l.indexOf(':', 4);
    if (c1 > 0) {
      String vname = l.substring(4, c1);
      String expr = l.substring(c1 + 1);
      setVar(vname.c_str(), (float)ProgramRunner::evalExpr(expr.c_str()));
    }
    pc++;
    return;
  }

  Comms::sendTelem("CMD", line);
  String t = CommandHelpers::getP(line, 0);
  t.toUpperCase();

  if (t == "PING") {
    Comms::sendTelem("STATUS", "IDLE");
  } else if (t == "STOP") {
    Motors::motorOff();
  } else if (t == "FORWARD") {
    float r = ProgramRunner::evalExpr(CommandHelpers::getP(line, 1).c_str());
    int sp = (int)ProgramRunner::evalExpr(CommandHelpers::getP(line, 2).c_str());
    Motors::moveSteps((long)(r * Config::PASOS_X_VUELTA), true, sp);
  } else if (t == "BACKWARD") {
    float r = ProgramRunner::evalExpr(CommandHelpers::getP(line, 1).c_str());
    int sp = (int)ProgramRunner::evalExpr(CommandHelpers::getP(line, 2).c_str());
    Motors::moveSteps((long)(r * Config::PASOS_X_VUELTA), false, sp);
  } else if (t == "TURN_RIGHT") {
    float g = ProgramRunner::evalExpr(CommandHelpers::getP(line, 1).c_str());
    int sp = (int)ProgramRunner::evalExpr(CommandHelpers::getP(line, 2).c_str());
    Motors::turnSteps((long)(g * Config::PASOS_X_GRADO), true, sp);
  } else if (t == "TURN_LEFT") {
    float g = ProgramRunner::evalExpr(CommandHelpers::getP(line, 1).c_str());
    int sp = (int)ProgramRunner::evalExpr(CommandHelpers::getP(line, 2).c_str());
    Motors::turnSteps((long)(g * Config::PASOS_X_GRADO), false, sp);
  } else if (t == "MOTOR_A") {
    float r = ProgramRunner::evalExpr(CommandHelpers::getP(line, 1).c_str());
    bool fwd = (CommandHelpers::getP(line, 2) != "0");
    int sp = (int)ProgramRunner::evalExpr(CommandHelpers::getP(line, 3).c_str());
    Motors::moveSteps((long)(r * Config::PASOS_X_VUELTA), fwd, sp);
  } else if (t == "MOTOR_B") {
    float r = ProgramRunner::evalExpr(CommandHelpers::getP(line, 1).c_str());
    bool fwd = (CommandHelpers::getP(line, 2) != "0");
    int sp = (int)ProgramRunner::evalExpr(CommandHelpers::getP(line, 3).c_str());
    Motors::moveSteps((long)(r * Config::PASOS_X_VUELTA), fwd, sp);
  } else if (t == "TANK") {
    float vA = ProgramRunner::evalExpr(CommandHelpers::getP(line, 1).c_str());
    float vB = ProgramRunner::evalExpr(CommandHelpers::getP(line, 2).c_str());
    float s = ProgramRunner::evalExpr(CommandHelpers::getP(line, 3).c_str());
    Motors::tankMove(vA, vB, s);
  } else if (t == "WAIT") {
    float s = ProgramRunner::evalExpr(CommandHelpers::getP(line, 1).c_str());
    CommandHelpers::doWait(s);
  } else if (t == "RESET_ENC") {
    Motors::resetEncoders();
    Comms::sendTelem("STATUS", "ENC_RESET");
  } else {
    Comms::sendTelem("WARN", line);
  }

  pc++;
}

void runTaskFn(void*) {
  for (;;) {
    ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
    runProgram();
  }
}
}

namespace ProgramRunner {

void begin() {
  xTaskCreatePinnedToCore(runTaskFn, "RunTask", 4096, NULL, 2, &runTask, 1);
}

void addLine(const char* line, bool resetBuffer) {
  if (resetBuffer) progLen = 0;
  if (progLen >= Config::MAX_LINES) return;

  strncpy(prog[progLen], line, Config::MAX_LINE_LEN - 1);
  prog[progLen][Config::MAX_LINE_LEN - 1] = '\0';
  progLen++;
}

bool startBufferedProgram() {
  if (progLen <= 0 || runTask == NULL) return false;

  RuntimeState::clearAbort();
  progDone = false;
  xTaskNotifyGive(runTask);
  return true;
}

void stopNow() {
  RuntimeState::requestAbort();
  Motors::motorOff();
  progLen = 0;
  progDone = true;
  Comms::sendTelem("STATUS", "IDLE");
}

bool isDone() {
  return progDone;
}

float evalExpr(const char* expr) {
  char e[48];
  strncpy(e, expr, 47);
  e[47] = '\0';
  int s = 0;
  while (e[s] == ' ') s++;
  int len = strlen(e + s);
  while (len > 0 && e[s + len - 1] == ' ') len--;
  e[s + len] = '\0';
  const char* ex = e + s;

  char* end;
  float v = strtof(ex, &end);
  if (end != ex && *end == '\0') return v;

  if (strncmp(ex, "random(", 7) == 0) {
    char inner[40];
    strncpy(inner, ex + 7, 39);
    inner[39] = '\0';
    char* comma = strchr(inner, ',');
    if (!comma) return 0;
    *comma = '\0';
    float a = evalExpr(inner);
    float b = evalExpr(comma + 1);
    return a + (float)(esp_random() % (int)(fabsf(b - a) + 1));
  }

  if (ex[0] == '(') {
    char inner[40];
    strncpy(inner, ex + 1, 39);
    inner[39] = '\0';
    int l = strlen(inner);
    if (l > 0 && inner[l - 1] == ')') inner[l - 1] = '\0';
    int depth = 0;
    for (int i = strlen(inner) - 1; i >= 0; i--) {
      if (inner[i] == ')') depth++;
      else if (inner[i] == '(') depth--;
      else if (depth == 0 &&
               (inner[i] == '+' || inner[i] == '-' || inner[i] == '*' || inner[i] == '/' ||
                inner[i] == '=' || inner[i] == '<' || inner[i] == '>')) {
        char op = inner[i];
        inner[i] = '\0';
        float a = evalExpr(inner);
        float b = evalExpr(inner + i + 1);
        if (op == '+') return a + b;
        if (op == '-') return a - b;
        if (op == '*') return a * b;
        if (op == '/') return b != 0 ? a / b : 0;
        if (op == '=') return (fabsf(a - b) < 0.001f) ? 1 : 0;
        if (op == '<') return a < b ? 1 : 0;
        if (op == '>') return a > b ? 1 : 0;
      }
    }
  }

  if (strncmp(ex, "COLOR_IS_", 9) == 0) {
    char wanted[16];
    strncpy(wanted, ex + 9, 15);
    wanted[15] = '\0';
    char* paren = strchr(wanted, '(');
    if (paren) *paren = '\0';
    return ColorSensor::isColor(wanted) ? 1.0f : 0.0f;
  }

  if (strcmp(ex, "COLOR_RED()") == 0) return (float)ColorSensor::red();
  if (strcmp(ex, "COLOR_GREEN()") == 0) return (float)ColorSensor::green();
  if (strcmp(ex, "COLOR_BLUE()") == 0) return (float)ColorSensor::blue();
  if (strcmp(ex, "DISTANCE()") == 0) return (float)DistanceSensor::getDistanceMM();

  return getVar(ex);
}

}  // namespace ProgramRunner
