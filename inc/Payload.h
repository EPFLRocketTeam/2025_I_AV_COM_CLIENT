#ifndef PACKET_H
#define PACKET_H

#include <cstddef>
#include <cstdint>

#ifndef ARDUINO
#include "Packets.h"
#endif

class Payload
{
  private:
    static const size_t MAX_SIZE = 1024; // Maximum packet size in bytes
    uint8_t payload[MAX_SIZE];
    size_t payloadSize;
    size_t readPosition;

  public:
    // Constructor
    Payload();

    // Payload Getters and Setters
    bool SetBytes(const uint8_t *bytes, size_t size);
    const uint8_t *GetBytes() const;
    size_t GetSize() const;

    // Write methods for basic types
    bool WriteInt(int value);
    bool WriteFloat(float value);
    bool WriteDouble(double value);
    bool WriteBool(bool value);
    bool WriteBytes(const uint8_t *bytes, size_t size);

    // Write methods for our custom types
    bool WriteVec3(const Vec3 &vec);
    bool WriteState(const State &state);
    bool WriteSetpointSelection(const SetpointSelection &setpoint);

    // Write methods for the actual packet data
    bool WriteControlInputPacket(const ControlInputPacket &control_input);
    bool WriteControlOutputPacket(const ControlOutputPacket &control_output);

    // Read methods for basic types
    bool ReadInt(int &value);
    bool ReadFloat(float &value);
    bool ReadDouble(double &value);
    bool ReadBool(bool &value);
    bool ReadBytes(uint8_t *destBuffer, size_t length);

    // Read methods for our custom types
    bool ReadVec3(Vec3 &vec);
    bool ReadState(State &state);
    bool ReadSetpointSelection(SetpointSelection &setpoint);

    // Read methods for the actual packet data
    bool ReadControlInputPacket(ControlInputPacket &control_input);
    bool ReadControlOutputPacket(ControlOutputPacket &control_output);

    // Utility methods
    void ResetReadPosition();
    size_t GetReadPosition() const;
    void Clear();
};

#endif // PACKET_H