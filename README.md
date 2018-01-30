# thing-thermometer

## An AWS IoT "Thing" Kitchen / BBQ Thermometer

This project includes Arduino ESP8266 code for the device,
and AWS Lambda code for the Alexa Skill

A temperature probe with a display can be controlled through Alexa:
- choose a food to cook and the high or low alarm temperatures are set automatically
- the probe displays the current step, the temperature and set point, and alerts the user when the heat should be adjusted or shut off
- Alexa can supply the current status at any time

![network diagram](img/iot-diagram.png?raw=true "IoT Diagram")

![photo of e-paper display](img/display.jpg?raw=true "Thermometer probe display")

![Fritzing breadboard diagram](img/fritzing-nodisplay.jpg?raw=true "Breadboard (without e-paper)")

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
- ESP8266 device, such as Adafruit HUZZAH (if using a display) or Wemos D1 Mini (fewer pins, suitable for display-less projects)
- USB programming cable (usually, a micro-usb DATA cable)
- (optional) E-paper display, I am using a 1.54" red-black-white display, you can also use oLED, or omit the display
- DS18B20 thermometer probe (for immersion)
- thermocouple probe (for bbq & oven)
- piezo buzzer (to alert someone)
- 4.7K ohm resistor
- pushbutton (user can silence alarm)
- neopixel (status and to alert someone)
- breadboard and wires
### Wiring it up
- 
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
