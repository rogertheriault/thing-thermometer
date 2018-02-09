

#include "FastLED.h"
CRGB leds[1];

void setup_pixel() {
  LEDS.addLeds<WS2811, NEOPIXELPIN, RGB>(leds, 1);
  leds[0] = CRGB( 76, 91, 32); // amber
  FastLED.show();
}

void pixel_off() {
  leds[0] = CRGB::Black;
  FastLED.show();
}

void pixel_wifidone() {
  leds[0] = CRGB::Blue;
  FastLED.show();
}

void pixel_iotdone() {
  leds[0] = CRGB::Green;
  FastLED.show();
  delay(300);
}

void pixel_alarm() {
  leds[0] = CRGB::Red;
  FastLED.show();
}

// piezo buzzer
#if defined(ARDUINO_ARCH_ESP32)
void analogWrite(unsigned int pin, unsigned int value) {
  ledcWrite(pin, value);
}
#endif

void beep(unsigned char delayms) {
  analogWrite(ALARMPIN, 96); // best sound from tmb12a05 piezo at 3.3v
  delay(delayms);
  analogWrite(ALARMPIN, LOW);
  delay(delayms);
}

void setup_alarm() {
  #if defined(ARDUINO_ARCH_ESP32)
      ledcAttachPin(ALARMPIN, 0);
      ledcSetup(0, 5000, 8);
  #else
      pinMode(ALARMPIN, OUTPUT);
  #endif
  
  digitalWrite(ALARMPIN,LOW);
  beep(50);
  beep(50);
  beep(50);
  in_alarm_state = false;
}


void check_alarm() {
  if (in_alarm_state) {
    beep(400);
    pixel_alarm();
  } else {
    pixel_off();
  }
}


// check the sensor state
// returns true if one of our limits was exceeded, false otherwise
boolean shouldAlarm( ) {
  if ( alarm_high == 0 ) {
    watching_high = false;
  }
  if ( alarm_low == 0 ) {
    watching_low = false;
  }
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


