#ifndef VEHICLE_INPUTS_H
#define VEHICLE_INPUTS_H

#include "State.h"
#include "SetpointSelection.h"
#include "Payload.h"

struct VehicleInputs
{
    bool armed;                          // if the drone is armed
    double timestamp;                    // the time the control input was created in ms
    State desired_state;                 // the desired state of the drone
    State current_state;                 // the current state of the drone
    SetpointSelection setpointSelection; // the setpoint selection
    double inline_thrust;                // the inline thrust

    void serialize(Payload &payload) const
    {
        payload.write(armed);
        payload.write(timestamp);
        desired_state.serialize(payload);
        current_state.serialize(payload);
        setpointSelection.serialize(payload);
        payload.write(inline_thrust);
    }

    void deserialize(Payload &payload)
    {
        payload.read(armed);
        payload.read(timestamp);
        desired_state.deserialize(payload);
        current_state.deserialize(payload);
        setpointSelection.deserialize(payload);
        payload.read(inline_thrust);
    }
};

#endif // VEHICLE_INPUTS_H