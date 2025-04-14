#ifndef SETPOINT_SELECTION_H
#define SETPOINT_SELECTION_H

#include "Vec3.h"

class SetpointSelection
{
  public:
    bool posSPActive[3];
    bool velSPActive[3];
    bool attSPActive[3];
    bool rateSPActive[3];

    static Vec3 selectSetpoint(const Vec3 &desiredStateSP, const Vec3 &pidChainSP, const bool *activeSP)
    {
        Vec3 setpoint = pidChainSP;
        if (activeSP[0])
            setpoint.x = desiredStateSP.x;
        if (activeSP[1])
            setpoint.y = desiredStateSP.y;
        if (activeSP[2])
            setpoint.z = desiredStateSP.z;
        return setpoint;
    }

    void serialize(Payload &payload) const
    {
        // Pack all 12 boolean values into 2 bytes to minimize payload size
        uint8_t buffer[2] = {0, 0};

        // Pack posSPActive (3 bits)
        if (posSPActive[0])
            buffer[0] |= (1 << 0);
        if (posSPActive[1])
            buffer[0] |= (1 << 1);
        if (posSPActive[2])
            buffer[0] |= (1 << 2);

        // Pack velSPActive (3 bits)
        if (velSPActive[0])
            buffer[0] |= (1 << 3);
        if (velSPActive[1])
            buffer[0] |= (1 << 4);
        if (velSPActive[2])
            buffer[0] |= (1 << 5);

        // Pack attSPActive (3 bits across byte boundary)
        if (attSPActive[0])
            buffer[0] |= (1 << 6);
        if (attSPActive[1])
            buffer[0] |= (1 << 7);
        if (attSPActive[2])
            buffer[1] |= (1 << 0);

        // Pack rateSPActive (3 bits)
        if (rateSPActive[0])
            buffer[1] |= (1 << 1);
        if (rateSPActive[1])
            buffer[1] |= (1 << 2);
        if (rateSPActive[2])
            buffer[1] |= (1 << 3);

        // Write the packed data to the payload
        payload.write(buffer, sizeof(buffer));
    }

    void deserialize(Payload &payload)
    {
        // Read the packed data from the payload
        uint8_t buffer[2];
        payload.read(buffer, sizeof(buffer));

        // Unpack posSPActive
        posSPActive[0] = (buffer[0] & (1 << 0)) != 0;
        posSPActive[1] = (buffer[0] & (1 << 1)) != 0;
        posSPActive[2] = (buffer[0] & (1 << 2)) != 0;

        // Unpack velSPActive
        velSPActive[0] = (buffer[0] & (1 << 3)) != 0;
        velSPActive[1] = (buffer[0] & (1 << 4)) != 0;
        velSPActive[2] = (buffer[0] & (1 << 5)) != 0;

        // Unpack attSPActive
        attSPActive[0] = (buffer[0] & (1 << 6)) != 0;
        attSPActive[1] = (buffer[0] & (1 << 7)) != 0;
        attSPActive[2] = (buffer[1] & (1 << 0)) != 0;

        // Unpack rateSPActive
        rateSPActive[0] = (buffer[1] & (1 << 1)) != 0;
        rateSPActive[1] = (buffer[1] & (1 << 2)) != 0;
        rateSPActive[2] = (buffer[1] & (1 << 3)) != 0;
    }
};

const SetpointSelection RATE_CONTROL_SELECTION = {
    .posSPActive = {0, 0, 0},
    .velSPActive = {0, 0, 0},
    .attSPActive = {0, 0, 0},
    .rateSPActive = {1, 1, 1}};

const SetpointSelection ATTITUDE_CONTROL_SELECTION = {
    .posSPActive = {0, 0, 0},
    .velSPActive = {0, 0, 0},
    .attSPActive = {1, 1, 1},
    .rateSPActive = {0, 0, 0}};

const SetpointSelection ATTITUDE_CONTROL_YAW_RATE_SELECTION = {
    .posSPActive = {0, 0, 0},
    .velSPActive = {0, 0, 0},
    .attSPActive = {1, 1, 0},
    .rateSPActive = {0, 0, 1}};

const SetpointSelection ALTITUDE_CONTROL_SELECTION = {
    .posSPActive = {0, 0, 1},
    .velSPActive = {0, 0, 0},
    .attSPActive = {1, 1, 0},
    .rateSPActive = {0, 0, 1}};

const SetpointSelection VERTICAL_VELOCITY_CONTROL_SELECTION = {
    .posSPActive = {0, 0, 0},
    .velSPActive = {0, 0, 1},
    .attSPActive = {1, 1, 0},
    .rateSPActive = {0, 0, 1}};

const SetpointSelection POSITION_CONTROL_SELECTION = {
    .posSPActive = {1, 1, 1},
    .velSPActive = {0, 0, 0},
    .attSPActive = {0, 0, 0},
    .rateSPActive = {0, 0, 0}};

const SetpointSelection VELOCITY_CONTROL_SELECTION = {
    .posSPActive = {0, 0, 0},
    .velSPActive = {1, 1, 1},
    .attSPActive = {0, 0, 0},
    .rateSPActive = {0, 0, 0}};

#endif