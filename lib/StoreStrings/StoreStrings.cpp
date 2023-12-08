#include "Arduino.h"
#include "StoreStrings.h"
#include "EEPROM.h"

StoreStrings::StoreStrings(int partition1)
{
  if (partition1 >= _minSize) {
    _part1 = partition1;
    _startAddr1 = 1;
    _endAddr1 = partition1 - 1;
  }
  _eepromSize = _part1;
  resetReadCounter();
  resetWriteCounter();

  if (_eepromSize > _minSize){
    EEPROM.begin(_eepromSize);
  }
}

StoreStrings::StoreStrings(int partition1, int partition2)
{
  if (partition1 >= _minSize) {
    _part1 = partition1;
    _startAddr1 = 1;
    _endAddr1 = partition1 - 1;
  }

  if (partition2 >= _minSize) {
    _part2 = partition2;
    _startAddr2 = partition1;
    _endAddr2 = partition1 + partition2 - 1;
  }
  
  _eepromSize = _part1 + _part2;
  resetReadCounter();
  resetReadCounter2();
  resetWriteCounter();
  resetWriteCounter2();

  if (_eepromSize > _minSize * 2){
    EEPROM.begin(_eepromSize);
  }

}

void StoreStrings::print_all()
{
  int k = 0;
  for (int i = 0; i < _eepromSize; i++){
    if (k < 10){
      k++;
      Serial.print((char)EEPROM.read(i));
      Serial.print("\t");
    } else {
      k = 0;
      Serial.println((char)EEPROM.read(i));
    }
  }
  Serial.println("");
}

String StoreStrings::read(int read_addr)
{

  if (read_addr >= _startAddr1 && read_addr < _endAddr1){
    _ReadAddress1 = read_addr;
    String value = "";

    while (_ReadAddress1 < _endAddr1 && char(EEPROM.read(_ReadAddress1)) != _null) {
      value += char(EEPROM.read(_ReadAddress1));
      _ReadAddress1++;
    }
    _ReadAddress1++;
    _ReadAddressPre1 = _ReadAddress1;
    return value;

  } else {
    Serial.println("Read address " + String(read_addr) + " must be >= " + String(_startAddr1) + " and <" + String(_endAddr1));
    _ReadAddress1 = _ReadAddressPre1;
    return String(_null);
  }
}

String StoreStrings::read_pt2(int read_addr)
{

  if (read_addr >= _startAddr2 && read_addr < _endAddr2){
    _ReadAddress2 = read_addr;
    String value = "";

    while (_ReadAddress2 < _endAddr2 && char(EEPROM.read(_ReadAddress2)) != _null) {
      value += char(EEPROM.read(_ReadAddress2));
      _ReadAddress2++;
    }
    _ReadAddress2++;
    _ReadAddressPre2 = _ReadAddress2;
    return value;

  } else {
    Serial.println("Read address " + String(read_addr) + " must be >= " + String(_startAddr2) + " and <" + String(_endAddr2));
    _ReadAddress2 = _ReadAddressPre2;
    return String(_null);
  }
}

bool StoreStrings::write(int write_addr, String val)
{
  if (write_addr >= _startAddr1 && write_addr < _endAddr1){
    _WriteAddress1 = write_addr;
    val += _null;

    for (unsigned int i=0; i<val.length(); i++){
      if(_WriteAddress1 < _endAddr1){
        if(char(EEPROM.read(_WriteAddress1)) != val.charAt(i)){
          EEPROM.write(_WriteAddress1, val.charAt(i));
          EEPROM.commit();
        }
        _WriteAddress1 ++;
      } else {
        Serial.println("EEPROM Full");
        _WriteAddress1 = _WriteAddressPre1;
        return false;
      }
    }
    _WriteAddressPre1 = _WriteAddress1;
    return true;

  } else {
    Serial.println("Write address " + String(write_addr) + " must be >= " + String(_startAddr1) + " and <" + String(_endAddr1));
    _WriteAddress1 = _WriteAddressPre1;
    return false;
  }
}

bool StoreStrings::write_pt2(int write_addr, String val)
{
  if (write_addr >= _startAddr2 && write_addr < _endAddr2){
    _WriteAddress2 = write_addr;
    val += _null;

    for (unsigned int i=0; i<val.length(); i++){
      if(_WriteAddress2 < _endAddr2){
        if(char(EEPROM.read(_WriteAddress2)) != val.charAt(i)){
          EEPROM.write(_WriteAddress2, val.charAt(i));
          EEPROM.commit();
        }
        _WriteAddress2 ++;
      } else {
        Serial.println("EEPROM Full");
        _WriteAddress2 = _WriteAddressPre2;
        return false;
      }
    }
    _WriteAddressPre2 = _WriteAddress2;
    return true;

  } else {
    Serial.println("Write address " + String(write_addr) + " must be >= " + String(_startAddr2) + " and <" + String(_endAddr2));
    _WriteAddress2 = _WriteAddressPre2;
    return false;
  }
}

bool StoreStrings::resetReadCounter()
{
  if (_endAddr1 > 0) {
    _ReadAddress1 = _startAddr1;
    _ReadAddressPre1 = _startAddr1;
    return true;
  } else {
    return false;
  }
}

bool StoreStrings::resetReadCounter2()
{  
  if (_endAddr2 > 0) {
    _ReadAddress2 = _startAddr2;
    _ReadAddressPre2 = _startAddr2;
    return true;
  } else {
    return false;
  }
}

bool StoreStrings::resetWriteCounter()
{
  if (_endAddr1 > 0) {
    _WriteAddress1 = _startAddr1;
    _WriteAddressPre1 = _startAddr1;
    return true;
  } else {
    return false;
  }
}

bool StoreStrings::resetWriteCounter2()
{
  if (_endAddr2 > 0) {
    _WriteAddress2 = _startAddr2;
    _WriteAddressPre2 = _startAddr2;
    return true;
  } else {
    return false;
  }
}

int StoreStrings::getEepromSize()
{
  return _eepromSize;
}

int StoreStrings::getFirstPartitionSize()
{
  return _part1;
}

int StoreStrings::getSecondPartitionSize()
{
  return _part2;
}

int StoreStrings::getLastReadedByte()
{
  return _ReadAddress1;
}

int StoreStrings::getLastReadedByte2()
{
  return _ReadAddress2;
}

int StoreStrings::getLastWrittenByte()
{
  return _WriteAddress1;
}

int StoreStrings::getLastWrittenByte2()
{
  return _WriteAddress2;
}

int StoreStrings::getStartAddr()
{
  return _startAddr1;
}

int StoreStrings::getStartAddr2()
{
  return _startAddr2;
}

int StoreStrings::getEndAddr()
{
  return _endAddr1;
}

int StoreStrings::getEndAddr2()
{
  return _endAddr2;
}

bool StoreStrings::clear()
{
  if (_eepromSize > 0){
    for(int i = 0; i < _eepromSize; i++){
      EEPROM.write(i, _null);
    }
    if (EEPROM.commit()){
      return true;
    }
  }
  return false;
}

bool StoreStrings::isReady(){
  if (EEPROM.read(0) == _null){
    return true;
  } else {
    return false;
  }
}