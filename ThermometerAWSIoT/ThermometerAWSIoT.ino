/**
 * ThermometerAWSIoT
 * 
 * An AWS IoT ESP8266/Arduino "kitchen/bbq" thermometer
 *
 * See https://github.com/rogertheriault/thing-thermometer
 * 
 * This sketch is for the main component of the Thermometer Thing (ESP8266 + display)
 * The sketch named ThermometerUI is for the peripheral control (Arduino Uno + probes etc)
 * 
 * The main component handles WiFi and IoT state, main thermometer state logic,
 * fetches recipe data and reports the shadow state over an MQTT connection to AWS,
 * and controls the e-paper display
 * 
 * The peripheral component is an I2C slave controlled by the main component. It
 * will measure and report the probe temperature(s), sound an audible alert when needed,
 * display status on a neopixel ring, and accept user button presses to silence the alarm
 * 
 * Currently the recommended device for the main component is an Adafruit HUZZAH ESP8266 or
 * any ESP8266 development board, these are programmable via the Arduino IDE
 * 
 * The recommended device for the peripheral control is an Arduino Uno. Any Arduino device that
 * supports I2C slave will work, though there are many that do not support it and such lack of
 * support is not well documented.
 * 
 * Connections (assuming an Adafruit HUZZAH ESP8266)
 * 
 * Waveshare 1.54" b/w e-paper display (connected via SPI bus)
 * (colors are somewhat consistent in supplied units but check anyway)
 * * GND to GND (black)
 * * 3.3v to VCC (red)
 * * GPIO13 (MO or MOSI) to DIN (blue)
 * * GPIO14 (SCK) to CLK (yellow)
 * * GPIO2 to RST (white)
 * * GPIO0 to DC (green)
 * * GPIO15 to CS (orange)
 * * GPIO12 to BUSY (purple)
 * 
 * I2C to the Peripheral Device
 * * connect GND of each device together (important)
 * * connect SDA of the ESP (GPIO4) to SDA on the slave (pin A4 on Uno)
 * * connect SCK of the ESP (GPIO5) to SCK on the slave (pin A5 on Uno)
 * 
 * You may notice you're pretty much out of usable pins now, that's
 * why the sensors and LEDs are connected to the slave device. The ESP8266
 * has a fast processor, lots of memory, and WiFI, but only one ADC pin and
 * not many GPIO pins. So this project also relies on an Arduino as a peripheral,
 * See the sketch for ThermometerUI
 *
 * Copyright (c) 2018 Roger Theriault
 * Licensed under the MIT license.
 * 
 */

// copy config-sample.h to config.h and edit it
#include "config.h"


// e-paper pins
// default SPI:
// DIN = 13 (MOSI)
// CLK = 14 (SCK)
// selectable:
#define EPAPER_RST 2
#define EPAPER_DC 0
#define EPAPER_CS 15
#define EPAPER_BUSY 12 //4

// if you have the 3-color (red or yellow + white & black) display uncomment this
//#define DISPLAY_3COLOR true

// I2C library
#include <Wire.h>
// I2C slave address
#define SLAVE_I2C 8

// SPIFFS filesystem
#include "FS.h"

// WiFi libraries
#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>
#include <ESP8266HTTPClient.h>

// globals for our state
String default_step_text = "\"Alexa, ask Kitchen Helper to make yogurt\"";
String default_title = "Kitchen Helper";
String cooking_mode = "measure";
boolean update_alarm = true; // flag to indicate a state change
boolean in_alarm_state = false;
unsigned int currentTemp = 0; // initializing to 0 should trigger an update on reset
long lastReading = 0; // last reading tiome in millis
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

// include the other bits - rather that separate headers and c files we've thrown it all in the header file,
// mostly for simplicity for the time being

// communications with the peripheral Arduino (ThermometerUI)
#include "peripherals.h";

// fetch and store/access Thing Data
#include "thingdata.h";

// e-paper display updates
#include "display.h"

// IoT communications over Wifi/MQTT with AWS IoT
#include "awsiot.h"


/**
 * setup
 */
void setup() {
  // Serial port for debugging
  Serial.begin(115200);
  Serial.print(F("heap size: "));
  Serial.println(ESP.getFreeHeap());

  setup_peripherals();
  pixel_start();

  // start the filesystem
  SPIFFS.begin();

  // initialize the display
  display.init();
  display.setRotation(1);
  
  delay(2000);
  Serial.setDebugOutput(1);

  // set everything up
  setup_wifi();
  setup_recipe_data();

  setup_awsiot();

  // keep the MQTT connection alive
  if (connect()) {
    subscribe();
  }
  Serial.print(F("setup heap size: "));
  Serial.println(ESP.getFreeHeap());
  delay(500);
  update_alarm = true;
  in_alarm_state = false;
  pixel_off();
}


// main loop
void loop() {

  check_alarm();

  // check the thermometer state
  check_thermometers();

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








