#ifndef ARDUINO
#include "Payload.h"
#endif // ARDUINO

#include <cstring>

Payload::Payload() : payloadSize(0), readPosition(0) {
    std::memset(payload, 0, MAX_SIZE);
}

bool Payload::SetBytes(const uint8_t* bytes, size_t size) {
    if (size > MAX_SIZE) {
        return false; // Error: Size too large
    }
    
    std::memcpy(payload, bytes, size);
    payloadSize = size;
    readPosition = 0;
    return true;
}

const uint8_t* Payload::GetBytes() const {
    return payload;
}

size_t Payload::GetSize() const {
    return payloadSize;
}

bool Payload::WriteInt(int value) {
    if (payloadSize + sizeof(int) > MAX_SIZE) {
        return false; // Error: Not enough space
    }
    
    std::memcpy(payload + payloadSize, &value, sizeof(int));
    payloadSize += sizeof(int);
    return true;
}

bool Payload::WriteFloat(float value) {
    if (payloadSize + sizeof(float) > MAX_SIZE) {
        return false;
    }
    
    std::memcpy(payload + payloadSize, &value, sizeof(float));
    payloadSize += sizeof(float);
    return true;
}

bool Payload::WriteDouble(double value) {
    if (payloadSize + sizeof(double) > MAX_SIZE) {
        return false;
    }
    
    std::memcpy(payload + payloadSize, &value, sizeof(double));
    payloadSize += sizeof(double);
    return true;
}

bool Payload::WriteBool(bool value) {
    if (payloadSize + 1 > MAX_SIZE) {
        return false;
    }
    
    payload[payloadSize] = value ? 1 : 0;
    payloadSize += 1;
    return true;
}

bool Payload::WriteBytes(const uint8_t* bytes, size_t size) {
    if (payloadSize + size > MAX_SIZE) {
        return false;
    }
    
    std::memcpy(payload + payloadSize, bytes, size);
    payloadSize += size;
    return true;
}

bool Payload::WriteVec3(const Vec3 &vec)
{
    bool success = true;
    success &= WriteDouble(vec.x);
    success &= WriteDouble(vec.y);
    success &= WriteDouble(vec.z);
    return success;
}

bool Payload::WriteState(const State &state)
{
    bool success = true;
    success &= WriteVec3(state.pos);
    success &= WriteVec3(state.vel);
    success &= WriteVec3(state.att);
    success &= WriteVec3(state.rate);
    return success;
}

bool Payload::WriteSetpointSelection(const SetpointSelection &setpoint)
{
    // Pack all 12 boolean values into 2 bytes to minimize payload size
    uint8_t buffer[2] = {0, 0};

    // Pack posSPActive (3 bits)
    if (setpoint.posSPActive[0])
        buffer[0] |= (1 << 0);
    if (setpoint.posSPActive[1])
        buffer[0] |= (1 << 1);
    if (setpoint.posSPActive[2])
        buffer[0] |= (1 << 2);

    // Pack velSPActive (3 bits)
    if (setpoint.velSPActive[0])
        buffer[0] |= (1 << 3);
    if (setpoint.velSPActive[1])
        buffer[0] |= (1 << 4);
    if (setpoint.velSPActive[2])
        buffer[0] |= (1 << 5);

    // Pack attSPActive (3 bits across byte boundary)
    if (setpoint.attSPActive[0])
        buffer[0] |= (1 << 6);
    if (setpoint.attSPActive[1])
        buffer[0] |= (1 << 7);
    if (setpoint.attSPActive[2])
        buffer[1] |= (1 << 0);

    // Pack rateSPActive (3 bits)
    if (setpoint.rateSPActive[0])
        buffer[1] |= (1 << 1);
    if (setpoint.rateSPActive[1])
        buffer[1] |= (1 << 2);
    if (setpoint.rateSPActive[2])
        buffer[1] |= (1 << 3);

    // Write the packed bytes to the payload
    return WriteBytes(buffer, sizeof(buffer));
}

bool Payload::ReadInt(int &value) {
    if (readPosition + sizeof(int) > payloadSize) {
        return false; // Error: Not enough bytes
    }
    
    std::memcpy(&value, payload + readPosition, sizeof(int));
    readPosition += sizeof(int);
    return true;
}

bool Payload::ReadFloat(float &value) {
    if (readPosition + sizeof(float) > payloadSize) {
        return false;
    }
    
    std::memcpy(&value, payload + readPosition, sizeof(float));
    readPosition += sizeof(float);
    return true;
}

bool Payload::ReadDouble(double &value) {
    if (readPosition + sizeof(double) > payloadSize) {
        return false;
    }

    std::memcpy(&value, payload + readPosition, sizeof(double));
    readPosition += sizeof(double);
    return true;
}

bool Payload::ReadBool(bool &value) {
    if (readPosition >= payloadSize) {
        return false;
    }
    
    value = payload[readPosition] != 0;
    readPosition += 1;
    return true;
}

bool Payload::ReadBytes(uint8_t* destBuffer, size_t length) {
    if (readPosition + length > payloadSize) {
        return false;
    }
    
    std::memcpy(destBuffer, payload + readPosition, length);
    readPosition += length;
    return true;
}

bool Payload::ReadVec3(Vec3 &vec)
{
    bool success = true;
    success &= ReadDouble(vec.x);
    success &= ReadDouble(vec.y);
    success &= ReadDouble(vec.z);
    return success;
}

bool Payload::ReadState(State &state)
{
    bool success = true;
    success &= ReadVec3(state.pos);
    success &= ReadVec3(state.vel);
    success &= ReadVec3(state.att);
    success &= ReadVec3(state.rate);
    return success;
}

bool Payload::ReadSetpointSelection(SetpointSelection &setpoint)
{
    // Read 2 bytes from the payload
    uint8_t buffer[2];
    bool success = ReadBytes(buffer, sizeof(buffer));

    if (!success)
    {
        return false;
    }

    // Unpack posSPActive
    setpoint.posSPActive[0] = (buffer[0] & (1 << 0)) != 0;
    setpoint.posSPActive[1] = (buffer[0] & (1 << 1)) != 0;
    setpoint.posSPActive[2] = (buffer[0] & (1 << 2)) != 0;

    // Unpack velSPActive
    setpoint.velSPActive[0] = (buffer[0] & (1 << 3)) != 0;
    setpoint.velSPActive[1] = (buffer[0] & (1 << 4)) != 0;
    setpoint.velSPActive[2] = (buffer[0] & (1 << 5)) != 0;

    // Unpack attSPActive
    setpoint.attSPActive[0] = (buffer[0] & (1 << 6)) != 0;
    setpoint.attSPActive[1] = (buffer[0] & (1 << 7)) != 0;
    setpoint.attSPActive[2] = (buffer[1] & (1 << 0)) != 0;

    // Unpack rateSPActive
    setpoint.rateSPActive[0] = (buffer[1] & (1 << 1)) != 0;
    setpoint.rateSPActive[1] = (buffer[1] & (1 << 2)) != 0;
    setpoint.rateSPActive[2] = (buffer[1] & (1 << 3)) != 0;

    return true;
}

bool Payload::ReadControlInputPacket(ControlInputPacket &control_input)
{
    bool success = true;
    success &= ReadBool(control_input.armed);
    success &= ReadState(control_input.desired_state);
    success &= ReadState(control_input.current_state);
    success &= ReadSetpointSelection(control_input.setpointSelection);
    success &= ReadDouble(control_input.inline_thrust);
    return success;
}

bool Payload::WriteControlOutputPacket(const ControlOutputPacket &control_output)
{
    bool success = true;
    success &= WriteDouble(control_output.timestamp);
    success &= WriteDouble(control_output.d1);
    success &= WriteDouble(control_output.d2);
    success &= WriteDouble(control_output.avg_throttle);
    success &= WriteDouble(control_output.throttle_diff);
    return success;
}

void Payload::ResetReadPosition()
{
    readPosition = 0;
}

size_t Payload::GetReadPosition() const {
    return readPosition;
}

void Payload::Clear() {
    payloadSize = 0;
    readPosition = 0;
}
