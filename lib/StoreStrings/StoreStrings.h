#ifndef StoreStrings_h
#define StoreStrings_h

#include "Arduino.h"
#include "EEPROM.h"

class StoreStrings
{
  public:
    StoreStrings(int size);
    StoreStrings(int partition1, int partition2);
    String read(int start_addr);
    String read_pt2(int start_addr);
    void print_all();
    bool write(int start_addr, String val);
    bool write_pt2(int start_addr, String val);
    int getEepromSize();
    int getFirstPartitionSize();
    int getSecondPartitionSize();
    bool resetReadCounter();
    bool resetReadCounter2();
    bool resetWriteCounter();
    bool resetWriteCounter2();
    int getLastReadedByte();
    int getLastReadedByte2();
    int getLastWrittenByte();
    int getLastWrittenByte2();
    int getStartAddr();
    int getStartAddr2();
    int getEndAddr();
    int getEndAddr2();
    bool clear();
    bool isReady();
    
  private:
    int _ReadAddress1 = 0;
    int _ReadAddress2 = 0;
    int _ReadAddressPre1 = 0;
    int _ReadAddressPre2 = 0;
    int _WriteAddress1 = 0;
    int _WriteAddress2 = 0;
    int _WriteAddressPre1 = 0;
    int _WriteAddressPre2 = 0;
    int _eepromSize = 0;
    char _null = '\0';
    int _part1 = 0;
    int _part2 = 0;
    int _startAddr1 = 0;
    int _endAddr1 = 0;
    int _startAddr2 = 0;
    int _endAddr2 = 0;
    byte _minSize = 2;
};

#endif
