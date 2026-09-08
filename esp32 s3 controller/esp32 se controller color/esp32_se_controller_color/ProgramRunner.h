#pragma once

namespace ProgramRunner {

void begin();
void addLine(const char* line, bool resetBuffer = false);
bool startBufferedProgram();
void stopNow();
bool isDone();
float evalExpr(const char* expr);

}  // namespace ProgramRunner
