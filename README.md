# thing-thermometer

## An AWS IoT "Thing" Kitchen / BBQ Thermometer

![photo of e-paper display](img/prototype.jpg?raw=true "Thermometer probe display")

This project includes Arduino code for the thermometer device,
and AWS Lambda code for the Alexa Skill

You'll find [directions on getting everything set up at Hackster.io](https://www.hackster.io/rogertheriault/alexa-bbq-kitchen-thermometer-with-iot-arduino-and-e-paper-63c64f)

A temperature probe with a display can be controlled through Alexa:
- choose a food to cook and the high or low alarm temperatures are set automatically
- the probe displays the current step, the temperature and set point, and alerts the user when the heat should be adjusted or shut off
- Alexa can supply the current status at any time

![network diagram](img/iot-diagram.png?raw=true "IoT Diagram")

### How it works in a nutshell

The thermometer has one or more thermometer probes, for measuring liquid and 
food and oven temperatures. It also has an audio annunciator (buzzer) and a 
visual one (neopixel LED), and a button to shut off the alarms.

Most importantly, the microcontroller in the device can connect to WiFi, and has 
the key and secret for a very limited IoT user. This allows the device to connect 
to MQTT topics in the AWS cloud and report its state (temperature, set points, 
current step) and react to the Alexa skill's desired state (controlling the 
thermometer).

The Alexa skill is a custom skill that permits a user to request that a recipe 
be started, or to control the stage of the recipe
The skill requires the user to link their account to be able to identify the 
devices the user owns. In this code, we've hardcoded some of those settings.

The skills' Lambda code can look up a user id in a DynamoDB table and obtain 
the user's Thing ID. It also can look up the user's state in another table, so 
that Alexa knows a recipe is in progresss when the user starts talking to 
Alexa after the recipe's been underway for a while.

The AWS Cloud has a shadow state for each device, showing what the current 
and desired states of the Thing (device, in our case the thermometer) are. 
The Alexa Lambda code can access this or change the desired device state in 
response to user interactions, depending on the User state. The MQTT topics 
allow the device to constantly be aware of changes without Alexa code 
needing to locate and communicate directly with the device.

A JSON file contains all the recipes the Alexa Skill can handle, with steps, 
and thermometer setpoints and dialog info for each step. A summary version of 
this is in a public S3 bucket and available to the device by requesting a URL. 
The device checks this URL and compares the ETAG with a copy of the file stored 
in the device's filesytem area of FLASH (called SPIFFS). It checks and updates 
the file if necessary only when the device powers on, but has access to the 
data when new desired state commands arrive from the MQTT topic.

The device regularly reads the probe temperature(s), looks for changes, and 
then updates its display and the Thing Shadow. It also monitors the shadow's 
Delta topic, reacting to new commands - start a recipe or go to the next 
recipe step. It uses the JSON data in the recipes file to know which 
temperature setpoints to set and what text to display.

The device is actually composed of two separately programmed microcontrollers:

* an Adafruit HUZZAH ESP8266 handles the WiFi, AWS connection, recipe logic, and
  the e-paper display
* an Arduino Uno (you can use a smaller device, this is what I had on hand) is an
  I2C slave controlled by the ESP8266, and handles thermometer readings, beeping,
  flashing, and such peripheral things.

Why two controllers? This is not an uncommon pattern for embedded design, but
because we chose an ESP8266 to handle the WiFi, we wind up with few available ports
for the thermometers. We'll need a couple of analog ports and few
pins to handle the buzzer, buttons, and multiple thermometers.

The Arduino Uno supports Slave Mode on the I2C bus. I'd originally targeted the low
power and powerful Arduino MKRZERO, but it turns out the I2C libraries for the SAMD
processor haven't yet implemented I2C slave mode.

![Fritzing breadboard diagram](img/fritzing-breadboard.png?raw=true "Breadboard (without e-paper)")

### Sample Utterances
*"Alexa, ask my thermometer to make Yogurt"*

*"Alexa, ask my thermometer for the status"*

### Voice UI Diagrams
![diagram of the voice ui](img/basic-recipe-flow.png?raw=true "Basic Recipe Flow")

![diagram of the voice ui](img/unknown-recipe-flow.png?raw=true "Unknown Recipe Flow")

## Requirements
### AWS
- an AWS account
- an Amazon Developer account
### Hardware Components
- ESP8266 device, such as Adafruit HUZZAH or Wemos D1 Mini
- Arduino Uno (or another Arduino that supports I2C slave mode)
- USB programming cable (usually, a micro-usb DATA cable)
- E-paper display, I am using a 1.54" red-black-white display, you can also use oLED, or omit the display
- DS18B20 thermometer probe (for immersion) and 4.7K ohm resistor
- thermocouple probe (for bbq & oven) (future, not implemented here) and resistors
- piezo buzzer (to alert someone) and 100 ohm resistor
- momentary (Normally Open) pushbutton (user can silence alarm) and 10k ohm resistor
- neopixel (status and to alert someone) (I used an 8-pixel ring)
- breadboard and wires

### Wiring it up

see https://cdn-learn.adafruit.com/assets/assets/000/030/930/original/adafruit_products_2889_pinout_v1_0.png?1457306365

#### ESP 8266
Peripheral | PIN | ESP8266 Pin
--- | --- | --- 
Waveshare 1.54" e-paper | 3.3v | 3.3v
" | GND | GND
" | DIN | GPIO13
" | CLK | GPIO14
" | CS | GPIO15
" | DC | GPIO0
" | RST | GPIO2
" | BUSY | GPIO12
Uno I2C | SDA | SDA (GPIO4)
" | SCK | SCK (GPIO5)
" | GND | GND

#### Arduino Uno
Peripheral | PIN | Arduino Pin
--- | --- | --- 
NeoPixel Ring | VIN | VCC (+5V)
" | GND | GND
" | Data In | Digital pin 6
Piezo Buzzer | + | Through 100 ohm resistor to digital pin 9
" | GND | GND
Momentary Push Button | A | through 10k ohm resistor to GND
" | A | directly to digital pin 12
" | B (the other leg) | VCC (+5V)
DS18B20 | black | GND
" | red | VCC (+5V)
" | yellow | directly to digital pin 10
4.7K resistor | needed for DS18B20 | between pin 10 and VCC

### Arduino Libraries
- The Arduino IDE (latest version)
- Adafruit GFX
- Adafruit Sensors
- Dallas Temperature library
- GxEPD e-paper (if using a display)
- AWS Websocket MQTT Client (and the libraries it requires)
- ArduinoJSON
- ESP8266 hardware libraries

See [Hackster.io](https://www.hackster.io/rogertheriault/alexa-bbq-kitchen-thermometer-with-iot-arduino-and-e-paper-63c64f) for a build walkthrough