/**
 * ThermometerUI
 * 
 * This sketch runs the peripherals for the Thing Thermometer
 * * an Arduino Uno (or any arduino device that supports I2C slave mode)
 * * a DS18B20 waterproof temperature probe for measuring liquid temps (max 150C)
 * * FUTURE: two thermistor probes for measuring BBQ chambers or meat temperatures
 * * a neopixel ring to display alert status and thermometer states
 * * a piezo buzzer to audibly alert anyone nearby that a setpoint was reached
 * * a button to turn off the audible alert, and FUTURE to put the device to sleep
 * 
 * Connections (assuming an Arduino Uno)
 * 
 * Neopixel Ring
 * * GND to Arduino GND, VCC to Arduino 5V (3.3V is also ok)
 * * Data In to Arduino digital pin 6
 * 
 * Piezo Buzzer
 * * GND to Arduino GND
 * * + to a 100 ohm resistor, then connect that resistor to Arduino digital pin 9
 * 
 * Momentary Pushbutton
 * * one leg to Arduino +5V (3.3V is also ok)
 * * the other leg should have a 10k ohm resistor to GND
 * * the other leg should also connect directly to Arduino digital pin 12
 * 
 * DS18B20 Temperature Probe
 * You may prefer to solder a 3.5mm plug to the thermometer cable, and use a
 * corresponding socket to enable easy connection
 * * GND and VCC connect to the GND and +5V (or 3.3V if that's the voltage your Arduino uses)
 * * the yellow wire (data) should have a 4.7k ohm resistor to +5V (or 3.3V)
 * * and data should also be directly connected to Arduino digital pin 10
 * 
 * Thermistor probes
 * These are not yet supported, but if you have one, you'll use a resistor pair and connect
 * to any of the unused Analog pins, then put the temperature measurement into the 6 byte array
 * 
 * I2C to the Master Device (or I2C "bus")
 * * connect GND of each device together (important)
 * * connect SDA (pin A4 on Uno) to SDA of the master
 * * connect SCK (pin A5 of the Uno) to SCK of the master
 * * if you need to power the Uno from the master, connect VIN of the Uno to +5V on the master
 * * the master may have pins that supply the USB 5V, that works too.
 *
 * NOTE: if the master is a 3.3V device, the Uno typically shouldn't damage it
 * because of how I2C works, but if you're unsure, obtain and use appropriate level
 * shifter circuitry in the I2C connection
 * 
 * See https://github.com/rogertheriault/thing-thermometer
 * Copyright (c) 2018 Roger Theriault
 * Licensed under the MIT license.
 * 
 */

// Wire library for the I2C comms
#include <Wire.h>
// Our slave will respond to address 0x08 (or 8 decimal)
// this should match the address used by the host
#define SLAVE_I2C 8

// Libraries and settings for the DS18B20 probe on pin 10
#include <DallasTemperature.h>
#define ONE_WIRE_BUS 10
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);
DeviceAddress tempsens;
// keep track of the last temperature reading
volatile unsigned int lastTemp;

// I2C command buffer
#define  MAX_SENT_BYTES   3
byte receivedCommands[MAX_SENT_BYTES];
// this flag is set to true if a command needs to be processed
boolean newCommands = false;

// Fancy LED stuff for the NeoPixel ring (or any neopixels you care to use) on pin 6
#include "FastLED.h"
// set this to the number of pixels you have, they vary
#define NUM_LEDS 8

CRGB leds[NUM_LEDS];
#define PIXELPIN 6
int pixel_id = 0;
CRGB shade = CRGB::Green;

// if we're "in alarm state" this will be true
// alarm state is meant to get a user's attention, and involves multiple parts:
// the button, the LEDs, and the piezo buzzer
boolean in_alarm_state = false;

// the piezo buzzer is on pin 9, the momentary pushbutton on 12
#define ALARMPIN 9
#define BUTTONPIN 12

void setup() {
  // initiate the sensors
  sensors.begin();
  // Serial here is only needed for debugging, and could be removed
  Serial.begin(9600);

  // set up the alarm portion of the code
  setup_alarm();

  // look for the one DS18B20 which should be at slot 0
  if ( !sensors.getAddress(tempsens, 0) ) {
    // TODO likely it's not plugged in, we should display a message, keep checking, and not report anything
    Serial.println(F("Unable to find address for Device 0"));
    lastTemp = 0;
  }
  sensors.setResolution(tempsens, 9);
  Serial.println("started");

  // set up the neopixel code
  setup_pixel();

  // open the I2C bus as a slave by supplying our address
  Wire.begin(SLAVE_I2C); 

  // register callback handlers for event requests and receiving data
  Wire.onRequest(requestEvent);
  Wire.onReceive(receiveEvent);
}

// main loop, just check everying as quickly as we can...
void loop() {
  check_button();
  
  check_alarm();
  
  delay(100);
  
  checkTemperature();

  // handle I2C command that was received and stored in the buffer by the callback
  if (newCommands) {
    // alarm mode change a1 on a0 off
    if (receivedCommands[0] == 'a') {
      if (receivedCommands[1] == '1') {
        shade = CRGB::Red;
        pixel_all();
        in_alarm_state = true;
      } else {
        pixel_off();
        in_alarm_state = false;
      }
    }
    // shade change - this can be expanded as needed
    if (receivedCommands[0] == 's') {
      switch (receivedCommands[1]) {
        case 'a':
          shade = CRGB::DarkOrange;
          break;
        case 'r':
          shade = CRGB::Red;
          break;
        case 'b':
          shade = CRGB::Blue;
          break;
        case 'g':
          shade = CRGB::Green;
          break;
        default: // off
          shade = CRGB::Black;
          break;
      }
      pixel_all();
    }
    newCommands = false;
  }
  // animate the neopixel ring in whatever way we desire
  pixel_move();
}

/**
 * callback handler for the I2C MASTER sending this device a data request
 * 
 * this function is registered as an event, see setup()
 * keep the temperature states in a buffer, and simply send the current
 * buffer when it's requested
 *
 * the code here needs to just quickly send the data, nothing else
 */
void requestEvent() {
  // lastTemp, 2 bytes
  Wire.write( (unsigned char) (lastTemp >> 8) ); // high
  Wire.write( (unsigned char) (lastTemp & 0xff) ); // low
  for (int i = 2; i < 6; i++) {
    Wire.write(" "); // respond with message of 6 bytes
  }
}

/**
 * callback handler for the I2C MASTER sending this device a command
 * 
 * this function is registered as an event, see setup()
 *
 * like the sending callback, we have to be quick, so the code is
 * just reading all the data (even if it's more than we expected)
 * and storing the first few characters in the buffer
 */
void receiveEvent(int bytesReceived) {
  Serial.println(bytesReceived);
  for (int a = 0; a < bytesReceived; a++) {
    if ( a < MAX_SENT_BYTES) {
      receivedCommands[a] = Wire.read();
    } else {
      Wire.read();  // if we receive more data then allowed just throw it away
    }
    newCommands = true;
  }
}

// this variable keeps track of the last millis() timestamp so we can 
// avoid requesting the temperature too often
long lastReading = 0;

/**
 * get the probe temperature and update the stored value
 * 
 * this code could also obtain thermistor readings and store multiple
 * temperatures
 * 
 */
void checkTemperature() {
 long now = millis();
 Serial.print(".");
  // check temp every 5 seconds - TODO use watchdog timer
 if (now - lastReading > 5000) {
    lastReading = now;
    // ask the temperature probes on the OneWire bus to take a measurement
    sensors.requestTemperatures();
    // now obtain the Celsius temp of our one device
    float tempC = sensors.getTempC(tempsens);
    if (tempC < 0) {
      tempC = 0;
    }
    // deliberately changing this to an integer because the precision is not
    // important and we'll be sending just the int value to the host device
    lastTemp = (unsigned int) tempC;
    Serial.println(lastTemp);
  }
}

// set up the neopixel ring
void setup_pixel() {
  LEDS.addLeds<WS2811, PIXELPIN, GRB>(leds, 8);

  shade = CRGB::Orange;
  pixel_all();
  delay(5000);
}

// generate some pixel motion periodically
void pixel_move() {

  // this block of code makes all the pixels "breathe"
  // see https://gist.github.com/hsiboy/4eae11073e9d5b21eae3
  float breath = (exp(sin(millis()/2000.0*PI)) - 0.36787944)*108.0;
  FastLED.setBrightness(breath);

  // This block of code rotates one lit pixel around the ring
  /*
  leds[pixel_id] = CRGB::Black;
  pixel_id += 1;
  if ( pixel_id >= NUM_LEDS) {
    pixel_id = 0;
  }
  leds[pixel_id] = shade;
  */
  
  FastLED.show();
}

// turn on all pixels
void pixel_all() {
  for (int i = 0; i < NUM_LEDS; i++) {
    leds[i] = shade;
  }
  FastLED.show();
}

void pixel_off() {
  shade = CRGB::Black;
  pixel_all();
}


// piezo buzzer

// this is just an inefficient beep
// TODO remove the delays and use timing loop or an interrupt
void beep(unsigned char delayms) {
  analogWrite(ALARMPIN, 97); // best sound from tmb12a05 piezo at 5v with 100 ohm resistor
  delay(delayms);
  analogWrite(ALARMPIN, LOW);
  delay(delayms);
}

// called only once, in setup
void setup_alarm() {
  pinMode(ALARMPIN, OUTPUT);
  digitalWrite(ALARMPIN,LOW);
  beep(50);
  beep(50);
  beep(50);
}

// if the alarm flag is set, beep
void check_alarm() {
  if (in_alarm_state) {
    beep(200);
  }
}

int button_state = 1;

// very simple code to turn off the alarm when the button state changes
void check_button() {
  // read the button state (1 = pressed, 0 = "open")
  int newState = digitalRead(BUTTONPIN);
  if (newState != button_state) {
    Serial.print("button now:");
    Serial.println(newState);
    button_state = newState;
    
    // assume any detected press is meant to shut off that horrible noise
    if (in_alarm_state) {
      in_alarm_state = false;
    }
  }
}

