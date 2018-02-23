Thing Simulation
----------------

This is a totally optional component to simulate thermometers for the main skill.


### IoT Group and Rule
Go to the AWS IoT console and create a group. This group will hold all the simulation things.
Add the group name to the main skill Lambda's environment variables as SIMULATE_GROUP
This will enable the ability for users to ask for a simuated device

Create an IoT rule:
Rule query statement should be
```
SELECT *, topic(3) as thingId FROM '$aws/things/+/shadow/update/delta'
```
Action should invoke this lambda function

### Uploading
You can create this lambda using the utility bash script,
e.g. from the main repo dir:
```
./scripts/createlambda.sh simulation SimulationLambdaName
```

Then set up two env variables, SIMULATE_GROUP, and THING_API

And set up a trigger to run this once per minute in addition to running it when you have a delta

### Testing with lambda-local

To test, install lambda-local with
```
npm install -g lambda-local
```

then in this directory edit test.sh to set up the env variables
and then run test.sh (or you can run the commands there separately)

Edit examples/*.js to modify the event data

