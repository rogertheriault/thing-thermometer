# Alexa Thermometer Skill

## Setup
- set and export a bash environment var AWS_ACCOUNT_ID
- from the main repo dir (one up from here) run the script:
$ ./scripts/createlambda.sh skill lambda-function-name
- Go to the AWS Lambda console and configure the Environment Variables with:
1. APP_ID The Alexa app id
2. THING_API The api endpoint domain of the things
3. THING_NAME The actual thing name
4. CDN A URL where assets can be fetched for the skill (audio and images)
- Configure the trigger Alexa Skills Kit
- Go to the Alexa Skill console and enter the function ARN in the configuratio
- Test the skill in the console
