#ifndef PACKET_H
#define PACKET_H

#include <cstddef>
#include <cstdint>

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

    // Write methods for different types
    bool WriteInt(int value);
    bool WriteFloat(float value);
    bool WriteDouble(double value);
    bool WriteBool(bool value);
    bool WriteBytes(const uint8_t *bytes, size_t size);

    // Read methods for different types
    bool ReadInt(int &value);
    bool ReadFloat(float &value);
    bool ReadDouble(double &value);
    bool ReadBool(bool &value);
    bool ReadBytes(uint8_t *destBuffer, size_t length);

    // Utility methods
    void ResetReadPosition();
    size_t GetReadPosition() const;
    void Clear();
};

#endif // PACKET_H