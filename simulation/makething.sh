#!/bin/bash

# create a simulation thing, add it to the group, and set the thermometer to 15C

THINGNAME=SIM_$1
GROUP="thing_simulation"

aws iot create-thing --thing-name $THINGNAME --attribute-payload '{"attributes":{"SIMULATION":"KitchenHelper"}}'
aws iot add-thing-to-thing-group --thing-name $THINGNAME --thing-group-name $GROUP
aws iot-data update-thing-shadow --thing-name $THINGNAME --payload '{"state":{"reported":{"temperature":15}}}' /dev/null
