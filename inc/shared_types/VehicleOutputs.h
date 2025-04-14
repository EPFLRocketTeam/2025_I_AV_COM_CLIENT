#ifndef VEHICLE_OUTPUTS_H
#define VEHICLE_OUTPUTS_H

#include "../Payload.h"

struct VehicleOutputs
{
    double timestamp;     // in ms
    double d1;            // in degrees
    double d2;            // in degrees
    double avg_throttle;  // from 0 to 1
    double throttle_diff; // from -1 to 1, top_throttle - bot_throttle

    void serialize(Payload &payload) const
    {
        payload.write(timestamp);
        payload.write(d1);
        payload.write(d2);
        payload.write(avg_throttle);
        payload.write(throttle_diff);
    }

    void deserialize(Payload &payload)
    {
        payload.read(timestamp);
        payload.read(d1);
        payload.read(d2);
        payload.read(avg_throttle);
        payload.read(throttle_diff);
    }
};

#endif VEHICLE_OUTPUTS_H