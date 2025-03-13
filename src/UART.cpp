#ifndef ARDUINO
#include "UART.h"
#include "Payload.h"
#endif // ARDUINO

#include <cstring>
#include <stdexcept>

UART::UART()
    : readIndex(0),
      writeIndex(0),
      peekIndex(0),
      sendBufferStart(0),
      sendBufferEnd(0)
{
}

void UART::RegisterHandler(int packetId, std::function<void(Payload &)> handler)
{
    handlers[packetId] = handler;
}

bool UART::SendUARTPacket(const uint8_t id, Payload &payload)
{
    uint8_t packetBuffer[MAX_PACKET_SIZE_UNSTUFFED];
    size_t packetBufferIndex = 0;

    // 1. Start byte
    packetBuffer[packetBufferIndex++] = START_BYTE;

    // 2. Packet ID
    packetBuffer[packetBufferIndex++] = id;

    // 3. Length
    packetBuffer[packetBufferIndex++] = payload.GetSize();

    // 4. Payload
    const uint8_t *payloadData = payload.GetBytes();
    for (size_t i = 0; i < payload.GetSize(); i++)
    {
        packetBuffer[packetBufferIndex++] = payloadData[i];
    }

    // 5. Checksum
    uint8_t checksum = ComputeChecksum(packetBuffer + 1, packetBufferIndex - 1); // Exclude start byte
    packetBuffer[packetBufferIndex++] = checksum;

    // 6. End byte
    packetBuffer[packetBufferIndex++] = END_BYTE;

    // Stuff the packet
    uint8_t stuffedBuffer[MAX_PACKET_SIZE_STUFFED];
    size_t stuffedBufferIndex = 0;

    // 1. Copy the START_BYTE as is
    stuffedBuffer[stuffedBufferIndex++] = packetBuffer[0]; // START_BYTE

    // 2. Stuff the middle portion (ID, length, payload, checksum)
    for (size_t i = 1; i < packetBufferIndex - 1; i++) // Skip first and last bytes
    {
        if (packetBuffer[i] == START_BYTE || packetBuffer[i] == END_BYTE || packetBuffer[i] == ESCAPE_BYTE)
        {
            stuffedBuffer[stuffedBufferIndex++] = ESCAPE_BYTE;
            stuffedBuffer[stuffedBufferIndex++] = packetBuffer[i] ^ ESCAPE_MASK;
        }
        else
        {
            stuffedBuffer[stuffedBufferIndex++] = packetBuffer[i];
        }
    }

    // 3. Copy the END_BYTE as is
    stuffedBuffer[stuffedBufferIndex++] = packetBuffer[packetBufferIndex - 1]; // END_BYTE

    // Add the stuffed packet to the send buffer
    if (AvailableSendBufferSpace() < stuffedBufferIndex)
    {
        return false;
    }

    for (size_t i = 0; i < stuffedBufferIndex; i++)
    {
        sendBuffer[sendBufferEnd] = stuffedBuffer[i];
        sendBufferEnd = (sendBufferEnd + 1) % SEND_BUFFER_SIZE;
    }

    return true;
}

size_t UART::AvailableBytesToPeek() const
{
    return (writeIndex - (readIndex + peekIndex) + RING_BUFFER_SIZE) % RING_BUFFER_SIZE;
}

uint8_t UART::Peek()
{
    uint8_t byte = circularBuffer[(readIndex + peekIndex) % RING_BUFFER_SIZE];
    peekIndex = (peekIndex + 1) % RING_BUFFER_SIZE;
    return byte;
}

void UART::AdvanceReadIndex(size_t amount)
{
    readIndex = (readIndex + amount) % RING_BUFFER_SIZE;
}

std::optional<uint8_t> UART::PeekUnstuff()
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

bool UART::DiscardCurrentByteAndContinue()
{
    AdvanceReadIndex(1);
    return true;
}

bool UART::TryParsePacket()
{
    peekIndex = 0;

    uint8_t packetBuffer[MAX_PACKET_SIZE_UNSTUFFED];
    size_t packetBufferIndex = 0;

    // 1. Read start byte
    if (AvailableBytesToPeek() < 1)
        return false;

    uint8_t start = Peek();
    if (start != START_BYTE)
        return DiscardCurrentByteAndContinue();

    packetBuffer[packetBufferIndex++] = start;

    // 2. Read packet ID
    auto maybeId = PeekUnstuff();
    if (!maybeId.has_value())
        return false;

    uint8_t id = maybeId.value();

    // Check if ID is valid
    if (handlers.find(id) == handlers.end())
    {
        Log(LOG_LEVEL::WARNING, "Invalid packet ID received");
        return DiscardCurrentByteAndContinue();
    }

    packetBuffer[packetBufferIndex++] = id;

    // 3. Read length
    auto maybeLength = PeekUnstuff();
    if (!maybeLength.has_value())
        return false;

    uint8_t length = maybeLength.value();

    // Check if length is valid
    if (length > MAX_PAYLOAD_SIZE)
    {
        Log(LOG_LEVEL::WARNING, "Invalid packet length received");
        return DiscardCurrentByteAndContinue();
    }

    packetBuffer[packetBufferIndex++] = length;

    // 4. Read payload
    for (size_t i = 0; i < length; i++)
    {
        auto maybeByte = PeekUnstuff();
        if (!maybeByte.has_value())
            return false;
        packetBuffer[packetBufferIndex++] = maybeByte.value();
    }

    // 5. Read checksum
    auto maybeChecksum = PeekUnstuff();
    if (!maybeChecksum.has_value())
        return false;

    uint8_t checksum = maybeChecksum.value();

    // Verify checksum (excluding start byte)
    if (ComputeChecksum(packetBuffer + 1, packetBufferIndex - 1) != checksum)
    {
        Log(LOG_LEVEL::WARNING, "Invalid checksum received");
        // std::cout << "received: " << std::hex << std::setfill('0') << std::setw(2) << (int)checksum << std::endl;
        // std::cout << "computed: " << std::hex << std::setfill('0') << std::setw(2) << (int)ComputeChecksum(packetBuffer + 1, packetBufferIndex - 1) << std::endl;
        // std::cout << std::dec;
        return DiscardCurrentByteAndContinue();
    }

    packetBuffer[packetBufferIndex++] = checksum;

    // 6. Read end byte
    if (AvailableBytesToPeek() < 1)
        return false;

    uint8_t end = Peek();
    if (end != END_BYTE)
        return DiscardCurrentByteAndContinue();

    packetBuffer[packetBufferIndex++] = end;

    // 7. Process valid packet
    Payload payload;
    if (!payload.SetBytes(packetBuffer + 3, packetBufferIndex - 5)) // Exclude start, id, length, checksum, end
    {
        Log(LOG_LEVEL::ERROR, "Failed to initialize payload, size exceeds limit.");
        return DiscardCurrentByteAndContinue();
    }
    auto handler = handlers.find(id);
    handler->second(payload);

    // Advance past this packet
    AdvanceReadIndex(peekIndex);
    packetsRead++;
    return true;
}

void UART::SendUARTPackets()
{
    // Send data from the send buffer
    if (sendBufferStart != sendBufferEnd)
    {
        size_t bytesToSend = (sendBufferEnd >= sendBufferStart) ? (sendBufferEnd - sendBufferStart) : (SEND_BUFFER_SIZE - sendBufferStart);
        // std::cout << "Sending " << bytesToSend << " bytes" << std::endl;
        size_t bytesSent = Send(sendBuffer + sendBufferStart, bytesToSend);

        sendBufferStart = (sendBufferStart + bytesSent) % SEND_BUFFER_SIZE;
    }
}

size_t UART::AvailableSendBufferSpace() const
{
    if (sendBufferEnd >= sendBufferStart)
    {
        return SEND_BUFFER_SIZE - (sendBufferEnd - sendBufferStart) - 1;
    }
    else
    {
        return sendBufferStart - sendBufferEnd - 1;
    }
}

uint8_t UART::ComputeChecksum(const uint8_t *data, size_t data_size)
{
    uint8_t checksum = 0;
    for (size_t i = 0; i < data_size; ++i)
    {
        checksum += data[i];
    }
    return checksum; // This will give the sum modulo 256, as it's a uint8_t
}

int UART::ReceiveUARTPackets()
{
    // Log(LOG_LEVEL::DEBUG, "Receiving UART packets");

    packetsRead = 0;

    // Receive new data into circular buffer
    uint8_t tempBuffer[RECEIVE_BUFFER_SIZE];
    size_t bytesReceived = Receive(tempBuffer, RECEIVE_BUFFER_SIZE);

    // Check if we might have filled the receive buffer completely
    if (bytesReceived == RECEIVE_BUFFER_SIZE)
    {
        Log(LOG_LEVEL::WARNING, "Receive buffer filled completely, might have lost data");
    }

    // Copy received data to circular buffer
    // We cannot unstuff bytes here, because it would cause errors if an ESCAPE_BYTE appears at the end of the buffer :(
    for (size_t i = 0; i < bytesReceived; i++)
    {
        circularBuffer[writeIndex] = tempBuffer[i];
        writeIndex = (writeIndex + 1) % RING_BUFFER_SIZE;
    }

    // Try to parse packets until no more can be parsed
    while (TryParsePacket())
    {
    }

    return packetsRead;
}