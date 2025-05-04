#ifndef UART_DRIVER_H
#define UART_DRIVER_H

#include "Payload.h"

#include <cstddef> // For size_t
#include <cstdint> // For uint8_t
#include <optional>
#include <string>
#include <iostream>

// Special bytes
constexpr uint8_t START_BYTE = 0x7E;
constexpr uint8_t END_BYTE = 0x7F;
constexpr uint8_t ESCAPE_BYTE = 0x7D;
constexpr uint8_t ESCAPE_MASK = 0x20;

// Buffers for storing the packets
constexpr size_t MAX_PACKET_SIZE_STUFFED = (MAX_PAYLOAD_SIZE + 2) * 2 + 2;
constexpr size_t MAX_PACKET_SIZE_UNSTUFFED = MAX_PAYLOAD_SIZE + 4;

// Buffers for reading and writing UART
constexpr size_t RECEIVE_BUFFER_SIZE = 1024;
constexpr size_t SEND_BUFFER_SIZE = 1024;

// The parent driver class
class UARTDriver
{
  public:
    UARTDriver();
    ~UARTDriver() = default;

  protected:
    // Tries to find a packet in the receive buffer and decodes it into payload.
    // Returns true if successful, false otherwise.
    // This advances the readIndex of the receive buffer
    bool DecodePacket(Payload &payload);

    // Tries to encode the payload into a packet and places it in the send buffer.
    // Returns true if successful, false otherwise.
    void EncodePacket(Payload &payload);

    // This is a circular buffer.
    // New data is put at the write index.
    // DecodePacket() reads packets from the read index, and peeks at the peek index.
    // DecodePacket() discard bytes by incrementing the read index.
    uint8_t receiverBuffer[RECEIVE_BUFFER_SIZE];
    size_t receiveBufferWriteIndex; // Where we are currently writing to
    size_t receiveBufferReadIndex;  // Where we're currently reading from
    size_t receiveBufferPeekIndex;  // Where we are currently peeking

    // This is a normal buffer.
    uint8_t sendBuffer[SEND_BUFFER_SIZE];
    size_t sendBufferIndex; // The size of the send buffer

  private:
    // Compute the checksum of the data.
    uint8_t ComputeChecksum(const uint8_t *data, size_t data_size);
    // Number of bytes that can be peek before the end of the ring buffer
    size_t AvailableBytesToPeek() const;
    // The raw next byte in the ring buffer. Need to check AvailableBytesToPeek() > 0 before calling!
    uint8_t Peek();
    // The unstuffed next byte in the buffer, or nullopt if we have reached the end of the buffer.
    std::optional<uint8_t> PeekUnstuff();
    // Advance the readIndex (the index of the start of the packet) by amount
    void AdvanceReadIndex(size_t amount);
};

#endif // UART_DRIVER_H