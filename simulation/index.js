/**
 * simulation support lambda function for thing-thermometer
 *
 * This optional lambda can provide simulated state changes for
 * Things created by the thermometer skill.
 *
 * It can be triggered from IoT rules or run on a cron
 * 
 * It will fetch all the things in the group, then, for each,
 * get the current state, and then if needed, update it
 * It follows a similar flow as the Arduino code, without heat.
 * * if the thermometer is not past the target, move it about half way
 * * if there is a delta on the shadow, update the state to match
 * * and also update the target temps per the recipe
 * * finally, if we're on an idle step, return to idle mode "measure"
 *
 */

var AWS = require("aws-sdk");
const iot = new AWS.Iot();
const iotdata = new AWS.IotData({
    endpoint: process.env.THING_API
})

exports.handler = (event, context, callback) => {
    console.log('Received event: ', JSON.stringify(event));
    console.log('context: ', JSON.stringify(context));

    // if this is a delta trigger, process just the one thing
    // otherwise, process as many things as possible in the scheduled event
    // Exact same logic to ensure it's a group member - at least until 
    // the components can be refactored out of a callback chain
    let onlyThing = null;
    if (event.thingId) {
        onlyThing = event.thingId;
    }
    console.log("processing " + (onlyThing ? onlyThing : "scheduled event"));
    let params = {
        thingGroupName: process.env.SIMULATE_GROUP,
        maxResults: 100,
        recursive: false
    }

    iot.listThingsInThingGroup(params, function(err, data) {
        if (err) {
            console.log(err, err.stack);
            return callback(err);
        } else {
            data.things.forEach( thingId => {
                if (onlyThing && thingId !== onlyThing) {
                    return;
                }
                console.log("updating thing " + thingId);
                iotdata.getThingShadow({
                    thingName: thingId
                }, function(err, data) {
                    if (err) {
                        console.log('getThingShadow error:');
                        console.log(err);
                    }
                    if (!err) {
                        let target = 24; // room temperature
                        let temperature = 19; // cool room
                        let variance = 0;
                        let state = {};
                        if (data) {
                            console.log("current");
                            console.log(data);
                            let payload = JSON.parse(data.payload);
                            state = payload.state || {};
                            console.log(state);
                        }

                        // copy only the reported section to the new state
                        let newState = {reported: state.reported};

                        // pretend to be a device responding to a delta
                        let delta = false;
                        if (state.delta) {
                            // mode & step is all we need, and there are only so many
                            // options, so for now, hardcode
                            delta = state.delta;
                            for ( var key in delta ) {
                                newState.reported[key] = delta[key];
                            }
                            let mode = delta.mode || state.reported.mode || measure;
                            let step = delta.step || 1;
                            newState.reported['alarm_low'] = 0;
                            newState.reported['alarm_high'] = 0;
                            switch (mode + '-' + step) {
                                case 'yogurt-1':
                                    newState.reported['alarm_high'] = 82;
                                    break;
                                case 'yogurt-2':
                                    newState.reported['alarm_low'] = 43;
                                    break;
                                case 'ricotta-1':
                                    newState.reported['alarm_high'] = 77;
                                    break;
                                default:
                                    break;
                            }
                        }

                        // pretend to have heating or cooling action going on
                        if (state.reported && state.reported.temperature) {
                            temperature = state.reported.temperature;
                        }
                        let needUpdate = false;
                        if (state.reported.alarm_high && state.reported.alarm_high > 0) {
                            target = state.reported.alarm_high;
                            needUpdate = target > temperature;
                            variance = +3;
                        } else if (state.reported.alarm_low && state.reported.alarm_low > 0) {
                            target = state.reported.alarm_low;
                            needUpdate = target < temperature;
                            variance = -3;
                        }
                        // skip update if the alarm would be on, provided there's no delta
                        // first check whether the recipe's done
                        if ( (state.reported.mode === "yogurt" &&
                                state.reported.step == 3) ||
                                (state.reported.mode === "ricotta" &&
                                state.reported.step == 2) ) {
                            console.log("setting back to idle");
                            newState.desired = {
                                "mode": "measure",
                                "step": 0
                            }
                        }
                        if (variance && !needUpdate && !delta && !newState.desired) {
                            console.log("no need to update " + thingId);
                            return;
                        }

                        // new temperature, nudging
                        temperature = temperature
                            + ( (target - temperature) / 2 )
                            + variance;

                        newState.reported.temperature = parseInt(temperature, 10);
                        
                        console.log(newState);

                        // now update the thing
                        iotdata.updateThingShadow({
                            thingName: thingId,
                            payload: JSON.stringify({state: newState})
                        }, function(err, data) {
                            console.log(err);
                            console.log(data);
                        });
                    } else {
                        console.log("error");
                        console.log(err);
                    }
                });
            });
            ///return callback(null);
        }
    });

};
