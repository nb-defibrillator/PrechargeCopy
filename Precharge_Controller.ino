/**
Authors:
Natu Benyam Demeke
Violet Enslow
Ryanne Wilson


Precharge Controller
*/

#include <CANSAME5x.h>

CANSAME5x CAN;

#define ULONG_MAX 4294967295UL //Serves as a 'null' state for the timers

//pin IDs
const uint8_t AIR_Precharge = 4; 
const uint8_t AIR_Main = 5;
const uint8_t AIR_Discharge = 13;
const uint8_t BPS_Fault = 6;
const uint8_t LED_Discharge = 23;
const uint8_t LED_Fault = 10;

const uint8_t Optocoupler = 9;
const uint8_t BMS_DischargeEn = 11;   
const uint8_t BMS_MPO = 12; // BMS Multi-Purpose Output

bool carRunning = false; //True when the car can start driving ; as in, when precharging has finished and is successful.
bool prechargeFailed = false; //True if the precharge failed.
bool dischargeFinished = false; // True if the discharge finished!

//All measurements of time are in milliseconds!

const long bitrate = 250e3; // 250 kbps

unsigned long initalizeStart = ULONG_MAX; // Start time for waiting for Discharge Enable signal from BMS
const unsigned long initalizeTimeout = 5e3; // 0.5s -- amount of time to wait for Discharge Enable to initalize before faulting

unsigned long prechargeStart = ULONG_MAX; // The time at which precharge started; in millis; ULONG_MAX acts as a 'null' here.
const unsigned long prechargeTimeoutInterval = 10e4; // 10s ---amount of time that has to pass to mean the precharge failed; in millis
const unsigned long prechargeInterval = 1.5e4; // 1.5s -- amount of time necessary to precharge; in millis
bool prechargeTimedOut = false;

unsigned long optocouplerActivatedStart = ULONG_MAX; // The time at which Optocoupler was active; in millis

unsigned long dischargeStart = ULONG_MAX;
const unsigned long dischargeInterval = 7.79e7; // 77.9s; the amount of time it takes to discharge; in millis;

// CANbus constants
const int correctID = 0x003;
const int HITEMP_INDEX = 2; // 8;  // Index of High Temprature in CAN message
const int OFFSET = -40;


// /**
//  * Author: Ryanne Wilson
//  * BPS Arduino for Props.
//  * 
//  */


 // If MPS is HIGH or Hitemp >= 55, Fault (HIGH; 5V) else output LOW; 0V

// void loop(){
//   if(CAN.available()){
//     CanMsg const msg = CAN.read();
//     Serial.println(msg);
//     int idMessage = msg.getStandardid();
//     Serial.print("IDMessage= ");
//     Serial.println(idMessage);

//     // offset -40
//     int temp = msg.data[HITEMP_INDEX];
//     temp -= OFFSET;

//     if (temp >= 55){
//       // Send out a fault!
//     }
    
//   }
// }


void setup() {
  // put your setup code here, to run once:
  pinMode(AIR_Precharge, OUTPUT);     // Normally open- LOW = open, HIGH = closed
  pinMode(AIR_Main, OUTPUT);          // Normally open
  pinMode(AIR_Discharge, OUTPUT);     // Normally closed
  
  pinMode(BPS_Fault, OUTPUT);         // Normally open
  //TODO: Add Fault LED
  pinMode(LED_Discharge, OUTPUT);
  pinMode(Optocoupler, INPUT_PULLUP); // Uses internal pullup resistors; default HIGH -> active LOW
  pinMode(LED_Fault, OUTPUT);
  
  pinMode(BMS_MPO, INPUT);            // Behavior depends on BMS settings
  pinMode(BMS_DischargeEn, INPUT);

  pinMode(PIN_CAN_STANDBY, OUTPUT);
  digitalWrite(PIN_CAN_STANDBY, false); // turn off STANDBY
  pinMode(PIN_CAN_BOOSTEN, OUTPUT);
  digitalWrite(PIN_CAN_BOOSTEN, true); // turn on booster

  while(!Serial);
  Serial.begin(115200);
   

  if(!CAN.begin(bitrate)){
       Serial.println("CAN.begin(...) failed.");
       for(;;) {}
     }

  digitalWrite(AIR_Discharge, HIGH);

  if(!digitalRead(BMS_DischargeEn)) {
    initalizeStart = millis();
    while(millis() - initalizeStart < initalizeTimeout) {
      Serial.println("Waitng for BMS Discharge Enable");
    }
  }
  if(!digitalRead(BMS_DischargeEn)) {
    Serial.println("Error: BMS Discharge Enable Timeout");
    fault();
  }

}

void readCAN() {
  int packetSize = CAN.parsePacket();

  if (packetSize) {
    Serial.print("Received packet with id ");
    long packetID = CAN.packetId();
    Serial.print(packetID, HEX);
    
    if (packetID == correctID) {
      CAN.read();
      CAN.read(); //gets to index 2
      int temp = CAN.read();
      CAN.flush();

      if (temp >= 55){
        fault(); 
      }

    }
    
  }
}

void precharge() {
  /**
  Precharge function for the circuit.

  if precharge failed -> deactivates AIR Precharge.
  if Optocoupler sends signal -> if has been precharging for enough time -> start car; deactivate AIR Precharge.
  */

  unsigned long now = millis();
  if (digitalRead(Optocoupler) == LOW) { //if Optocoupler is active
    
    
    //This part waits for a steady signal from the optocoupler, 
    //current duration .5s but that's just a guess.
    if (optocouplerActivatedStart == ULONG_MAX) optocouplerActivatedStart = now;

    unsigned long chargingInterval = now - optocouplerActivatedStart;

    if (chargingInterval > prechargeInterval) {
      carRunning = true;
      digitalWrite(AIR_Precharge, LOW);
      digitalWrite(AIR_Main, HIGH);
    }
  } 

  //timeout check
  unsigned long timeoutInterval = now - prechargeStart;
  if (timeoutInterval > prechargeTimeoutInterval) { 
    // Timed out; precharge has failed! fault :(
    prechargeFailed = true;
    digitalWrite(AIR_Precharge, LOW);
    digitalWrite(AIR_Main, LOW);
    digitalWrite(LED_Fault, HIGH);
    prechargeTimedOut == true;
    fault();
  }
}

void discharge() {
  digitalWrite(AIR_Discharge, HIGH);
  unsigned long now = millis();
  if (now - dischargeStart > dischargeInterval) {
    digitalWrite(LED_Discharge, HIGH);
    dischargeFinished = true;
  }
}

void fault() {
  while (true)
  digitalWrite(AIR_Main, LOW);
  digitalWrite(AIR_Discharge, HIGH);
}

void loop() {
  // put your main code here, to run repeatedly:
  
  readCAN();

  if (digitalRead(BMS_DischargeEn) == LOW || digitalRead(BMS_MPO) == HIGH || prechargeTimedOut == true) {
    fault();
  }

  if (carRunning == false) {
      if (digitalRead(BMS_DischargeEn) == HIGH ) { //&& digitalRead(BMS) == HIGH
        if (prechargeStart == ULONG_MAX) {   //if the prechargeStart hasn't yet been assigned         
            digitalWrite(AIR_Precharge, HIGH); //Closes AIR precharge
            prechargeStart = millis();
            }
        precharge();
      }
  }
  // else {
  //   if (!Switch2) {
  //     if (dischargeStart == ULONG_MAX) dischargeStart = millis();
  //     if (dischargeFinished == false) discharge();
  //   }
  // }
}
