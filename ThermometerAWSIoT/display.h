
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

GxIO_Class io(SPI, SS, 0, 2); // D3(=0), D4(=2)
GxEPD_Class display(io); // D4(=2), D2(=4)

// update the e-paper display
void updateDisplay() {
  
  display.fillScreen(GxEPD_WHITE);
  display.setTextColor(GxEPD_BLACK);
 
  // what we're cooking
  display.setFont(&FreeSansBold12pt7b);
  display.setCursor(0, 20);
  // TODO center
  display.print( cooking_mode );

  // current temperature & step
  display.setFont(&FreeSans9pt7b);
  display.setCursor(0, 40);
  // TODO get the message from a recipe structure
  switch (recipe_step) {
    case 1:
      display.print("Step 1: heating milk");
      break;
    case 2:
      display.print("Step 2: cooling milk");
      break;
    case 3:
      display.print("Done. Pitch culture");
      break;
    case 0:
    default:
      display.print("measuring");
      break;
  }
  display.setCursor(0, 114);
  display.print("CURRENT");
  display.setCursor(10, 128);
  display.print("TEMP");
  display.setCursor(0, 32);

  // if the alarm's on, temp should be red
  if (in_alarm_state) {
    display.setTextColor(GxEPD_RED);
  }
  display.setFont(&FreeSansBold18pt7b);
  display.setCursor(100, 130);
  display.print(currentTemp);
  display.println("C");

  // if alarm is enabled show target temp(s)
  if (watching_high || watching_low) {
    display.setFont(&FreeSansBold12pt7b);
    display.setTextColor(GxEPD_RED);
    display.setCursor(0, 90);
    display.print("TARGET: ");
    if (watching_low) {
      display.print(alarm_low);
    }
    if (watching_low && watching_high) {
      display.print("-");
    }
    if (watching_high) {
      display.print(alarm_high);
    }
    display.print("C");
  }

  display.update();
}
