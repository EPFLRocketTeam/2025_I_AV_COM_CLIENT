#ifndef CONTROLLER_PROTO_SETPOINT_SELECTION_H
#define CONTROLLER_PROTO_SETPOINT_SELECTION_H

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