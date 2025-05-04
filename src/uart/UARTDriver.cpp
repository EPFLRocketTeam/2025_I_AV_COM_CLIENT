#include "UARTDriver.h"

// TODO: Better logging with a dedicated log method for each implementation

UARTDriver::UARTDriver()
    : receiveBufferReadIndex(0),
      receiveBufferWriteIndex(0),
      receiveBufferPeekIndex(0)
{
}

bool UARTDriver::DecodePacket(Payload &payload)
{
    while (true)
    {
        receiveBufferPeekIndex = 0;

        uint8_t unstuffedPacketBuffer[MAX_PACKET_SIZE_UNSTUFFED];
        size_t unstuffedPacketBufferIndex = 0;

        // 1. Read start byte
        if (AvailableBytesToPeek() < 1)
            return false; // Not enough bytes to decode a packet
        uint8_t start = Peek();
        if (start != START_BYTE)
        {
            AdvanceReadIndex(1); // This byte is noise, we can skip it
            continue;            // We continue to parse from the next byte
        }
        unstuffedPacketBuffer[unstuffedPacketBufferIndex++] = start;

        // 2. Read length
        auto maybeLength = PeekUnstuff();
        if (!maybeLength.has_value())
            return false; // Not enough bytes to decode a packet
        uint8_t length = maybeLength.value();
        // Check if length is valid
        if (length > MAX_PAYLOAD_SIZE)
        {
            std::cerr << "Invalid packet length received" << std::endl;
            AdvanceReadIndex(1); // This byte is noise, we can skip it
            continue;            // We continue to parse from the next byte
        }
        unstuffedPacketBuffer[unstuffedPacketBufferIndex++] = length;

        // 3. Read payload
        for (size_t i = 0; i < length; i++)
        {
            auto maybeByte = PeekUnstuff();
            if (!maybeByte.has_value())
                return false; // Not enough bytes to read the payload
            unstuffedPacketBuffer[unstuffedPacketBufferIndex++] = maybeByte.value();
        }

        // 4. Read checksum
        auto maybeChecksum = PeekUnstuff();
        if (!maybeChecksum.has_value())
            return false; // Not enough bytes to read checksum
        uint8_t checksum = maybeChecksum.value();

        // Verify checksum (excluding start byte)
        if (ComputeChecksum(unstuffedPacketBuffer + 1, unstuffedPacketBufferIndex - 1) != checksum)
        {
            std::cerr << "Invalid checksum received" << std::endl;
            AdvanceReadIndex(1); // This byte is noise, we can skip it
            continue;            // We continue to parse from the next byte
        }
        unstuffedPacketBuffer[unstuffedPacketBufferIndex++] = checksum;

        // 5. Read end byte
        if (AvailableBytesToPeek() < 1)
            return false; // No bytes left so we can't read the end byte :(
        uint8_t end = Peek();

        // Verify that it is indeed the end byte
        if (end != END_BYTE)
        {
            std::cerr << "Unterminated packet received" << std::endl;
            AdvanceReadIndex(1); // This byte is noise, we can skip it
            continue;            // We continue to parse from the next byte
        }
        unstuffedPacketBuffer[unstuffedPacketBufferIndex++] = end;

        // 6. Process valid packet
        payload.write(unstuffedPacketBuffer + 2, unstuffedPacketBufferIndex - 4);
        if (payload.hasOverflow())
        {
            std::cerr << "Failed to initialize payload, size exceeds limit." << std::endl;
            payload.clear();
            AdvanceReadIndex(1); // This byte is noise, we can skip it
            continue;            // We continue to parse from the next byte
        }

        // We clear the buffer of all of its data
        receiveBufferReadIndex = receiveBufferWriteIndex;
        return true;
    }
}

void UARTDriver::EncodePacket(Payload &payload)
{
    uint8_t unstuffedPacketBuffer[MAX_PACKET_SIZE_UNSTUFFED];
    size_t unstuffedPacketBufferIndex = 0;

    // 1. Start byte
    unstuffedPacketBuffer[unstuffedPacketBufferIndex++] = START_BYTE;

    // 2. Length
    unstuffedPacketBuffer[unstuffedPacketBufferIndex++] = payload.size();

    // 3. Payload
    payload.resetRead();
    payload.read(unstuffedPacketBuffer + unstuffedPacketBufferIndex, payload.size());
    unstuffedPacketBufferIndex += payload.size();

    // 4. Checksum
    uint8_t checksum = ComputeChecksum(unstuffedPacketBuffer + 1, unstuffedPacketBufferIndex - 1); // Exclude start byte
    unstuffedPacketBuffer[unstuffedPacketBufferIndex++] = checksum;

    // 5. End byte
    unstuffedPacketBuffer[unstuffedPacketBufferIndex++] = END_BYTE;

    // Stuff the packet
    sendBufferIndex = 0;

    // 1. Copy the START_BYTE as is
    sendBuffer[sendBufferIndex++] = unstuffedPacketBuffer[0]; // START_BYTE

    // 2. Stuff the middle portion (ID, length, payload, checksum)
    for (size_t i = 1; i < unstuffedPacketBufferIndex - 1; i++) // Skip first and last bytes
    {
        if (unstuffedPacketBuffer[i] == START_BYTE || unstuffedPacketBuffer[i] == END_BYTE || unstuffedPacketBuffer[i] == ESCAPE_BYTE)
        {
            sendBuffer[sendBufferIndex++] = ESCAPE_BYTE;
            sendBuffer[sendBufferIndex++] = unstuffedPacketBuffer[i] ^ ESCAPE_MASK;
        }
        else
        {
            sendBuffer[sendBufferIndex++] = unstuffedPacketBuffer[i];
        }
    }

    // 3. Copy the END_BYTE as is
    sendBuffer[sendBufferIndex++] = unstuffedPacketBuffer[unstuffedPacketBufferIndex - 1]; // END_BYTE
}

uint8_t UARTDriver::ComputeChecksum(const uint8_t *data, size_t data_size)
{
    uint8_t checksum = 0;
    for (size_t i = 0; i < data_size; ++i)
    {
        checksum += data[i];
    }
    return checksum; // This will give the sum modulo 256, as it's a uint8_t
}

size_t UARTDriver::AvailableBytesToPeek() const
{
    return (receiveBufferWriteIndex - (receiveBufferReadIndex + receiveBufferPeekIndex) + RECEIVE_BUFFER_SIZE) % RECEIVE_BUFFER_SIZE;
}

uint8_t UARTDriver::Peek()
{
    uint8_t byte = receiverBuffer[(receiveBufferReadIndex + receiveBufferPeekIndex) % RECEIVE_BUFFER_SIZE];
    receiveBufferPeekIndex = (receiveBufferPeekIndex + 1) % RECEIVE_BUFFER_SIZE;
    return byte;
}

void UARTDriver::AdvanceReadIndex(size_t amount)
{
    receiveBufferReadIndex = (receiveBufferReadIndex + amount) % RECEIVE_BUFFER_SIZE;
}

std::optional<uint8_t> UARTDriver::PeekUnstuff()
{
    if (AvailableBytesToPeek() < 1)
        return std::nullopt;

    uint8_t byte = Peek();

    // Handle escape sequence
    if (byte == ESCAPE_BYTE)
    {
        if (AvailableBytesToPeek() < 1)
            return std::nullopt;

        byte = Peek() ^ ESCAPE_MASK;
    }

    return byte;
}