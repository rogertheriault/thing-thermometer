#!/bin/bash

THING_GROUP="thing_simulation"
THING_API="yourapiendpoint.iot.us-east-1.amazonaws.com"

## test as cron
lambda-local -l index.js -h handler -e examples/cron.js -E "{\"THING_API\":\"$THING_API\",\"SIMULATE_GROUP\":\"$THING_GROUP\"}"

## test as delta event trigger from IoT rule (edit delta.js to set the correct thing ID)
lambda-local -l index.js -h handler -e examples/delta.js -E "{\"THING_API\":\"$THING_API\",\"SIMULATE_GROUP\":\"$THING_GROUP\"}"
