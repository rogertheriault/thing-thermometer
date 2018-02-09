
/**
 * all the display settings (except pin assignments?)
 */
 
#include <SPI.h>
#include <GxEPD.h>
#include <GxGDEW0154Z04/GxGDEW0154Z04.cpp>  // 1.54" b/w/r
// free fonts from Adafruit_GFX
#include <Fonts/FreeSans9pt7b.h>
#include <Fonts/FreeSansBold12pt7b.h>
#include <Fonts/FreeSansBold18pt7b.h>

#include <GxIO/GxIO_SPI/GxIO_SPI.cpp>
#include <GxIO/GxIO.cpp>

GxIO_Class io(SPI, SS, EPAPER_DC, EPAPER_RST);
GxEPD_Class display(io);


// update the e-paper display
void updateDisplay() {
  if ( ! display_update) {
    return;
  }
  display_update = false;
  
  display.fillScreen(GxEPD_WHITE);
  display.setTextColor(GxEPD_BLACK);

  Serial.print(F("DISPLAY heap size: "));
  Serial.println(ESP.getFreeHeap());
  /*
  // test
  // get a bitmap and size from a SPIFFS file
  uint16_t bmW, bmH;
  uint8_t *myBitmap = getBitmapFromFS("/yogurt.bin", bmW, bmH);
  display.drawBitmap(myBitmap, 0, 30, bmW, bmH, GxEPD_BLACK);
  Serial.print(F("DRAW heap size: \n"));
  Serial.println(ESP.getFreeHeap());
  display.update();
  Serial.print(F("UPDATE heap size: "));
  Serial.println(ESP.getFreeHeap());
  //delete [] myBitmap;
  Serial.print(F("DELETE heap size: "));
  Serial.println(ESP.getFreeHeap());
  return;
   */
   
  // what we're cooking
  display.setFont(&FreeSansBold12pt7b);
  display.setCursor(0, 20);
  // TODO center
  display.print( recipe_title );

  // current temperature & step
  display.setFont(&FreeSans9pt7b);
  display.setCursor(0, 40);


  display.print( recipe_step_text );

  display.setCursor(0, 114);
  display.print(F("CURRENT"));
  display.setCursor(10, 128);
  display.print(F("TEMP"));
  display.setCursor(0, 32);

  // if the alarm's on, temp should be red
  if (in_alarm_state) {
    display.setTextColor(GxEPD_RED);
  }
  if (currentTemp > -100) {
    display.setFont(&FreeSansBold18pt7b);
    display.setCursor(100, 130);
    display.print(currentTemp);
    display.println(F("C"));
  } else {
    display.setTextColor(GxEPD_RED);
    display.setCursor(100, 130);
    // TODO icon here
    display.println(F("probe!"));
  }

  // if alarm is enabled show target temp(s)
  if (watching_high || watching_low) {
    display.setFont(&FreeSansBold12pt7b);
    display.setTextColor(GxEPD_RED);
    display.setCursor(0, 90);
    display.print(F("TARGET: "));
    if (watching_low) {
      display.print(alarm_low);
    }
    if (watching_low && watching_high) {
      display.print(F("-"));
    }
    if (watching_high) {
      display.print(alarm_high);
    }
    display.print(F("C"));
  }

  display.update();
}
