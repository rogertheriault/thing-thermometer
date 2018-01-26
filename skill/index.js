/* eslint-disable  func-names */
/* eslint quote-props: ["error", "consistent"]*/
/**
 * An AWS IoT ESP8266 "kitchen/bbq" thermometer
 * 
 * This is the Alexa Skill for https://github.com/rogertheriault/thing-thermometer
 * 
 * It allows the user to control the thermometer alarm setpoints and obtain the
 * current temperature (and maybe one day, get notified of an alarm state)
 * 
 * Copyright (c) 2018 Roger Theriault
 * Licensed under the MIT license.
 **/

'use strict';

// include the AWS SDK so we can access IoT and DynamoDB
var AWS = require('aws-sdk');

var async = require("async");

const Alexa = require('alexa-sdk');
const CDN = process.env.CDN;
const recipes = require('./recipes.json');

// Set up a list of recipes that can be used
// TODO filter by thermometer/probe
// TODO stick this logic in a function
const recipe_options = recipes.map(recipe => recipe.title);
let separator = " or ";
const help_options = recipe_options.reduceRight( (previous, current) => {
    let string = current + separator + previous;
    separator = ", ";
    return string;
})

// global so we can avoid passing this to callbacks, there's just no need
var alexa = {};

const languageStrings = {
    'en': {
        translation: {
            SKILL_NAME: 'Kitchen Helper',
            NOW_LETS_COOK: "OK, the thermometer is ready to go! ",
            HELP_MESSAGE: 'You can make ' + help_options + ', or you can set the thermometer. What can I help you with?',
            HELP_REPROMPT: 'What can I help you with?',
            STOP_MESSAGE: 'Goodbye!',
        },
    },
};

// utility methods for creating Image and TextField objects
//const makePlainText = Alexa.utils.TextUtils.makePlainText;
//const makeImage = Alexa.utils.ImageUtils.makeImage;

// OUR CUSTOM INTENTS
// cookSomething {foodToCook} (starts the recipe)
// changeTemperature {heatOrCool,newTemp}
// getStatus
// nextStep
// SLOTS
// foodToCook = yogurt, ricotta
// heatOrCool = heat, cool
// newTemp = ??

const handlers = {
    'LaunchRequest': function () {
        // TODO prompt user to determine the intent
        this.emit('Welcome');
    },
    'Welcome': function () {
        setUserThingId(this, (thingId) => {
            if ( ! thingId ) {
                this.emit(':tell', "You'll need to set up a thermometer first");
                // TODO account linking card and instructions
                return;
            }
            this.emit(':ask', "Welcome! Are we cooking today?");
        })
    },
    'getStatusIntent': function () {
        getDeviceStatus();
        //this.emit(':tellWithCard', "The temperature is 79 degrees", this.t('SKILL_NAME'),
        //    "The temperature is 79 degrees" );
    },
    'nextStepIntent': function () {
        // get user's state and then get the next step and update the device
        //thingId = getThingId(this);
        console.log(this.attributes);
        let currentRecipe = this.attributes["recipe"];
        if ( ! currentRecipe ) {
            this.emit(':tell', "I'm sorry, you're not cooking anything right now.");
            return;
        }

        let stepid = this.attributes["step"] + 1;
        let newStep = currentRecipe.steps.reduce((curr, prev) => {
            return (curr.step === stepid) ? curr : prev;
        })
        // TODO
        // handle stepid not found (ie, we are done)
        // handle recipe == complete (we are done)
        
        let responseText = newStep.speak;
        this.response.speak(responseText);

        let desired = {};
        desired.alarm_high = newStep.alarm_high || null;
        desired.alarm_low = newStep.alarm_low || null;
        desired.timer = newStep.timer || null;
        desired.step = stepid;

        // save state
        this.attributes['step'] = stepid;
        this.attributes['timestamp'] = Date.now();

        this.emit(':saveState', true)
        updateDevice(this, desired);
    },
    'cookSomethingIntent': function () {
        this.emit('cookSomething');
    },
    'cookSomething': function () {
        console.log("User state: " + this.attributes["step"]);
        // handle cooking yogurt or ricotta cheese
        const slots = this.event.request.intent.slots;
        let responseText = "";
        let displayText = "";
        if (slots.foodToCook.value) {
            const food = slots.foodToCook.value;
            console.log( food );
            const recipe = getRecipe(food);
            console.log( recipe );
            if ( !recipe ) {
                responseText = "I can't handle making " + food + " yet, Sorry." +
                    " Would you like to set the high or low temperature?";
                this.emit(':ask', responseText, "Please say that again?");
                return;
            }
            // get the first step or the step with step = 1
            const firstStep = recipe.steps.reduce((prev,curr) => {
                return (curr.step === 1) ? curr : prev;
            });
            responseText = recipe.speak + " " + firstStep.speak;
            displayText = "Making " + recipe.title + "\n" + firstStep.display;
            this.response.speak(responseText);

            let desired = {};
            desired.alarm_high = firstStep.alarm_high || null;
            desired.alarm_low = firstStep.alarm_low || null;
            desired.timer = firstStep.timer || null;
            desired.mode = recipe.id;
            desired.step = 1; // start at 1

            // save state
            this.attributes['recipe'] = recipe;
            this.attributes['step'] = 1;
            this.attributes['timestamp'] = Date.now();
            this.attributes['started'] = Date.now();

            // set the desired device state
            updateDevice(this, desired);
            this.emit(':saveState', true);
        } else {
            this.emit(':elicitSlot', "foodToCook", "I can make " + help_options +
                ", Which would you like?", "Please say that again?");
        }

        // TODO move some of this to helpers
        // Create speech output
        const speechOutput = responseText; //this.t('NOW_LETS_COOK') + responseText;
 
        if (this.event.context.System.device.supportedInterfaces.Display) {
            // utility methods for creating Image and TextField objects
            const makePlainText = Alexa.utils.TextUtils.makePlainText;
            const makeImage = Alexa.utils.ImageUtils.makeImage;

            const builder = new Alexa.templateBuilders.BodyTemplate1Builder();

	        const template = builder.setTitle(this.t('SKILL_NAME'))
			    .setBackgroundImage(makeImage(CDN + '/clouds.png'))
			    .setTextContent(makePlainText(displayText))
			    .build();

	        this.response.speak(speechOutput)
				.renderTemplate(template);
            this.emit(':responseReady');
        } else {
            this.emit(':tellWithCard', speechOutput, this.t('SKILL_NAME'),
                displayText );
        }
    },
    'AMAZON.HelpIntent': function () {
        const speechOutput = this.t('HELP_MESSAGE');
        const reprompt = this.t('HELP_MESSAGE');
        this.emit(':ask', speechOutput, reprompt);
    },
    'AMAZON.CancelIntent': function () {
        this.emit(':tell', this.t('STOP_MESSAGE'));
    },
    'AMAZON.StopIntent': function () {
        responseText = "Stopping your recipe now."
        this.response.speak(responseText);
        desired.alarm_high = 0;
        desired.alarm_low = 0;
        desired.mode = "";
        desired.step = 0;
        this.attributes["recipe"] = undefined;
        this.attributes["step"] = undefined;
        this.attributes["started"] = undefined;
        this.attributed["timestamp"] = Date.now();
        updateDevice(this, desired);
        this.emit(':tell', this.t('STOP_MESSAGE'));
    },
    'Unhandled': function () {
        console.log("unhandled: " + process.env.APP_ID);
        console.log(this.event);
        console.log("Intent:");
        console.log(this.event.request.intent);
        var speechOutput = "Sorry, can you try again please";
        this.emit(':tell', speechOutput);
    },
};

exports.handler = function (event, context, callback) {
    // NOTE alexa is already defined as a global so functions can access it
    alexa = Alexa.handler(event, context, callback);

    alexa.appId = process.env.APP_ID;
    alexa.dynamoDBTableName = process.env.DYNAMODB_STATE_TABLE;
    // To enable string internationalization (i18n) features, set a resources object.
    console.log(event);
    console.log(context);
    alexa.resources = languageStrings;

    alexa.registerHandlers(handlers);
    alexa.execute();
};

/*
 * fetch a Thing's shadow state before responding to the user
 * 
 * This is reasonably fast but we might also want to try Alexa's new async
 * responses i.e. Progressive Response
 */
function getDeviceStatus(sess) {
    console.log("getting device status");
    let thingId = sess.attributes["thingId"];
    if ( !thingId ) {
        console.log("Get device: no id");
        return false;
    }
    var thing = new AWS.IotData({endpoint: process.env.THING_API});
    var params = {
        thingName: thingId
    }
    thing.getThingShadow(params, function (err, data) {
        if (err) {
            // do something
            console.log('getThingShadow error:');
            console.log(err);
        } else {
            console.log('getThingShadow success:');
            console.log(data);
            var shadow = JSON.parse(data.payload);
            console.log(shadow);
            let cooking = shadow.state.reported.mode || "thermometer";
            if (shadow.state.reported.temperature) {
                alexa.emit(':tell', "Your " +
                    cooking +
                    " is now " +
                    shadow.state.reported.temperature +
                    " degrees.");
                return;
            }
        }
        alexa.emit(':tell', "I'm sorry Dave, I can't do that");
    });
}

/*
 * request a device shadow change and then respond to the user
 * 
 * We're passing the Alexa reference so we can call it
 */
function updateDevice(sess, desired) {
    console.log("controlling a device");
    console.log(desired);
    let thingId = sess.attributes["thingId"];
    if ( !thingId ) {
        console.log('Update: no device id');
        return;
    }
    var thing = new AWS.IotData({endpoint: process.env.THING_API});
    var payload = { 
        state: {
            desired: desired
        }
    };
    var params = {
        thingName: thingId,
        payload: JSON.stringify(payload)
    };
    thing.updateThingShadow(params, function (err, data) {
        if (err) {
            // do something
            console.log('updateThingShadow error:');
            console.log(err);
            alexa.emit(':tell', "Oh gosh, something went wrong!");
        } else {
            console.log('updateThingShadow success:');
            console.log(data);
            alexa.emit(':responseReady');
        }
    });
}
/**
 * return a recipe object matching a "Food" slot
 * 
 * Recipes can match multiple utterances e.g. "ricotta cheese" and "ricotta"
 * 
 * @param {String} slot 
 */
function getRecipe(slot) {
    let matches = recipes.filter(recipe => recipe.slots.includes(slot));
    if (matches.length > 0) {
        return matches[0];
    }
    return null;
}

/**
 * Utility to set a user's thing ID in their session attributes
 * 
 * will check user's saved session data before looking in the userdevices table
 * Currently only supports one device per user
 * Once the ID is set, it can be accessed in attributes["thingId"]
 * 
 * The callback is called once the attribute is set
 * NOTE this only needs to be called when a new session starts
 * 
 * @param {object} sess - main Alexa object
 * @param {function} callback - callback to execute after id retrieved
 */
function setUserThingId(sess, callback) {
    if ( sess.attributes && sess.attributes["thingId"] ) {
        console.log("using thing " + sess.attributes["thingId"]);
        if ( callback ) {
            callback( sess.attributes["thingId"] );
        }
    }

    let userId = sess.event.session.user.userId;
    // look in the USER/DEVICES table for the thing Id
    const ddb = new AWS.DynamoDB({apiVersion: '2012-10-08'});

    var params = {
        TableName: process.env.DYNAMODB_THING_TABLE,
        Key: {
            'userId': {"S": userId},
        },
        AttributesToGet: [ 'thingId' ]
    };

    // Call DynamoDB to read the item from the table
    let getItemPromise = ddb.getItem(params).promise();
    getItemPromise.then( (data) => {
            // data.Item looks like:
            // { thingId: {S: "actualId"} }
            let thingId = data.Item.thingId.S;
            console.log("Found user thing " + thingId);
            // save in user session
            sess.attributes["thingId"] = thingId;
            if ( callback ) {
                callback(sess);
            }
         })
        .catch( err => {
            console.log(err);
        });
}