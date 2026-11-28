#include <Arduino.h>
#include <ReadInput.h>
#include "WallSwitch.h"
#include "Config.h"
#include "Device.h"

static ReadInput* wallSwitch = nullptr;
static bool switchStatePre  = false;

void WallSwitch_Begin() {
  // Created here so that the free pin is configured only in Switch mode
  wallSwitch = new ReadInput(Device_IoPin());
  switchStatePre = wallSwitch->State();
}

void WallSwitch_Loop() {
  if (wallSwitch == nullptr) {
    return;
  }
  bool switchState = wallSwitch->State();
  if (switchState != switchStatePre) {
    Switch_Toggle();
  }
  switchStatePre = switchState;
}
