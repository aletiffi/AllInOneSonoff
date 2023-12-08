#ifndef DigiOut_h
#define DigiOut_h

#include "Arduino.h"

class DigiOut
{
  public:
    DigiOut(int pin, bool offState);
    void Begin();
    void On();
    void Off();
    void Toggle();
    bool State();
    void MemState(bool state);
    void Blink(int placingTime, int numBlink, int velBlink);
    
  private:
    int _pin;
    bool _offState;
    bool _actualState;
};

#endif
