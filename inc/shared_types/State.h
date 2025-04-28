#ifndef STATE_H
#define STATE_H

#include "Vec3.h"
#include "Payload.h"


struct State{
    Vec3 pos=Vec3::Zero(); // in meters
    Vec3 vel=Vec3::Zero(); // in m/s
    Vec3 att=Vec3::Zero(); // in radians
    Vec3 rate=Vec3::Zero(); // in rad/s
    
    void serialize(Payload& payload) const {
        pos.serialize(payload);
        vel.serialize(payload);
        att.serialize(payload);
        rate.serialize(payload);
    }

    void deserialize(Payload& payload) {
        pos.deserialize(payload);
        vel.deserialize(payload);
        att.deserialize(payload);
        rate.deserialize(payload);
    }
};

#endif //STATE_H
