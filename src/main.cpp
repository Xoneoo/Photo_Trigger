

/*
##########################################################################################################################################################
# Photo Trigger via a light barrier controlled by an AVRMega MCU including Auto ADC learning on startup and Battery Low indicator 
#
# LED description (on Autolearn Mode):
# RunLED ON + Heartbeat 0.5Hz = waiting for trigger by Photosensor, Ready
# RunLED ON + Heartbeat 2Hz = waiting for trigger by Photosensor, Battery below warning threshold
# RunLED ON + Heartbeat 5Hz = waiting for trigger by Photosensor, Battery below lowlevel threshold
# ErrorLED ON + Heartbeat OFF + RunLED OFF = ADCautolearn failed (IR Transmitter and Receiver have probably no visual connection), resume by MCU Reset
# ErrorLED ON + Heartbeat OFF or stuck + RunLED ON = Photosensor stuck at trigger (IR transmitter and Receiver have probably no visual connection),
# auto resume if Photosensor see the Transmitter again 
##########################################################################################################################################################
*/

#include <Arduino.h>

//library for Heartbeat
#include "HeartBeat.h"
HeartBeatSL HB;

//debugging via serial output
//#define debug
//#define debug1
//#define debug2
//#define debug3 //Battery ADC debugging

//Battery low check
#define BattLow_check
#if defined BattLow_check
  #define ADC_BattWarning 500 //need to be set by debug3
  #define ADC_BattLow 490     //need to be set by debug3
  int BattValue =0;
  int Battcheck =0;
  int bi = 0; 
#endif

//Pins if Arduino Nano are used
#if defined(__AVR_ATmega328P__)
  #define HeartbeatLED 2
  #define RunLED 3
  #define ErrorLED 13 //Nano Onboard LED
  #define TriggerOut 9
  #define PhotoSensor A1
  #define BattSensor A2  
#endif

//Pins if Arduino Pro Micro are used
#if defined(__AVR_ATmega32U4__) 
  #define HeartbeatLED 
  #define RunLED 
  #define ErrorLED 
  #define TriggerOut 
  #define PhotoSensor A7
  #define BattSensor A0
#endif

//Invert TriggerOut logic to low active
//#define TOInvert

//Automatic learn Phototransistors ADC value 
#define ADCautolearn
#ifdef ADCautolearn
  #define minLimit 500
  #define offset 150
  #define learncount 25 
  int photo_value = 0;
  int photo_trigger = 0;
#endif

#ifndef ADCautolearn
  #define photo_trigger 500
  int photo_value = 0;
#endif

#define triggerholddelay 150
#define error_threshold 1000
unsigned long previousMillis= 0;

//###############################################
//Setup
void setup() {
  Serial.begin(9600);
  Serial.println("PhotoTrigger started v1.0");
  Serial.println("serial ready");
  pinMode(HeartbeatLED, OUTPUT);
  pinMode(RunLED, OUTPUT);
  pinMode(ErrorLED, OUTPUT);
  pinMode(TriggerOut, OUTPUT);

  #ifdef TOInvert
    digitalWrite(TriggerOut, HIGH);
  #else
    digitalWrite(TriggerOut, LOW); 
  #endif

#ifdef ADCautolearn
  Serial.println("ADC Autolearn Start");
  photo_value = analogRead(PhotoSensor);

  for (int i=0; i <= learncount; i++){
    #ifdef debug1
      Serial.println(photo_trigger);
    #endif  
    photo_trigger = photo_trigger + analogRead(PhotoSensor);
    delay(80);
  }

  photo_trigger = photo_trigger/(learncount+1);

  #ifdef debug
    Serial.print("photo_trigger ");
    Serial.println(photo_trigger);
  #endif

  if (photo_trigger < minLimit) {
    digitalWrite(ErrorLED,HIGH);
    Serial.println("Autolearn error > reset needed");
    while(1) {
      //stop all, need mcu reset
    }
  }

  if (photo_trigger >= minLimit) {
    photo_trigger = photo_trigger - offset;
    digitalWrite(RunLED,HIGH);
    Serial.print("Autolearn finished, will trigger below ");
    Serial.println(photo_trigger);
  }
#endif 

#ifndef ADCautolearn
  digitalWrite(RunLED,HIGH);
#endif

HB.begin(HeartbeatLED, 0.5);
HB.setDutyCycle(50);

}

//###############################################
//Battery voltage check
#ifdef BattLow_check
void checkbatt(){
  BattValue = BattValue + analogRead(BattSensor);
    bi = bi + 1;
    if (bi >= 25)
    {
      bi = 0;
      BattValue = BattValue / 25; 
      #ifdef debug3
        Serial.print("BattSensor Value: ");
        Serial.println(BattValue);
        delay(200);
      #endif

      if (BattValue <= ADC_BattLow)
      {
        #ifdef debug3
          Serial.print("LowBattSensor Value: ");
          Serial.println(BattValue);       
        #endif
        HB.setFrequency(5);
        HB.setDutyCycle(50);
      }

      if (BattValue <= ADC_BattWarning && BattValue > ADC_BattLow)
      {
        #ifdef debug3
          Serial.print("WarnBattSensor Value: ");
          Serial.println(BattValue);       
        #endif
        HB.setFrequency(2);
        HB.setDutyCycle(10);
      }

      if (BattValue > ADC_BattWarning && BattValue > ADC_BattLow)
      {
        #ifdef debug3
          Serial.print("ResumeBattSensor Value: ");
          Serial.println(BattValue);       
        #endif
        HB.setFrequency(0.5);
        HB.setDutyCycle(50);
      }
    } 
}
#endif

//###############################################
//Photo barrier check
void checkbarrier(){
  unsigned long currentMillis = millis();
  photo_value=analogRead(PhotoSensor);
  if (photo_value <= photo_trigger)  {
    #ifdef TOInvert
      digitalWrite(TriggerOut, LOW);
    #else
      digitalWrite(TriggerOut, HIGH); 
    #endif
    delay(triggerholddelay);
    previousMillis = currentMillis;
    #ifdef TOInvert
      digitalWrite(TriggerOut, HIGH);
    #else
      digitalWrite(TriggerOut, LOW); 
    #endif
    while (photo_value <= photo_trigger){
      photo_value=analogRead(PhotoSensor);
      currentMillis = millis();
      if (currentMillis - previousMillis > error_threshold) {
        digitalWrite(ErrorLED,HIGH);  
      }
      #ifdef debug1
      Serial.print("millis: ");
      Serial.println(currentMillis);
      Serial.println(previousMillis);
      delay(100);
      #endif
    }
    digitalWrite(ErrorLED,LOW);    
  }
  #ifdef debug1
    Serial.print("Raw Sensor Value ");
    Serial.println(analogRead(PhotoSensor));
    delay(300);
  #endif
}

//###############################################
//Loop
void loop() {
  //start Heartbeat LED
  HB.beat();
  
  //check light barrier
  checkbarrier();

  // checking battery voltage
  #ifdef BattLow_check
    checkbatt();
  #endif
  
}

