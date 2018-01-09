# aws-esp8266-thermometer

## An AWS IoT Kitchen / BBQ Thermometer

This project includes Arduino ESP8266 code for the device,
and AWS Lambda code for the Alexa Skill

## Requirements
### AWS
- an AWS account
- an Amazon Developer account
### Hardware Components
- ESP8266 device, such as Adafruit HUZZAH
- USB programming cable (usually, a micro-usb DATA cable)
- (optional) E-paper display, I am using a 1.54" red-black-white display
- DS18B20 thermometer probe
- one NeoPixel LED, I used a 5mm diffused LED
- piezo buzzer
- 4.7K ohm resistor
- pushbutton
- breadboard and wires
### Arduino Libraries
- The Arduino IDE (latest version)
- Adafruit GFX
- Adafruit Sensors
- Dallas Temperature library (if using DS18B20)
- GxEPD e-paper (if using a display)
- AWS Websocket MQTT Client (and the libraries it requires)
- ArduinoJSON
- ESP8266 hardware libraries

## Setting Up AWS
### Create a Thing
### Create a lambda Role
### Create the lambda function
## Setting Up Amazon Alexa
### Create a new Skill
## Build the Thermometer
### Set up Arduino IDE
### Load and build the code
### Flash the device
### Test
