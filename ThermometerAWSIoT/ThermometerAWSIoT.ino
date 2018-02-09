// An AWS IoT ESP8266 "kitchen/bbq" thermometer
//
// See https://github.com/rogertheriault/thing-thermometer
//
// Copyright (c) 2018 Roger Theriault
// Licensed under the MIT license.

// use this to differentiate code for an Adafruit HUZZAH ESP8266 instead of ESP32
// #ifdef ESP8266

// copy config-sample.h to config.h and edit it
#include "config.h"

// Piezo Buzzer on GPIO12
#define ALARMPIN 12 // GPIO12

// user button on GPIO3
#define BUTTONPIN 3 // GPIO3

// one-wire DS18B20 on GPIO5 (add 4.7k pullup)
#define OWBUS 5 // GPIO5

// NeoPixel / WS2811 data pin
#define NEOPIXELPIN 12 // GPIO12

// E-Paper
#define EPAPER_RST 2 // GPIO2
#define EPAPER_DC 0 // GPIO0


#include "FS.h"
#ifdef ESP8266
  #include <ESP8266WiFi.h>
  #include <ESP8266WiFiMulti.h>
  #include <ESP8266HTTPClient.h>
#else
  #include <WiFi.h>
  #include <WiFiMulti.h>
  #include <HTTPClient.h>
  #include <SPIFFS.h>
#endif



// globals for our state
String default_step_text = "\"Alexa, ask Kitchen Helper to make yogurt\"";
String default_title = "Kitchen Helper";
String cooking_mode = "measure";
boolean in_alarm_state = false;
int currentTemp = -127; // initializing to -127 should trigger an update on reset
boolean watching_high = false;
boolean watching_low = false;
boolean display_update = true;
boolean shadow_update = true;
int alarm_high = 0;
int alarm_low = 0;
int button_state = 0;
int recipe_step = 0;
String recipe_step_text = default_step_text;
String recipe_title = default_title;
const char *aws_topic_update = "$aws/things/ESP8266Test1/shadow/update";
const char *aws_topic_delta = "$aws/things/ESP8266Test1/shadow/update/delta";

#include "annunciators.h";
#include "thingdata.h";
#include "display.h"
#include "sensors.h"
#include "awsiot.h"


/**
 * setup
 */
void setup() {
  // Serial port for debugging
  Serial.begin(115200);
  Serial.print(F("heap size: "));
  Serial.println(ESP.getFreeHeap());
  
  SPIFFS.begin();

  // initialize the display
  display.init();
  display.setRotation(1);
  
  delay(2000);
  Serial.setDebugOutput(1);

  // set everything up
  setup_pixel();
  setup_wifi();
  setup_sensors();
  setup_alarm();
  setup_recipe_data();

  setup_awsiot();

  
  if (connect()) {
    subscribe();
    //updateShadow();
  }
  Serial.print(F("setup heap size: "));
  Serial.println(ESP.getFreeHeap());
}

// keep track of when our last temperature reading was taken
long lastReading = 0;

void loop() {

  check_button();
  check_alarm();

  // TODO
  //check_thermometers();
  
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
      shadow_update = true;
      display_update = true;
    }
    Serial.print(F("LOOP heap size: "));
    Serial.println(ESP.getFreeHeap());
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

  updateShadow();
  updateDisplay();
  checkModeTimer();
}








