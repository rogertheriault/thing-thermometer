
// define your THING_ID in config.h (or, get from EEPROM)

// these macros are used to set up the topic strings, THING_ID will be inserted in place of x
#define AWS_TOPIC_UPDATE(x) "$aws/things/"x"/shadow/update"
#define AWS_TOPIC_SUBSCRIBE(x) "$aws/things/"x"/shadow/update/delta"

// This set of libraries has proven "reliable" for AWS IoT MQTT
// The other option is to use PubSubClient, which does support 
// certificates, but seems a bit unstable.
// A bit more experimentation is needed.

#include <AWSWebSocketClient.h>
// MQTT PAHO
#include <IPStack.h>
#include <Countdown.h>
#include <MQTTClient.h>
// JSON parsing library
#include <ArduinoJson.h>

// MQTT config
const int maxMQTTpackageSize = 512;
const int maxMQTTMessageHandlers = 1;
AWSWebSocketClient awsWSclient(1000);

ESP8266WiFiMulti WiFiMulti;

IPStack ipstack(awsWSclient);
MQTT::Client<IPStack, Countdown, maxMQTTpackageSize, maxMQTTMessageHandlers> *client = NULL;

// # of connections counter
long connection = 0;

// set up the AWS IoT (MQTT) connection
void setup_awsiot() {
  // AWS parameters should be set in config.h  
  awsWSclient.setAWSRegion(AWS_REGION);
  awsWSclient.setAWSDomain(AWS_ENDPOINT);

  // Note this should be replaced with a certificate and TLS1.2 when libraries support it (and we don't run out of RAM)
  awsWSclient.setAWSKeyID(AWS_KEY_ID);
  awsWSclient.setAWSSecretKey(AWS_KEY_SECRET);
  
  awsWSclient.setUseSSL(true);
}


// blocks until connected
// connect to the WiFi router
void setup_wifi() {
    WiFiMulti.addAP(WLAN_SSID, WLAN_PASS);
    Serial.println ("connecting to wifi");
    while(WiFiMulti.run() != WL_CONNECTED) {
        delay(100);
        Serial.print (".");
    }
    Serial.println ("\nconnected");

    // update the state of the pixels
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

    Serial.println("DESIRED device shadow");
    Serial.println( AWS_TOPIC_UPDATE(THING_ID) );
    
    MQTT::Message message;
    
    //{"state":{"desired":{"mode":"measure","alarm_high":null,"alarm_low":null,"step":0}}}

    char buf[] = "{\"state\":{\"desired\":{\"mode\":\"measure\",\"alarm_high\":null,\"alarm_low\":null,\"step\":0}}}";
    message.qos = MQTT::QOS0;
    message.retained = false;
    message.dup = false;
    message.payload = (void*)buf;
    message.payloadlen = strlen(buf)+1;
    int rc = client->publish(AWS_TOPIC_UPDATE(THING_ID), message); 
    Serial.println(buf);
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
  File file = SPIFFS.open("/recipes.json", "r");
  if ( !file ) {
    Serial.println("recipes.json file open failed");
  }
  char* filedata = new char[file.size()+1]();
  file.readBytes(filedata, file.size());
  file.close();

  const size_t bufferSize = JSON_OBJECT_SIZE(1) + 6*JSON_OBJECT_SIZE(2) + JSON_OBJECT_SIZE(3) + JSON_OBJECT_SIZE(4) + 290;
  DynamicJsonBuffer jsonBuffer(bufferSize);
  
  JsonObject& recipes = jsonBuffer.parseObject(filedata);
  if (recipes.success() && recipes["recipes"]) {
    Serial.println("parsed Json");
    
    // Extract values
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
    if (alarm_high > 0) {
      watching_high = true;
    }
    alarm_low = new_low;
    if (alarm_low > 0) {
      watching_low = true;
    }
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

// send an update to the Thing shadow
void updateShadow() {
    if ( !shadow_update ) {
      return;
    }
    shadow_update = false;
    Serial.println("updating device shadow");
    Serial.println( AWS_TOPIC_UPDATE(THING_ID) );
    
    MQTT::Message message;
    
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
    message.qos = MQTT::QOS0;
    message.retained = false;
    message.dup = false;
    message.payload = (void*)buf;
    message.payloadlen = strlen(buf)+1;
    int rc = client->publish(AWS_TOPIC_UPDATE(THING_ID), message); 
    //delete buf;
}

// count messages arrived
int arrivedcount = 0;

// callback to handle thing shadow delta messages
// these are settings that we must change to reflect the desired state

void messageArrived(MQTT::MessageData& md) {
  MQTT::Message &message = md.message;

  Serial.print(F("Message "));
  Serial.print(++arrivedcount);
  Serial.print(F(" arrived: qos "));
  Serial.print(message.qos);
  Serial.print(F(", retained "));
  Serial.print(message.retained);
  Serial.print(F(", dup "));
  Serial.print(message.dup);
  Serial.print(F(", packetid "));
  Serial.println(message.id);
  Serial.print(F("Payload "));
  char* msg = new char[message.payloadlen+1]();
  memcpy (msg,message.payload,message.payloadlen);
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
        update_alarm = true;
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
        update_alarm = true;
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
    pixel_iotdone();
}



