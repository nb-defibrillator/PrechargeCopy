#include <CANSAME5x.h>

CANSAME5x CAN;  

const long bitrate = 250e3; // 250 kbps
unsigned long timestamp;


void setup() {
  pinMode(PIN_CAN_STANDBY, OUTPUT);
  digitalWrite(PIN_CAN_STANDBY, false); // turn off STANDBY
  pinMode(PIN_CAN_BOOSTEN, OUTPUT);
  digitalWrite(PIN_CAN_BOOSTEN, true); // turn on booster

  while(!Serial);
  Serial.begin(115200);
   

  if (!CAN.begin(250000)) {
    Serial.println("Starting CAN failed!");
    while (1) delay(10);
  }
  Serial.println("Starting CAN!");

  timestamp == millis();
}

void loop() {
  if ((millis() - timestamp) > 500) {
    Serial.println("Sending packed with id 0x25 and length 2");
    CAN.beginPacket(0x25);
    CAN.write('H');
    CAN.write('W');
    CAN.endPacket();
  }
  
  int packetSize = CAN.parsePacket();

  if (packetSize) {
    Serial.print("Received packet with id ");
    Serial.print(CAN.packetId(), HEX);
    Serial.print(": ");
    while (CAN.available()) {
      Serial.print(CAN.read());
    }
    Serial.println();
  }


}