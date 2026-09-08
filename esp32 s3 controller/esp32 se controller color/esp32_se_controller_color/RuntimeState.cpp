#include "RuntimeState.h"

namespace {
volatile bool motAbort = false;
}

namespace RuntimeState {

void requestAbort() {
  motAbort = true;
}

void clearAbort() {
  motAbort = false;
}

bool isAbortRequested() {
  return motAbort;
}

}  // namespace RuntimeState
