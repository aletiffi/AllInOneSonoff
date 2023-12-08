#include <StoreStrings.h>

#define EEPROM_SIZE   256


String words[] = {
  "Store",
  "Strings",
  "Example", 
  "Sketch"
  };

void setup()
{
  StoreStrings mem(EEPROM_SIZE);
  Serial.begin(115200);
  delay(500);
  print("\nDemo Started\n");
  print("EEPROM initialized with " + String(mem.getEepromSize()) + " bytes");
  print("\t\t- Start addr = " + String(mem.getStartAddr()));
  print("\t\t- End addr = " + String(mem.getEndAddr()));

  print("\nWords stored:");
  for (int i = 0; i < 4; i++) {
    if(mem.write(mem.getLastWrittenByte(), words[i])){
      print(mem.read(mem.getLastReadedByte()));
    } else {
      print("Writing failed!");
    }
  }
  print("\nThese words are from byte " + String(mem.getStartAddr()) + " to byte " + String(mem.getLastReadedByte()));
  print("and leaves another " + String(mem.getEndAddr() - mem.getLastWrittenByte()) + " byte free\n");
    
  print("\nReset of counters:");
  mem.resetReadCounter();
  mem.resetWriteCounter();
  print("\t- Last readed byte: " + String(mem.getLastReadedByte()));
  print("\t- Last written byte: " + String(mem.getLastWrittenByte()));
  
  print("\nDemo ended, bye");
}

void loop()
{

}

void print(String val) {
  Serial.println(val);
}
