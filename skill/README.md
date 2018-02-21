# Alexa Thermometer Skill

This is just the Lambda function for the Alexa skill

## Setup

See the main README and the Hackster project for a better walkthrough

- set and export a bash environment var AWS_ACCOUNT_ID
- install node.js
- run "npm install" to pull down the modules used in the project
- from the main repo dir (one up from here) run the script:
$ ./scripts/createlambda.sh skill lambda-function-name
- This zips the code and creates a Lambda function
- To update the code, use
$ ./scripts/updatelambda.sh skill lambda-function-name
- Go to the AWS Lambda console and configure the Environment Variables with:
1. APP_ID The Alexa app id
2. THING_API The api endpoint domain of the things
3. CDN A URL where assets can be fetched for the skill (audio and images)
- Configure the trigger Alexa Skills Kit
- Go to the Alexa Skill console and enter the function ARN in the configuration
- Supply an interaction model or use the one here as a start
- Test the skill in the console

I've supplied the Alexa Interaction Model (in ../assets/interactions.json), you can copy that straight into the Alexa JSON editor, but will need to change the "Invocation name". A warning, if you have multiple skills with similar names, Alexa may not be able to distinguish them when you do a direct invocation.

For example, "Kitchen Gadget" and "Kitchen Thermometer" may be too similar. If you have problems and see no errors in the Cloudwatch logs, try changing the invocation name.
