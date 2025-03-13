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

void Payload::ResetReadPosition() {
    readPosition = 0;
}

size_t Payload::GetReadPosition() const {
    return readPosition;
}

void Payload::Clear() {
    payloadSize = 0;
    readPosition = 0;
}
