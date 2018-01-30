# thing-thermometer

## An AWS IoT "Thing" Kitchen / BBQ Thermometer

This project includes Arduino ESP8266 code for the device,
and AWS Lambda code for the Alexa Skill

A temperature probe with a display can be controlled through Alexa:
- choose a food to cook and the high or low alarm temperatures are set automatically
- the probe displays the current step, the temperature and set point, and alerts the user when the heat should be adjusted or shut off
- Alexa can supply the current status at any time

### NOTE: While this is currently functional, it's still incomplete and undergoing changes!

![network diagram](img/iot-diagram.png?raw=true "IoT Diagram")

### How it works in a nutshell

The thermometer has one or more thermometer probes, for measuring liquid and food and oven temperatures. It also has an audio annunciator (buzzer) and a visual one (neopixel LED), and a button to shut off the alarms.

Most importantly, the microcontroller in the device can connect to WiFi, and has provisioned certificates identifying the device, or, for the ESP8266, the key and secret for a very limited IoT user. This allows the device to connect to MQTT topics in the AWS cloud and report its state (temperature, set points, current step) and react to the Alexa skill's desired state (controlling the thermometer).

Alexa actually has two skills: a custom skill that permits a user to request that a recipe be started, or to control the stage of the recipe, and a basic Alexa Home Skill that permits a user to ask for the thermometer's temperature. Both require the user to link their account to be able to identify the devices the user owns. In this code, we've hardcoded some of those settings.

The skills' Lambda code can look up a user id in a DynamoDB table and obtain the user's Thing ID. It also can look up the user's state in another table, so that Alexa knows a recipe is in progresss when the user starts talking to Alexa after the recipe's been underway for a while.

The AWS Cloud has a shadow state for each device, showing what the current and desired states of the Thing (device, in our case the thermometer) are. The Alexa Lambda code can access this or change the desired device state in response to user interactions, depending on the User state. The MQTT topics allow the device to constantly be aware of changes without Alexa code needing to locate and communicate directly with the device.

A JSON file contains all the recipes the Alexa Skill can handle, with steps, and thermometer setpoints and dialog info for each step. A summary version of this is in a public S3 bucket and available to the device by requesting a URL. The device checks this URL and compares the ETAG with a copy of the file stored in the device's filesytem area of FLASH (called SPIFFS). It checks and updates the file if necessary only when the device powers on, but has access to the data when new desired state commands arrive from the MQTT topic.

The device regularly reads the probe temperature(s), looks for changes, and then updates its display and the Thing Shadow. It also monitors the shadow's Delta topic, reacting to new commands - start a recipe or go to the next recipe step. It uses the JSON data in the recipes file to know which temperature setpoints to set and what text to display.

(more to come)


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
