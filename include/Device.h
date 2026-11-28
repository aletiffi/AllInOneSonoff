#ifndef Device_h
#define Device_h

#include <DigiOut.h>

// Relay and status led, shared by both modes
extern DigiOut Led;
extern DigiOut Rele;

void Device_Begin();
// Free pin of the model chosen in the settings: wall switch input or ledstrip data
uint8_t Device_IoPin();
bool IsOn();
void Switch_On();
void Switch_Off();
void Switch_Toggle();
void Restart();
void CleanEEPROM();

#endif
