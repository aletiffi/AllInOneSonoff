#include "Arduino.h"
#include "DigiOut.h"

DigiOut::DigiOut(int pin, bool offState)
{
  _pin = pin;
  _offState = offState;
}

void DigiOut::Begin()
{
  pinMode(_pin, OUTPUT);
  digitalWrite(_pin, _offState);
  _actualState = _offState;
}

void DigiOut::On()
{
  digitalWrite(_pin, !_offState);
  _actualState = !_offState;
}

void DigiOut::Off()
{
  digitalWrite(_pin, _offState);
  _actualState = _offState;
}

void DigiOut::Toggle()
{
  digitalWrite(_pin, !_actualState);
  _actualState = !_actualState;
}

bool DigiOut::State()
{
  return _actualState;
}

void DigiOut::Blink(int placingTime, int numBlink, int velBlink)
{
  bool tmpState = State();
  if (tmpState != _offState){
    Off();
    delay(placingTime);
  }
  
  for(int i=0; i<numBlink; i++){
    On();
    delay(velBlink);
    Off();
    delay(velBlink);
  }

  if (tmpState != _offState){
    delay(placingTime);
    On();
  }
}