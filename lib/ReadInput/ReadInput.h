#ifndef ReadInput_h
#define ReadInput_h

#include "Arduino.h"

class ReadInput
{
  public:
    ReadInput(int pin, int debounceTime = 20);
    bool State();
    bool isActive();
    int getErrors();
    void setErrors(int num);
    
  private:
    int _pin                        = 0;
    bool _inputState                = false;
    unsigned long _readingStartTime = 0;
    unsigned long _debounceTime     = 0;
    unsigned long _readingTime      = 0;
    int _badReadings                = 0;
    int _badReadingsMax             = 10;
    bool _readingFault              = false;
};

#endif
