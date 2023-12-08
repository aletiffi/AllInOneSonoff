#include "Arduino.h"
#include "ReadInput.h"

ReadInput::ReadInput(int pin, int debounceTime)
{
  pinMode(pin, INPUT_PULLUP);
  _pin = pin;
  _debounceTime = debounceTime;
  _inputState = digitalRead(_pin);
}

bool ReadInput::isActive()
{
  return !_readingFault;
}


int ReadInput::getErrors()
{
  return _badReadings;
}


void ReadInput::setErrors(int num)
{
  if (num >= 0 && num <= _badReadingsMax)
  {
    _badReadings = num;
    Serial.println("Another " + String(_badReadingsMax - _badReadings) + " bad readings left");
  } else {
    Serial.println("Accepted values ​​are between 0 and " + String(_badReadingsMax));
  }
  
}


bool ReadInput::State()
{
  if (!_readingFault && _inputState != digitalRead(_pin))
  {
    _readingStartTime = millis();
    
    while (millis() - _readingStartTime < _debounceTime * 3) {
      _inputState = digitalRead(_pin);
      _readingTime = millis();

      while (millis() - _readingTime < _debounceTime) {
        if (_inputState != digitalRead(_pin)) {
          Serial.println("Spike detected on pin " + String(_pin));
          break;
        }
      }
      if(_badReadings > 0){
        _badReadings--;
      }
      return _inputState;
    }
    
    Serial.println("Unable to read Pin " + String(_pin) + "! Unstable reading");
    _badReadings++;

    if (_badReadings >= _badReadingsMax){
      _readingFault = true;
      Serial.println("Reading of Pin " + String(_pin) + " disabled");
    }

    _inputState = false;
    return _inputState;

  } else {
    return _inputState;
  }
}
