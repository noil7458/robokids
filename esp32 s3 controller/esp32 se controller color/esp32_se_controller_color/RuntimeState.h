#pragma once

namespace RuntimeState {

void requestAbort();
void clearAbort();
bool isAbortRequested();

}  // namespace RuntimeState
