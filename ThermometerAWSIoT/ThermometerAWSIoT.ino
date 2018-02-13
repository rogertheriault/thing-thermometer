// An AWS IoT ESP8266 "kitchen/bbq" thermometer
//
// See https://github.com/rogertheriault/thing-thermometer
//
// Copyright (c) 2018 Roger Theriault
// Licensed under the MIT license.

<<<<<<< Updated upstream
=======
// IMPORTANT
// Edit PubSubClient.h and change the define for max packet size thusly:
// #define MQTT_MAX_PACKET_SIZE 1024
// because that gets processed before any defines in here :-(
// Without this chaange, the long topic names and data will result in failed publish/subscribe

// to debug in the Arduino IDE, set the "Core Debug Level" to Verbose in the Tools dropdown
// This will result in the ESP_LOGV statements being compiled in.
// You can rebuild with that off and save a bunch of bytes later.

>>>>>>> Stashed changes
// copy config-sample.h to config.h and edit it
// also follow the instructions to install your AWS IoT certificates
#include "config.h"

// Piezo Buzzer on GPIO12
#define ALARMPIN 12 // D6(=12)

// user button on GPIO3
<<<<<<< Updated upstream
#define BUTTONPIN 3
=======
#define BUTTONPIN 3 // GPIO3

// one-wire DS18B20 on GPIO5 (add 4.7k pullup)
#define OWBUS 17 //25
// on ESP32 I found GPIO25 to be a stable GPIO for OneWire.
// Avoid the GPIO that can be used for touch
// details: https://www.espressif.com/sites/default/files/documentation/esp32_datasheet_en.pdf

// NeoPixel / WS2811 data pin
#define NEOPIXELPIN 26 // GPIO12

// E-Paper - ESP32 (recommended) or ESP8266
// note DIN of display (blue wire) goes to MOSI of ESP (GPIO23)
// and  CLK of display (yellow wire) goes to SCK of ESP (GPIO18)
#ifdef ESP32
#define EPAPER_RST 2
#define EPAPER_DC 0
#define EPAPER_CS 5
#define EPAPER_BUSY 4
#else
#define EPAPER_RST 2
#define EPAPER_DC 0
#define EPAPER_CS 15
#define EPAPER_BUSY 4
#endif
// use if you have an RGB, ahd edit display.h if you have other than 1.54"
//#define DISPLAY_3COLOR
>>>>>>> Stashed changes
#define EPAPER_RST 2
#define EPAPER_DC 0
#define EPAPER_CS 15
#define EPAPER_BUSY 4

#include "FS.h"
#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>
#include <ESP8266HTTPClient.h>

static const char* TAG = "ThermometerAWSIoT";

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
<<<<<<< Updated upstream
=======
const char *aws_topic_update = "$aws/things/ThermometerThing_001/shadow/update";
const char *aws_topic_delta = "$aws/things/ThermometerThing_001/shadow/update/delta";
>>>>>>> Stashed changes

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








