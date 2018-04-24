#include <SoftwareSerial.h>

SoftwareSerial SerialAT(2,3);

void setup() {
  Serial.begin(9600);
  SerialAT.begin(9600);
}

void loop() {
  if (Serial.available()) {      // If anything comes in Serial (USB),
    SerialAT.write(Serial.read());   // read it and send it out SerialAT (pins 0 & 1)
  }

  if (SerialAT.available()) {     // If anything comes in SerialAT (pins 0 & 1)
    Serial.write(SerialAT.read());   // read it and send it out Serial (USB)
  }
}
