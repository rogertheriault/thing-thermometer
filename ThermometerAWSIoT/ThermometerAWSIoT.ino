// An AWS IoT ESP8266 "kitchen/bbq" thermometer
//
// See https://github.com/rogertheriault/thing-thermometer
//
// Copyright (c) 2018 Roger Theriault
// Licensed under the MIT license.

// copy config-sample.h to config.h and edit it
#include "config.h"

#include <ESP8266WiFiMulti.h>
#include <AWSWebSocketClient.h>
#include <DallasTemperature.h>
#include <ArduinoOTA.h>
#include <ESP8266WiFi.h>
#include <ArduinoJson.h>

//MQTT PAHO
#include <SPI.h>
#include <IPStack.h>
#include <Countdown.h>
#include <MQTTClient.h>

// e-paper
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

// define your THING_ID in config.h
#define AWS_TOPIC_UPDATE(x) "$aws/things/"x"/shadow/update"
#define AWS_TOPIC_SUBSCRIBE(x) "$aws/things/"x"/shadow/update/delta"

// one-wire DS18B20 settings
#define OWBUS D1 // D1
OneWire oneWire(OWBUS);
DallasTemperature sensors(&oneWire);
DeviceAddress tempsens;

#define ALARMPIN D6


//MQTT config
const int maxMQTTpackageSize = 512;
const int maxMQTTMessageHandlers = 1;

ESP8266WiFiMulti WiFiMulti;

AWSWebSocketClient awsWSclient(1000);

IPStack ipstack(awsWSclient);
MQTT::Client<IPStack, Countdown, maxMQTTpackageSize, maxMQTTMessageHandlers> *client = NULL;

//# of connections
long connection = 0;

/*     MAIN  */

// bunch of globals for our state
String cooking_mode = "-- Ready --";
boolean in_alarm_state = false;
int currentTemp = -127; // initializing to -127 should trigger an update on reset
boolean watching_high = false;
boolean watching_low = false;
int alarm_high = 0;
int alarm_low = 0;

void setup() {
  Serial.begin(115200);
  display.init();
  display.setRotation(1);
  delay(2000);
  Serial.setDebugOutput(1);
  setup_wifi();
 
  setup_OTA();
  setup_sensors();

  setup_alarm();
 

  // AWS parameters    
  awsWSclient.setAWSRegion(AWS_REGION);
  awsWSclient.setAWSDomain(AWS_ENDPOINT);
  awsWSclient.setAWSKeyID(AWS_KEY_ID);
  awsWSclient.setAWSSecretKey(AWS_KEY_SECRET);
  awsWSclient.setUseSSL(true);

  if (connect()) {
    subscribe();
    //updateShadow();
  }
}

long lastReading = 0;

void loop() {
  // remove if there's no need to OTA update
  ArduinoOTA.handle();

  check_alarm();
  
  long now = millis();
  // check temp every 10 seconds - TODO use watchdog timer
  if (now - lastReading > 10000) {
    lastReading = now;
    sensors.requestTemperatures();
    float tempC = sensors.getTempC(tempsens);
    int temp = (int)tempC;

    // send temp to shadow if changed
    boolean needUpdate = false;
    if (temp != currentTemp) {
      currentTemp = temp;
      needUpdate = true;
    }
    
    if ( shouldAlarm() ) {
      in_alarm_state = true;
      needUpdate = true;
    }
    
    if ( needUpdate ) {
      updateShadow();
      updateDisplay();
    }
  }


  // keep mqtt up and running
  if (awsWSclient.connected()) {    
    client->yield();
  } else {
    // handle reconnection
    if (connect()) {
      subscribe();      
    }
  }

}


// piezo buzzer
void setup_alarm() {
  pinMode(ALARMPIN, OUTPUT);
  beep(50);
  beep(50);
  beep(50);
  in_alarm_state = false;
}
void check_alarm() {
  if (in_alarm_state) {
    beep(400);
  }
}

void beep(unsigned char delayms) {
  analogWrite(ALARMPIN, 96); // best sound from tmb12a05 piezo at 3.3v
  delay(delayms);
  analogWrite(ALARMPIN, 0);
  delay(delayms);
}

void setup_sensors(){
  sensors.begin();
  //Serial.println(sensors.getDeviceCount(), DEC);
  if (!sensors.getAddress(tempsens, 0)) {
    Serial.println("Unable to find address for Device 0");
  }
  sensors.setResolution(tempsens, 9);
}

void setup_wifi() {
    WiFiMulti.addAP(WLAN_SSID, WLAN_PASS);
    Serial.println ("connecting to wifi");
    while(WiFiMulti.run() != WL_CONNECTED) {
        delay(100);
        Serial.print (".");
    }
    Serial.println ("\nconnected");
}

void setup_OTA() {
  delay(10);
  //ArduinoOTA.setHostname("Lounge");

  ArduinoOTA.onStart([]() {
    Serial.println("Start");
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("\nEnd");
  });
 ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
});
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
    else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
    else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
    else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
    else if (error == OTA_END_ERROR) Serial.println("End Failed");
  });
  ArduinoOTA.begin();
   
}

// check the sensor state
// returns true if one of our limits was exceeded, false otherwise
boolean shouldAlarm( ) {
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

// update the e-paper display
void updateDisplay() {
  
  display.fillScreen(GxEPD_WHITE);
  display.setTextColor(GxEPD_BLACK);
 
  // what we're cooking
  display.setFont(&FreeSansBold12pt7b);
  display.setCursor(0, 20);
  // TODO center
  display.print( cooking_mode );

  // current temperature
  display.setFont(&FreeSans9pt7b);
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

// count messages arrived
int arrivedcount = 0;

// callback to handle thing shadow delta messages
// these are settings that we must change to reflect the desired state

void messageArrived(MQTT::MessageData& md) {
  MQTT::Message &message = md.message;

  Serial.print("Message ");
  Serial.print(++arrivedcount);
  Serial.print(" arrived: qos ");
  Serial.print(message.qos);
  Serial.print(", retained ");
  Serial.print(message.retained);
  Serial.print(", dup ");
  Serial.print(message.dup);
  Serial.print(", packetid ");
  Serial.println(message.id);
  Serial.print("Payload ");
  char* msg = new char[message.payloadlen+1]();
  memcpy (msg,message.payload,message.payloadlen);
  Serial.println(msg);

  // convert the JSON text to an object
  // the delta Shadow object will look like this:
  // {"version":192,
  //  "timestamp":1515548804,
  //  "state":{
  //    "alarm_high":88,
  //    "mode":"Yogurt"},
  // "metadata":{"alarm_high":{"timestamp":1515548804},"mode":{"timestamp":1515548804}}}
  //
  // assume all the state is a REQUESTED state
  // assume any missing items might be things we need to unset?

  const size_t capacity = JSON_OBJECT_SIZE(3) + JSON_ARRAY_SIZE(2) + 60;
  DynamicJsonBuffer jsonBuffer(capacity);
  
  JsonObject& response = jsonBuffer.parseObject(msg);
  if (response.success() && response["state"]) {
    Serial.println("parsed Json");
    
    // Extract values
    Serial.println(F("Response:"));
    int new_high = response["state"]["alarm_high"].as<int>();
    int new_low = response["state"]["alarm_low"].as<int>();
    const char *new_mode = response["state"]["mode"].as<char*>();
    Serial.println(new_high);
    Serial.println(new_low);
    Serial.println(new_mode);


    // update any items in the state
    if ( new_high ) {
      Serial.print("Desired high: ");
      Serial.println(new_high);
      watching_high = true;
      alarm_high = new_high;
      // disable current alarm if new temp is below the threshold
      if ( in_alarm_state && currentTemp < new_high ) {
        in_alarm_state = false;
      }
    }

    if ( new_low ) {
      Serial.print("Desired low: ");
      Serial.println(new_low);
      watching_low = true;
      alarm_low = new_low;
      // disable current alarm if new temp is below the threshold
      if ( in_alarm_state && currentTemp > new_low ) {
        in_alarm_state = false;
      }
    }

    if ( new_mode ) {
      Serial.println("Cooking mode: ");
      Serial.println(new_mode);
      cooking_mode = new_mode;
    }

    // TODO send an update to the shadow API
    updateShadow();

    // update the display
    updateDisplay();
  }
  delete msg;
}

// connects to websocket layer and mqtt layer
bool connect() {

  if (client == NULL) {
    client = new MQTT::Client<IPStack, Countdown, maxMQTTpackageSize, maxMQTTMessageHandlers>(ipstack);
  } else {
    if (client->isConnected ()) {    
      client->disconnect ();
    }  
    delete client;
    client = new MQTT::Client<IPStack, Countdown, maxMQTTpackageSize, maxMQTTMessageHandlers>(ipstack);
  }


  // delay is not necessary... it just help us to get a "trustful" heap space value
  delay(1000);
  Serial.print(millis());
  Serial.print(" - conn: ");
  Serial.print(++connection);
  Serial.print(" - (");
  Serial.print(ESP.getFreeHeap());
  Serial.println(")");

  int rc = ipstack.connect(AWS_ENDPOINT, AWS_SERVERPORT);
  if (rc != 1) {
    Serial.println("error connection to the websocket server");
    return false;
  } else {
    Serial.println("websocket layer connected");
  }

  Serial.println("MQTT connecting");
  MQTTPacket_connectData data = MQTTPacket_connectData_initializer;
  data.MQTTVersion = 3;
  char* clientID = generateClientID();
  data.clientID.cstring = clientID;
  rc = client->connect(data);
  delete[] clientID;
  
  if (rc != 0) {
    Serial.print("error connection to MQTT server");
    Serial.println(rc);
    return false;
  }
  Serial.println("MQTT connected");
  return true;
}

 // generate random mqtt clientID
char* generateClientID () {
  char* cID = new char[23]();
  for (int i=0; i<22; i+=1) {
    cID[i]=(char)random(1, 256);
  }
  return cID;
}

// subscribe to the Thing Shadow topic
void subscribe() {
    Serial.println( AWS_TOPIC_SUBSCRIBE(THING_ID) );
    // subscribe, supplying messageArrived as the callback for new messages
    int rc = client->subscribe(AWS_TOPIC_SUBSCRIBE(THING_ID), MQTT::QOS0, messageArrived);
    if (rc != 0) {
      Serial.print("rc from MQTT subscribe is ");
      Serial.println(rc);
      return;
    }
    Serial.println("MQTT subscribed");
}

// send an update to the Thing shadow
void updateShadow() {
    Serial.println("updating device shadow");
    Serial.println( AWS_TOPIC_UPDATE(THING_ID) );
    
    MQTT::Message message;
    
    Serial.println( currentTemp );

    StaticJsonBuffer<500> jsonBuffer;
    JsonObject& payload = jsonBuffer.createObject();
    JsonObject& state = payload.createNestedObject("state");
    JsonObject& reported = state.createNestedObject("reported");
    
    reported["temperature"] = currentTemp;
    reported["mode"] = cooking_mode;
    if ( alarm_high ) {
      reported["alarm_high"] = alarm_high;
    }
    if ( alarm_low ) {
      reported["alarm_low"] = alarm_low;
    }
    payload.printTo(Serial);
    Serial.println();

    char buf[501];
    payload.printTo(buf, 500);
    message.qos = MQTT::QOS0;
    message.retained = false;
    message.dup = false;
    message.payload = (void*)buf;
    message.payloadlen = strlen(buf)+1;
    int rc = client->publish(AWS_TOPIC_UPDATE(THING_ID), message); 

}
