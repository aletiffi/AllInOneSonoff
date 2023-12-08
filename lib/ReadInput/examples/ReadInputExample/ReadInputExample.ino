#include <ReadInput.h>

#define IN_1            13

ReadInput in1(IN_1);

void setup()
{
  Serial.begin(115200);
  delay(1000);
  print("\n-------------------\nDemo started\n");
  readState();
  checkState();
}

void loop()
{
  for (byte i = 0; i<=10; i++) {
    readState();
    in1.setErrors(i + 1);
    checkState();
    delay(1000);
  }

  print("\nSpikes detection and disabled reading mode is up to you ;-P");
  print("Demo ended, bye!");
  while (true) {
    delay(1000);
  }

}

void print(String val) {
  Serial.println(val);
}

void checkState() {
  if (in1.isActive()) {
    print("Input reading is active and there are " + String(in1.getErrors()) + " error/s detected");
  } else {
    print("Input reading has been deactivated! " + String(in1.getErrors()) + " errors detected!");
  }
}

void readState() {
  print("Input state: " + String(in1.State()));
}
