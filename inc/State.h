#ifndef CONTROLLER_PROTO_STATE_H
#define CONTROLLER_PROTO_STATE_H

#include "Vec3.h"


struct State{
    Vec3 pos=Vec3::Zero(); // in meters
    Vec3 vel=Vec3::Zero(); // in m/s
    Vec3 att=Vec3::Zero(); // in radians
    Vec3 rate=Vec3::Zero(); // in rad/s
};

#endif //CONTROLLER_PROTO_STATE_H
