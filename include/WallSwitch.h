#ifndef WallSwitch_h
#define WallSwitch_h

// Switch mode: wall switch on GPIO14, every change of position toggles the relay
void WallSwitch_Begin();
void WallSwitch_Loop();

#endif
