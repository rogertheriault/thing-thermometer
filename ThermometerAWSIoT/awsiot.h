
// define your THING_ID in config.h (or, get from EEPROM)

// these macros are used to set up the topic strings, THING_ID will be inserted in place of x
//#define AWS_TOPIC_UPDATE(x) "$aws/things/"x"/shadow/update"
//#define AWS_TOPIC_SUBSCRIBE(x) "$aws/things/"x"/shadow/update/delta"

// declarations
//void messageArrived(char*, unsigned char*, unsigned int);
void messageArrived(char* topic, byte* payload, unsigned int length);

#include <PubSubClient.h>
#include <ArduinoJson.h>

// MQTT config for PubSubClient
#define MQTT_MAX_PACKET_SIZE 512

#ifdef ESP8266
ESP8266WiFiMulti WiFiMulti;
#else
//WiFiMulti WiFiMulti;
#endif


//# of connections
long connection = 0;

// SSL client
WiFiClientSecure espClient;
// MQTT client with port 8883 (secure)
PubSubClient mqttClient(AWS_ENDPOINT, 8883, messageArrived, espClient);

void setup_awsiot() {
  // AWS parameters should be set in config.h  

  // Load certificate file
  const char * cert = getCharsFromFS("/certificate.pem.crt");
  if (!cert) {
    Serial.println(F("Failed to open cert file"));
  } else {
    Serial.println(F("Success to open cert file"));
  }
  delay(1000);

  espClient.setCertificate(cert);
  Serial.println(F("cert set"));

  // Load private key file
  const char * private_key = getCharsFromFS("/private.pem.key");
  if (!private_key) {
    Serial.println(F("Failed to open private cert file"));
  } else {
    Serial.println(F("Success to open private cert file"));
  }

  delay(1000);

  espClient.setPrivateKey(private_key);
  Serial.println(F("private key set"));
  
  
  // Load CA file
  const char * ca = getCharsFromFS("/aws-root-ca.pem");
  if (!ca) {
    Serial.println(F("Failed to open ca "));
  } else {
    Serial.println(F("Success to open ca"));
  }

  delay(1000);

  espClient.setCACert(ca);
  Serial.println(F("ca loaded"));
    
  
  Serial.print("Heap: ");
  Serial.println(ESP.getFreeHeap());

}

void mqtt_reconnect() {
  while (!mqttClient.connected()) {
    Serial.println(F("Attempting to connect to AWS IoT on MQTT"));

    if (mqttClient.connect(THING_ID)) {
      Serial.println(F("connected"));
      mqttClient.subscribe(aws_topic_delta);
    } else {
      Serial.print(F("failed. rc="));
      Serial.print(mqttClient.state());
      Serial.println(F(" waiting 5 secs"));
      // TODO yield
      delay(5000);
    }
  }
}

// blocks until connected
void setup_wifi() {
    delay(10);
    
    //WiFiMulti.addAP(WLAN_SSID, WLAN_PASS);
    Serial.print(F("connecting to wifi: "));
    Serial.println(WLAN_SSID);
    
    WiFi.begin(WLAN_SSID, WLAN_PASS);

    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
        // TODO timeout and reset
    }

    Serial.println(F("\nWiFi connected"));
    Serial.print(F("IP address: "));
    Serial.println(WiFi.localIP());

    pixel_wifidone();
}

/** 
 * send an update request to the Thing shadow
 * 
 * This sends desired state rather than reported state... it happens when 
 * a user interactes with the device or the recipe ends and times out
 * 
 * TODO add params
 */
void doUpdateDesired() {

    Serial.println(F("DESIRED device shadow"));
    Serial.println( aws_topic_update );
    
    //{"state":{"desired":{"mode":"measure","alarm_high":null,"alarm_low":null,"step":0}}}

    char buf[] = "{\"state\":{\"desired\":{\"mode\":\"measure\",\"alarm_high\":null,\"alarm_low\":null,\"step\":0}}}";
    Serial.println(buf);
    int rc = mqttClient.publish(aws_topic_update, buf); 
    Serial.print(F("returned "));
    Serial.println(rc);
}

/** 
 * mode timer logic, purpose is to give the user 60(or more?) seconds after the last 
 * step of a recipe
 * 
 */
// TODO move to a Helpers file?
boolean modeTimerSet = false;
unsigned long modeTimerExpires;

void endRecipeMode(int seconds) {
  modeTimerSet = true;
  modeTimerExpires = millis() + seconds * 1000;
  // todo set what should be displayed
}

void checkModeTimer() {
  if (! modeTimerSet) {
    return;
  }
  Serial.print(F("timer tick: "));
  Serial.println((modeTimerExpires - millis()) / 1000);
  if (millis() >= modeTimerExpires) {
    modeTimerSet = false;
    // TODO run a callback
    doUpdateDesired();
  }
}


/**
 * Update the state to a new recipe / step
 * 
 * This modifies our state, based on the pre-sets in the recipes.json file
 * Much of the Received Update logic may move here
 * 
 * @param String recipeid id of the recipe
 * @paran String step step number from 1 to N
 */
void setRecipeStep(String recipeid, String step) {

  // special case, skip the recipe list
  if ( recipeid == "measure" ) {
    cooking_mode = "measure";
    recipe_step = 0;
    in_alarm_state = false;

    recipe_step_text = default_step_text;
    recipe_title = default_title;
    // TODO go to sleep after screen updates?
    return;
  }
    
  // Set current recipe info into our "state"
  // TODO refactor
  File file = SPIFFS.open("/recipes.json", "r");
  if ( !file ) {
    Serial.println(F("file open failed"));
  }
  char* filedata = new char[file.size()+1]();
  file.readBytes(filedata, file.size());
  //Serial.println(filedata);
  file.close();

  const size_t bufferSize = JSON_OBJECT_SIZE(1) + 6*JSON_OBJECT_SIZE(2) + JSON_OBJECT_SIZE(3) + JSON_OBJECT_SIZE(4) + 290;
  DynamicJsonBuffer jsonBuffer(bufferSize);
  
  JsonObject& recipes = jsonBuffer.parseObject(filedata);
  if (recipes.success() && recipes["recipes"]) {
    Serial.println(F("parsed Json"));
    
    // Extract values
    Serial.print(F("Current recipe: "));
    Serial.println(recipeid);
    if (! recipes["recipes"][recipeid] ) {
      Serial.println(F("recipeid not found"));
      return;
    }
    JsonObject& recipe = recipes["recipes"][recipeid];
    const char *title = recipe["title"];
    if (! title) {
      Serial.println(F("current recipe not found"));
      return;
    }
    Serial.println(title);
    String stepid = String("step") + step;
    Serial.println(stepid);
    JsonObject& recipe_step = recipe[stepid];
    int new_high = recipe_step["alarm_high"];
    int new_low = recipe_step["alarm_low"];
    bool isComplete = recipe_step["complete"];
    const char *steptext = recipe_step["text"];
    Serial.println(new_high);
    Serial.println(new_low);
    Serial.println(isComplete);
    Serial.println(steptext);
    alarm_high = new_high;
    alarm_low = new_low;
    recipe_step_text = steptext;
    recipe_title = title;

    // check alarm state

    // check recipe completion
    if ( isComplete ) {
      // set a timer for 1 minute, then go out of recipe mode
      endRecipeMode(60);
    }
  }
}

// generate random mqtt clientID
char* generateClientID() {
  char* cID = new char[23]();
  for (int i=0; i<22; i+=1) {
    cID[i]=(char)random(1, 256);
  }
  return cID;
}

// send an update to the Thing shadow
void updateShadow() {
    if ( !shadow_update ) {
      return;
    }
    shadow_update = false;
    Serial.println("updating device shadow");
    Serial.println( aws_topic_update );
    
    Serial.println( currentTemp );

    const size_t bufferSize = 2*JSON_OBJECT_SIZE(1) + 2*JSON_OBJECT_SIZE(2) + JSON_OBJECT_SIZE(4) + 140;
    DynamicJsonBuffer jsonBuffer(bufferSize);
    JsonObject& payload = jsonBuffer.createObject();
    JsonObject& state = payload.createNestedObject("state");
    JsonObject& reported = state.createNestedObject("reported");
    
    reported["temperature"] = currentTemp;
    reported["mode"] = cooking_mode;

    // report missing values as null
    reported["alarm_high"] = (alarm_high) ? alarm_high : 0;
    reported["alarm_low"] = (alarm_low) ? alarm_low : 0;
    reported["step"] = (recipe_step) ? recipe_step : 0;

    payload.printTo(Serial);
    Serial.println();

    char buf[201]; // TODO reduce more?
    payload.printTo(buf, 200);
    int rc = mqttClient.publish(aws_topic_update, buf); 
    //delete buf;
    Serial.print(F("return code: "));
    Serial.println(rc);
}

// count messages arrived
int arrivedcount = 0;

// callback to handle thing shadow delta messages
// these are settings that we must change to reflect the desired state

void messageArrived(char* topic, byte* payload, unsigned int length) {

  Serial.print(F("New MQTT message arrived on topic: "));
  Serial.println(topic);
  // TODO if we handle different topics do a switch here
  // assuming it's the delta topic we subscribed to (the only one) for now

  char* msg = new char[length + 1]();
  memcpy(msg, payload, length);
  Serial.println(msg);

  // convert the JSON text to an object
  // the delta Shadow object will look like this:
  // {"version":192,
  //  "timestamp":1515548804,
  //  "state":{
  //    "alarm_high":82,
  //    "step": 1,
  //    "mode":"Yogurt"},
  // "metadata":{"alarm_high":{"timestamp":1515548804},"mode":{"timestamp":1515548804}}}
  //
  // assume all the state is a REQUESTED state
  // assume any missing items might be things we need to unset?

  const size_t bufferSize = 2*JSON_OBJECT_SIZE(1) + 2*JSON_OBJECT_SIZE(2) + JSON_OBJECT_SIZE(4) + 140;
  DynamicJsonBuffer jsonBuffer(bufferSize);
  
  JsonObject& response = jsonBuffer.parseObject(msg);
  if (response.success() && response["state"]) {
    Serial.println("parsed Json");
    
    // Extract values
    Serial.println(F("Response:"));
    int new_high = response["state"]["alarm_high"];
    int new_low = response["state"]["alarm_low"];
    int new_step = response["state"]["step"];
    const char *new_mode = response["state"]["mode"];
    Serial.println(new_high);
    Serial.println(new_low);
    Serial.println(new_mode);

    // update any items in the state
    if ( new_high ) {
      Serial.print(F("Desired high: "));
      Serial.println(new_high);
      watching_high = true;
      alarm_high = new_high;
      // disable current alarm if new temp is below the threshold
      if ( in_alarm_state && currentTemp < new_high ) {
        in_alarm_state = false;
      }
    }

    if ( new_low ) {
      Serial.print(F("Desired low: "));
      Serial.println(new_low);
      watching_low = true;
      alarm_low = new_low;
      // disable current alarm if new temp is below the threshold
      if ( in_alarm_state && currentTemp > new_low ) {
        in_alarm_state = false;
      }
    }
 

    if ( new_mode ) {
      Serial.print(F("Recipe id: "));
      Serial.println(new_mode);
      cooking_mode = new_mode;
    }
    
    if ( new_step ) {
      Serial.print(F("Desired step: "));
      Serial.println(new_step);
      recipe_step = new_step;
    }

    
    setRecipeStep(cooking_mode, String(recipe_step));


    // TODO just set a flag, and let the loop do this
    // update the device shadow with the new state
    shadow_update = true;

    // update the display
    display_update = true;
  }
  //delete msg;
}

