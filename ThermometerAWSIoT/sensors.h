#include <DallasTemperature.h>

// one-wire DS18B20 settings
#define OWBUS 5 // D1(=5)
OneWire oneWire(OWBUS);
DallasTemperature sensors(&oneWire);
DeviceAddress tempsens;

/**
 * setup - called in setup()
 */
void setup_sensors() {
  // set up the button pin
  pinMode(BUTTONPIN, INPUT);
  
  sensors.begin();
  //Serial.println(sensors.getDeviceCount(), DEC);
  if ( !sensors.getAddress(tempsens, 0) ) {
    // TODO likely it's not plugged in, we should display a message, keep checking, and not report anything
    Serial.println(F("Unable to find address for Device 0"));
  }
  sensors.setResolution(tempsens, 9);
}

void check_button() {
  int newState = digitalRead(BUTTONPIN);
  if (newState != button_state) {
    Serial.print("button now:");
    Serial.println(newState);
    button_state = newState;
    
    // assume any detected press is meant to shut off that horrible noise
    if (in_alarm_state) {
      in_alarm_state = false;
      display_update = true;
    }
  }
}
