#include <SoftwareSerial.h>
/*
 * Uncomment this line to receive debug logging on the serial monitor
 */
//#define DEVMODE 1

/*
 * Uncomment the board you're using
 */
#define ATTINY85 1 // ATTiny85
//#define UNO 1      // Arduino UNO

#define MEASURE_INTERVAL 10
#define HEARTBEAT_INTERVAL 24
#define GPRS_BAUDRATE 38400

#if defined(UNO)
  #define GPRSPIN 4
  #define RX 2
  #define TX 3
  #define VREF 4.91
#else if defined(ATTINY85)
  #define GPRSPIN 0
  #define RX 3
  #define TX 4
  #define VREF 4.943
#endif
#define VINPIN A1

/*
 * Max value for adc converter as double
 * The arduino adc converter has a max range of 8-bits resulting in a max value of 1023 
 */
#define MAXVAL 1023.0

/*
 * VREF -> Reference voltage
 * might need to set to 3.7 for lipo batteries
 */

SoftwareSerial SerialAT(RX,TX);

/* 
 * Amount by which Vin is divided in the voltage divider 
 * divider = actualVin / actualVout
 */
#define DIVIDER 5.97
#define QUOTES char(34)

void setup()
{
  #if defined(DEVMODE)
    Serial.begin(38400);
  #endif
  
  pinMode(GPRSPIN, OUTPUT);
}

void loop()
{
  #if defined(ATTINY85)
    delayWithRelay(3000);
  #endif
  float Vin = measureVoltage();
  sendData(Vin);                                                                             // let server know we are alive at the start of the 24 hour cycle
  
  int cycle_counter = 0;
  while (cycle_counter < (60/MEASURE_INTERVAL) * HEARTBEAT_INTERVAL) {                       // run for 24 hours
    #if defined(DEVMODE)
      Serial.println("start measuring Vin");
    #endif
    Vin = measureVoltage();
    #if defined(DEVMODE)
      Serial.println("wait ten minutes");
    #endif

    for (int i = 0; i < MEASURE_INTERVAL * 60; i++) {
      delay(1000);
    }
    
    #if defined(DEVMODE)
      Serial.println("measure again");
    #endif
    float Vnew = measureVoltage();
    if (Vin - Vnew < -0.1 || Vin - Vnew > 0.1) {                  // does new measurement differ from old measurement by at least 0.1v?
      #if defined(DEVMODE)
        Serial.println("difference too big");
      #endif
      sendData(Vnew);                                             // then update battery voltage
    } else {
      #if defined(DEVMODE)
        Serial.println("difference negligible");
      #endif
    }
    cycle_counter++;
  }
}

double measureVoltage() {
  unsigned int sample_count = 0;
  unsigned int sum = 0;
  
  while (sample_count < 25) {                                     // Take a number of analog samples and add them up
      sum += analogRead(VINPIN);
      sample_count++;
      delayWithRelay(10);
  }

  return ((((float)sum / (float)25 * VREF) / MAXVAL) * DIVIDER) + 0.7;
}

void sendData(float v) {
  char battery_id[] = "A122333";
  char vin[5];
  sprintf(vin, "%f", v);

  unsigned int payloadSize = 80;
  payloadSize += strlen(battery_id);
  payloadSize += strlen(vin);
  
  GPRSOn();                                                       // Turn on GPRS module
  
  SerialAT.println("AT+TCPSETUP=0,85.222.229.221,80");              // Connect to server
  delay(3000);
  SerialAT.print("AT+TCPSEND=0,");                                // Perform HTTP GET
  SerialAT.println(payloadSize);
  delay(2000);
  SerialAT.print("GET /api/battery/car-update/");
  delayWithRelay(10);
  SerialAT.print(v);
  delayWithRelay(10);
  SerialAT.print("/");
  delayWithRelay(10);
  SerialAT.print(battery_id);
  delayWithRelay(10);
  SerialAT.print(" HTTP/1.1\n");
  delayWithRelay(10);
  SerialAT.print("Host: a.phi.guru\n");
  delayWithRelay(10);
  SerialAT.print("Connection: close\n\r\n");
  delayWithRelay(500);
  SerialAT.print((char)0x0D);
  delayWithRelay(5000);                                           // Wait until done receiving data
  SerialAT.println("AT+TCPCLOSE=0");                              // Close connection
  delayWithRelay(500);
  
  GPRSOff();                                                      // Turn off GPRS module
}

/*
 * Replacement for the default delay() method
 * Functions just the same, with the added bonus of passing through serial communication
 */
void delayWithRelay(uint32_t del) {
  #if defined(DEVMODE)
    uint32_t timestamp = millis();
    while (millis() - timestamp < del) {
      if (SerialAT.available()) {
        Serial.write(SerialAT.read());
      }
      if (Serial.available()) {
        SerialAT.write(Serial.read());
      }
    }
  #else
    delay(del);
  #endif
}

/*
 * Switch GPRS module on
 */
void GPRSOn() {
  #if defined(DEVMODE)
    Serial.println("TURNING ON GPRS");
  #endif
  digitalWrite(GPRSPIN, LOW);
  delay(300);
  digitalWrite(GPRSPIN, HIGH);
  SerialAT.begin(GPRS_BAUDRATE);
  for(int i = 0; i < 30; i++){
    delay(1000);
  }
  setupGPRS();
}

/*
 * Switch GPRS module off
 */
void GPRSOff() {
  #if defined(DEVMODE)
    Serial.println("TURNING OFF GPRS");
  #endif
  SerialAT.println("AT+CPWROFF");
  #if defined(ATTINY85)
    delayWithRelay(5000);
  #endif
  digitalWrite(GPRSPIN, LOW);
  delayWithRelay(5000);
  SerialAT.end();
}

/*
 * Prepare the GPRS module for HTTP requests
 */
void setupGPRS() {
  #if defined(DEVMODE)
    Serial.println("SETTING UP GPRS");
  #endif
  SerialAT.println("AT+XISP=0");
  delayWithRelay(1000);
 
  // Set apn
  SerialAT.print("AT+CGDCONT=1,");SerialAT.print(QUOTES);SerialAT.print("IP");SerialAT.print(QUOTES);SerialAT.print(",");SerialAT.print(QUOTES);SerialAT.print("internet");SerialAT.println(QUOTES);
  delayWithRelay(1000);
  
  // Authenticate
  SerialAT.print("AT+XGAUTH=1,1,");SerialAT.print(QUOTES);SerialAT.print("tmobile");SerialAT.print(QUOTES);SerialAT.print(",");SerialAT.print(QUOTES);SerialAT.print("tmobile");SerialAT.println(QUOTES);
  delayWithRelay(1000);
  
  SerialAT.println("AT+XIIC=1");                                  // Establish PPP link (check state with 'AT+XIIC?')
  delayWithRelay(1000);
}


