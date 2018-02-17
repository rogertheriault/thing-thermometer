
// this code is involved with communicating and controlling the I2C slave
// (an Arduino Uno with the ThermometerUI sketch)
//

// buffer size we expect from the slave
#define REGISTER_LENGTH 6

// storage for the data from the I2C slave device
byte slaveRegister[REGISTER_LENGTH];

// storage for the command data from Serial in
#define MAX_BUF 3
char rxbuf[MAX_BUF];

void setup_peripherals() {
  // open the I2C bus as master
  Wire.begin();
}

// send a string of characters to the slave device
void send_i2c(const char *s, int len) {
  // sending to a slave involves starting, writing bytes, then ending
  Serial.print("sending I2C command ");
  Serial.println(s);
  
  Wire.beginTransmission(SLAVE_I2C);
  for (int i = 0; i < len; i++) {
    Wire.write(s[i]);
  }
  Wire.endTransmission();    // stop transmitting
}


void pixel_off() {
  char msg[] = "sx";
  send_i2c(msg, 2);
}

void pixel_start() {
  char msg[] = "sa";
  send_i2c(msg, 2);
}

void pixel_wifidone() {
  char msg[] = "sb";
  send_i2c(msg, 2);
}

void pixel_iotdone() {
  char msg[] = "sg";
  send_i2c(msg, 2);
  delay(300);
}

void pixel_alarm(boolean onOrOff) {
  char msg[2];
  msg[0] = 'a';
  msg[1] = onOrOff ? '1' : '0';
  send_i2c(msg, 2);
}




void check_alarm() {
  if (in_alarm_state) {
    pixel_alarm(true);
  } else {
    //pixel_alarm(false);
  }
}


// check the sensor state
// returns true if one of our limits was exceeded, false otherwise
boolean shouldAlarm( ) {
  // 0 is a special value - and must be ignored
  if ( currentTemp <= 0 ) {
    return false;
  }
  
  if ( alarm_high == 0 ) {
    watching_high = false;
  }
  if ( alarm_low == 0 ) {
    watching_low = false;
  }

  // now check the valid values
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

void check_thermometers() {
  long now = millis();
  
  // check temp every 10 seconds
  if (now - lastReading > 10000) {
    
    lastReading = now;
    
    // request data from the slave (triggers the corresponding code's Wire.onRequest callback)
    Serial.println("Requesting register from I2C slave");
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

    // slaveRegister now contains a copy of the slave's register
    // which is an array of temperatures
    // byte 0 & 1 = the DS18B20 probe
    // byte 2 & 3 = future probe
    // byte 4 & 5 = future probe

    
    // debugging log to Serial
    for (bufpos = 0; bufpos < REGISTER_LENGTH; bufpos++) {
      Serial.print("0x");
      Serial.print(slaveRegister[bufpos], HEX);
      Serial.print(" ");
    }
    Serial.println();

    // the first probe's temperature is the first two bytes
    // here we are shifting the first byte by 8 bits and adding the lower byte
    unsigned int newTemp = slaveRegister[0] << 8 | slaveRegister[1];
    Serial.print("New temp received: ");
    Serial.println(newTemp);
  
    // send temp to shadow if changed
    boolean needUpdate = false;
    if (newTemp != currentTemp) {
      currentTemp = newTemp;
      needUpdate = true;
    }
    
    if ( shouldAlarm() ) {
      in_alarm_state = true;
      needUpdate = true;
    } else if (in_alarm_state) {
      // immediately turn off peripheral alarm when transitioning from on to off
      pixel_alarm(false);
    }

    // if something has changed, update the shadow and display
    if ( needUpdate ) {
      shadow_update = true;
      display_update = true;
    }
    Serial.print("heap size: ");
    Serial.println(ESP.getFreeHeap());
  }
}


