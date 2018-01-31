

#include "FastLED.h"
CRGB leds[1];

void setup_pixel() {
  LEDS.addLeds<WS2811, 12, RGB>(leds, 1);
  leds[0] = CRGB::Yellow;
  FastLED.show();
  delay(30);
}

void pixel_off() {
  leds[0] = CRGB::Black;
  FastLED.show();
  delay(30);
}

void pixel_wifidone() {
  leds[0] = CRGB::Blue;
  FastLED.show();
  delay(30);
}

void pixel_iotdone() {
  leds[0] = CRGB::Green;
  FastLED.show();
  delay(300);
}

void pixel_alarm() {
  leds[0] = CRGB::Red;
  FastLED.show();
  delay(30);
}


