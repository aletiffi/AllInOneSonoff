#ifndef Device_h
#define Device_h

#include <DigiOut.h>

// Relay and status led, shared by both modes
extern DigiOut Led;
extern DigiOut Rele;

void Device_Begin();
bool IsOn();
void Switch_On();
void Switch_Off();
void Switch_Toggle();
void Restart();
void CleanEEPROM();

#endif
