/**
 * test sketch for ThermometerUI
 * 
 * This sketch acts as a host for the I2C communications
 * and controls just the ThermometerUI part of the project
 * 
 * See https://github.com/rogertheriault/thing-thermometer
 * Copyright (c) 2018 Roger Theriault
 * Licensed under the MIT license.
 * 
 * To test: 
 * * program any Arduino with this code
 * * program an Arduino Uno (or suitable Arduino that supports I2C slave mode)
 *   with the Thermometer UI code
 * * wire up the ArduinoUI per the instructions in that sketch
 * * connect GND, SDA and SCK between the two devices
 * * connect a power source to the ThermometerUI or connect its VIN pin to
 *   the 5V output pin of the other device
 * * connect a USB cable to the test Arduino
 * * open the serial port at 9600 baud
 * * you should see a hex dump of the 6 bytes being read from ThermometerUI
 *   and the first two bytes converted to an integer representing degrees C
 * * the remaining bytes will be 0x20 (space) (padding set by that code)
 * * if the ThermometerUI is not sending, then the hex dump will be all 0xFF
 * * control the LED by typing into the serial console:
 * * "sr" sets the led color to red, "sb" to blue, "sg" to green, and s[anything else]
 *   will turn off the led (set the color to black)
 * * a1 enables the alarm state, a0 disables it
 *
 *
 * Based on Wire demo code by Nicholas Zambetti <http://www.zambetti.com>
 * See https://www.arduino.cc/en/Tutorial/MasterWriter and
 * https://www.arduino.cc/en/Tutorial/MasterReader
 */

#include <Wire.h>
#define SLAVE_I2C 8
#define REGISTER_LENGTH 6

// storage for the data from the I2C slave device
byte slaveRegister[REGISTER_LENGTH];
unsigned int currentTemp = 0;

// storage for the command data from Serial in
#define MAX_BUF 3
char rxbuf[MAX_BUF];

void setup() {
  // open the I2C bus as master
  Wire.begin();
  Serial.begin(9600);
}

void loop() {

  // request data from the slave (triggers the corresponding code's Wire.onRequest callback)
  Wire.requestFrom(SLAVE_I2C, REGISTER_LENGTH);

  // read the I2C bus if there's available data
  // and save it in slaveRegister
  int bufpos = 0;
  while (Wire.available()) {
    byte b = Wire.read();  // one byte at a time
    if (bufpos < REGISTER_LENGTH) {
      slaveRegister[bufpos] = b; // save it
      bufpos++;
    }
  }
  // fill the buffer with 0xFF
  for (; bufpos < REGISTER_LENGTH; bufpos++) {
    slaveRegister[bufpos] = 0xff;
  }

  // debugging log to Serial
  for (bufpos = 0; bufpos < REGISTER_LENGTH; bufpos++) {
    Serial.print("0x");
    Serial.print(slaveRegister[bufpos], HEX);
    Serial.print(" ");
  }
  Serial.println();

  // the first probe's temperature is the first two bytes
  // here we are shifting the first byte by 8 bits and adding the lower byte
  currentTemp = slaveRegister[0] << 8 | slaveRegister[1];
  Serial.println( currentTemp );

  // handle any commands from the user/tester
  if (Serial.available() > 0) {
    bufpos = 0;
    while (Serial.available()) {
      char rx_byte = Serial.read();
      // any bytes that are outside the ascii character range (such as newlines)
      // are read and discarded, as are any bytes beyond the max length we allow
      if (rx_byte >= '0' && rx_byte <= 'z' && bufpos < MAX_BUF) {
        // save the first few bytes, our command
        rxbuf[bufpos] = rx_byte;
        bufpos++;
      }
    }
    // send the command to the slave device
    send_i2c(rxbuf, bufpos);
  }

  // delay 1/2 a second before looping again
  delay(500);
}

void set_alarm(boolean onOff) {
  char msg[2];
  msg[0] = 'a';
  msg[1] = onOff ? '1' : '0';
  send_i2c(msg, 2);
}

void set_shade(char shade) {
  char msg[2];
  msg[0] = 's';
  msg[1] = shade;
  send_i2c(msg, 2);
}

// send a string of characters to the slave device
void send_i2c(const char *s, int len) {
  // sending to a slave involves starting, writing bytes, then ending
  Wire.beginTransmission(SLAVE_I2C);
  for (int i = 0; i < len; i++) {
    Wire.write(s[i]);
  }
  Wire.endTransmission();    // stop transmitting
}


