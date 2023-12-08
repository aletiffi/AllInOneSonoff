#include <StoreStrings.h>

#define PARTITION_1_SIZE   125
#define PARTITION_2_SIZE   125
#define WORDS_NUM          6

String words_pt1[] = {
  "Store",
  "Strings",
  "Example", 
  "Sketch",
  "Partition", 
  "One"
  };

String words_pt2[] = {
  "Store",
  "Strings",
  "Example", 
  "Sketch",
  "Partition", 
  "Two"
  };

void setup()
{
  StoreStrings mem(PARTITION_1_SIZE, PARTITION_2_SIZE);
  Serial.begin(115200);
  delay(500);
  print("\nDemo Started\n");
  print("EEPROM initialized with " + String(mem.getEepromSize()) + " bytes");
  print("\t- PARTITION 1 = " + String(mem.getFirstPartitionSize()) + " bytes");
  print("\t\t- Start addr = " + String(mem.getStartAddr()));
  print("\t\t- End addr = " + String(mem.getEndAddr()));
  print("\t- PARTITION 2 = " + String(mem.getSecondPartitionSize()) + " bytes");
  print("\t\t- Start addr = " + String(mem.getStartAddr2()));
  print("\t\t- End addr = " + String(mem.getEndAddr2()));

  print("\nWords in first partition:");
  for (byte i = 0; i < WORDS_NUM; i++) {
    if(mem.write(mem.getLastWrittenByte(), words_pt1[i])){
      print(mem.read(mem.getLastReadedByte()));
    } else {
      print("Writing failed!");
    }
  }
  print("\nThese words are from byte " + String(mem.getStartAddr()) + " to byte " + String(mem.getLastReadedByte()));
  print("and leaves another " + String(mem.getEndAddr() - mem.getLastWrittenByte()) + " byte free\n");

  print("\nWords in second partition:");
  for (byte i = 0; i < WORDS_NUM; i++) {
    if(mem.write_pt2(mem.getLastWrittenByte2(), words_pt2[i])){
      print(mem.read_pt2(mem.getLastReadedByte2()));
    } else {
      print("Writing failed!");
    }
  }
  print("\nThese words are from byte " + String(mem.getStartAddr2()) + " to byte " + String(mem.getLastReadedByte2()));
  print("and leaves another " + String(mem.getEndAddr2() - mem.getLastWrittenByte2()) + " byte free\n");
  
  print("\nReset of counters:");
  mem.resetReadCounter();
  mem.resetWriteCounter();
  mem.resetReadCounter2();
  mem.resetWriteCounter2();
  print("\t- Last readed byte in partition 1: " + String(mem.getLastReadedByte()));
  print("\t- Last written byte in partition 1: " + String(mem.getLastWrittenByte()));
  print("\t- Last readed byte in partition 2: " + String(mem.getLastReadedByte2()));
  print("\t- Last written byte in partition 2: " + String(mem.getLastWrittenByte2()));
  
  print("\nDemo ended, bye");
}

void loop()
{

}

void print(String val) {
  Serial.println(val);
}
