/* eslint-disable  func-names */
/* eslint quote-props: ["error", "consistent"]*/
/**
 * This is the Alexa Skill for https://github.com/rogertheriault/thing-thermometer
 * 
 * It allows the user to control the thermometer alarm setpoints and obtain the
 * current temperature (and maybe one day, get notified of an alarm state)
 * 
 **/

'use strict';

// include the AWS SDK so we can access IoT and DynamoDB
var AWS = require('aws-sdk');

const Alexa = require('alexa-sdk');
const CDN = process.env.CDN;

const languageStrings = {
    'en': {
        translation: {
            FACTS: [
            ],
            SKILL_NAME: 'Kitchen Helper',
            NOW_LETS_COOK: "OK, the thermometer is ready to go! ",
            HELP_MESSAGE: 'You can cook yogurt or ricotta, or you can set the thermometer. What can I help you with?',
            HELP_REPROMPT: 'What can I help you with?',
            STOP_MESSAGE: 'Goodbye!',
        },
    },
};

// utility methods for creating Image and TextField objects
//const makePlainText = Alexa.utils.TextUtils.makePlainText;
//const makeImage = Alexa.utils.ImageUtils.makeImage;

// OUR CUSTOM INTENTS
// cookSomething {foodToCook}
// changeTemperature {heatOrCool,newTemp}
// getStatus
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
        this.emit(':ask', "Welcome! Are we cooking today?");
    },
    'getStatusIntent': function () {
        getDeviceStatus( this ); // TODO unhack
        //this.emit(':tellWithCard', "The temperature is 79 degrees", this.t('SKILL_NAME'),
        //    "The temperature is 79 degrees" );
    },
    'cookSomethingIntent': function () {
        this.emit('cookSomething');
    },
    'cookSomething': function () {
        // handle cooking yogurt or ricotta cheese
        const slots = this.event.request.intent.slots;
        let responseText = "";
        let displayText = "";
        if (slots.foodToCook.value) {
            const food = slots.foodToCook.value;
            switch (food) {
                case 'yogurt':
                    // set max first
                    responseText = "I love " + food +
                        "! Start heating milk in a pot. Insert the probe in the milk.";
                    displayText = "Making Yogurt. Insert probe in milk and start heating."
                    updateDevice({food: food, response: responseText, max: 88});
                    break;
                case 'ricotta':
                    responseText = "OK, " + food + " it is." +
                        "Start heating milk in a pot. Insert the probe in the milk.";
                    displayText = "Making Ricotta Cheese. Insert probe in milk and start heating."
                    break;
                default:
                    responseText = "I can't handle making " + food + " yet, Sorry. You can ask me to set the high or low temperature.";
                    // can't cook that yet, please set a temperature
                    displayText = "Unable to help with " + food +
                        ". Set a max or min temperature.";
                    break;
            }
        }

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
        this.emit(':tell', this.t('STOP_MESSAGE'));
    },
    'Unhandled': function () {
        console.log("unhandled: " + process.env.APP_ID);
        console.log(this.event);
        console.log("Slots:");
        console.log(this.event.request.intent.slots);
        var speechOutput = "Sorry, can you try again please";
        this.emit(':tell', speechOutput);
    },
};

exports.handler = function (event, context) {
    const alexa = Alexa.handler(event, context);
    // SIGH alexa.appId = process.env.APP_ID;
    // To enable string internationalization (i18n) features, set a resources object.
    alexa.resources = languageStrings;
    alexa.registerHandlers(handlers);
    alexa.execute();
};

/*
 * fetch a Thing's shadow state before responding to the user
 * 
 * This is reasonably fast but we might also want to try Alexa's new async
 * responses
 */
function getDeviceStatus( alexa ) {
    var thing = new AWS.IotData({endpoint: process.env.THING_API});
    var params = {
        // TODO get via user's ID
        thingName: process.env.THING_NAME
    }
    thing.getThingShadow(params, function (err, data) {
        if (err) {
            // do something
            console.log(err);
        } else {
            console.log(data);
            var shadow = JSON.parse(data.payload);
            console.log(shadow);
            if (shadow.state.reported.temperature) {
                alexa.emit(':tell', "Your stuff is now " +
                    shadow.state.reported.temperature + " degrees.");
                return;
            }
        }
        alexa.emit(':tell', "I'm sorry Dave, I can't do that");
    })
}