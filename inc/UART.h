#ifndef UART_H
#define UART_H

#ifndef ARDUINO
#include "Payload.h"
#endif // ARDUINO

#include <cstddef> // For size_t
#include <cstdint> // For uint8_t
#include <functional>
#include <optional>
#include <unordered_map>
#include <string>

constexpr uint8_t START_BYTE = 0x7E;
constexpr uint8_t END_BYTE = 0x7F;
constexpr uint8_t ESCAPE_BYTE = 0x7D;
constexpr uint8_t ESCAPE_MASK = 0x20;

// TODO: Since the biggest packets are around 128 bytes, we should update these values
constexpr size_t MAX_PAYLOAD_SIZE = 256;
constexpr size_t MAX_PACKET_SIZE_STUFFED = (MAX_PAYLOAD_SIZE + 3) * 2 + 2;
constexpr size_t MAX_PACKET_SIZE_UNSTUFFED = MAX_PAYLOAD_SIZE + 5;
constexpr size_t RECEIVE_BUFFER_SIZE = 1024;
constexpr size_t SEND_BUFFER_SIZE = 1024;
constexpr size_t RING_BUFFER_SIZE = 2048;

class UART
{
  public:
    UART();
    ~UART() = default;
    
    // Sets up the UART connextion
    virtual bool Begin() = 0;
    
    // Sends the packet over UART.
    // Returns true if the packet was sent successfully, false in case of error.
    bool SendPacket(Payload &payload);

    // Tries to read a packet from the UART device.
    // Returns true if a packet was read successfully, false otherwise in case of error.
    // If no packet is available, it returns true and the payload is empty.
    bool ReceivePacket(Payload &payload);

  protected:  
    // These methods are specific to the UART implementation.
    // They need to be implemented by the derived classes for each platform.

    // Tries to write the data to the UART device, without blocking.
    // Returns the number of bytes written.
    virtual size_t Send(const uint8_t *data, const size_t data_size) = 0;

    // Tries to read data_size bytes into *data from the UART device, without blocking.
    // Returns the number of bytes read.
    virtual size_t Receive(uint8_t *data, const size_t data_size) = 0;

    enum class LOG_LEVEL
    {
        DEBUG,
        INFO,
        WARNING,
        ERROR,
    };

    // Log a message with the specified log level.
    virtual void Log(LOG_LEVEL level, std::string message) = 0;

  private:
    uint8_t circularBuffer[RING_BUFFER_SIZE];
    size_t readIndex;  // Where we're currently reading from
    size_t writeIndex; // Where new data gets written
    size_t peekIndex;  // Where to peek next in the ring buffer

    uint8_t sendBuffer[SEND_BUFFER_SIZE];
    size_t sendBufferStart;
    size_t sendBufferEnd;

    // Calculate the available space in the send buffer
    size_t AvailableSendBufferSpace() const;
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
    // Advance the ReadIndex by 1
    bool DiscardCurrentByteAndContinue();
    // Packet parsing method
    bool TryParsePacket();
};

#endif // UART_H