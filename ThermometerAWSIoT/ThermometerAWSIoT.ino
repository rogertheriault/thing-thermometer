// An AWS IoT ESP8266 "kitchen/bbq" thermometer
//
// See https://github.com/rogertheriault/thing-thermometer
//
// Copyright (c) 2018 Roger Theriault
// Licensed under the MIT license.

// copy config-sample.h to config.h and edit it
#include "config.h"

// Piezo Buzzer on GPIO12
#define ALARMPIN 12 // D6(=12)

// user button on GPIO3
#define BUTTONPIN 3

#include "FS.h"
#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>
#include <ESP8266HTTPClient.h>
#include <ArduinoOTA.h>


// globals for our state
String cooking_mode = "";
boolean in_alarm_state = false;
int currentTemp = -127; // initializing to -127 should trigger an update on reset
boolean watching_high = false;
boolean watching_low = false;
int alarm_high = 0;
int alarm_low = 0;
int button_state = 0;
int recipe_step = 0;
String recipe_step_text = "measuring";
String recipe_title = "Kitchen Helper";


#include "display.h"
#include "sensors.h"
#include "awsiot.h"


/**
 * setup
 */
void setup() {
  // Serial port for debugging
  Serial.begin(115200);
  SPIFFS.begin();

  // initialize the display
  display.init();
  display.setRotation(1);
  
  delay(2000);
  Serial.setDebugOutput(1);

  // set everything up
  setup_wifi();
  setup_OTA();
  setup_sensors();
  setup_alarm();
  setup_recipe_data();

  setup_awsiot();
  
  if (connect()) {
    subscribe();
    //updateShadow();
  }
}

// keep track of when our last temperature reading was taken
long lastReading = 0;

void loop() {
  // remove if there's no need to OTA update
  ArduinoOTA.handle();

  check_button();
  check_alarm();
  
  long now = millis();
  // check temp every 10 seconds - TODO use watchdog timer
  if (now - lastReading > 10000) {
    lastReading = now;
    sensors.requestTemperatures();
    float tempC = sensors.getTempC(tempsens);
    int temp = (int)tempC;

    // send temp to shadow if changed
    boolean needUpdate = false;
    if (temp != currentTemp) {
      currentTemp = temp;
      needUpdate = true;
    }
    
    if ( shouldAlarm() ) {
      in_alarm_state = true;
      needUpdate = true;
    }

    // if something has changed, update the shadow and display
    if ( needUpdate ) {
      updateShadow();
      updateDisplay();
    }
  }


  // keep mqtt up and running
  if (awsWSclient.connected()) {    
    client->yield();
  } else {
    // handle reconnection
    if (connect()) {
      subscribe();      
    }
  }

}


// piezo buzzer
void setup_alarm() {
  pinMode(ALARMPIN, OUTPUT);
  digitalWrite(ALARMPIN,LOW);
  beep(50);
  beep(50);
  beep(50);
  in_alarm_state = false;
}
void check_alarm() {
  if (in_alarm_state) {
    beep(400);
  }
}

void beep(unsigned char delayms) {
  analogWrite(ALARMPIN, 96); // best sound from tmb12a05 piezo at 3.3v
  delay(delayms);
  analogWrite(ALARMPIN, LOW);
  delay(delayms);
}




void setup_OTA() {
  delay(10);
  //ArduinoOTA.setHostname("Lounge");

  ArduinoOTA.onStart([]() {
    Serial.println("Start");
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("\nEnd");
  });
 ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
});
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
    else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
    else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
    else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
    else if (error == OTA_END_ERROR) Serial.println("End Failed");
  });
  ArduinoOTA.begin();
   
}

// check the sensor state
// returns true if one of our limits was exceeded, false otherwise
boolean shouldAlarm( ) {
  if ( watching_high && ( currentTemp >= alarm_high ) ) {
    watching_high = false;
    return true;
  }
  if ( watching_low && ( currentTemp <= alarm_low ) ) {
    watching_low = false;
    return true;
  }
  return false;
}





