#include <SoftwareSerial.h>

SoftwareSerial SerialAT(2,3);

#define CURRENT_BAUDRATE 38400
#define NEW_BAUDRATE 38400
#define GPRSPIN 0

void setup() {
  setPinMode(GPRSPIN, OUTPUT);
  GPRSOn();
  lowerBaud();
}

void loop() {
  
}

void GPRSOn() {
  digitalWrite(GPRSPIN, LOW);
  delay(300);
  digitalWrite(GPRSPIN, HIGH);
  SerialAT.begin(CURRENT_BAUDRATE);
  delay(2000);
}

void lowerBaud() {
  SerialAT.print("AT+IPR=");
  SerialAT.println(NEW_BAUDRATE);
}

