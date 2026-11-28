#ifndef WallSwitch_h
#define WallSwitch_h

// Switch mode: wall switch on the free pin (GPIO14 on the R1, GPIO2 on the R2), every change of position toggles the relay
void WallSwitch_Begin();
void WallSwitch_Loop();

#endif
