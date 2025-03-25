// The integer id for each of the packet.
// Each packet structure will need to bed definned in the README
// Each client then need to separately implement the encoder/decoder

#ifndef PACKETID_H
#define PACKETID_H

#ifndef ARDUINO
#include "Vec3.h"
#include "State.h"
#include "SetpointSelection.h"
#endif

enum class PacketId
{
    ControlInput = 1,
    ControlOutput = 2,
};

struct ControlInputPacket
{
    bool armed;                          // if the drone is armed
    double timestamp;                    // the time the control input was created in ms
    State desired_state;                 // the desired state of the drone
    State current_state;                 // the current state of the drone
    SetpointSelection setpointSelection; // the setpoint selection
    double inline_thrust;                // the inline thrust
};

struct ControlOutputPacket
{
    double timestamp;     // in ms
    double d1;            // in degrees
    double d2;            // in degrees
    double avg_throttle;  // from 0 to 1
    double throttle_diff; // from -1 to 1, top_throttle - bot_throttle
};

#endif // PACKETID_H