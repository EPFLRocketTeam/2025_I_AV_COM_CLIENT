#ifdef ARDUINO // Only compile on Teensy

#include "uart/TeensyDriver.h"

TeensyDriver::TeensyDriver(HardwareSerial &serial, int baudrate) : serial(serial),
                                                                   baudRate(baudrate)
{
}

bool TeensyDriver::Begin()
{
    serial.begin(baudRate);
    return true;
}

bool TeensyDriver::ReadUntilPacketOrTimeout(int timeout, Payload &payload)
{
    // We set the timeout to 1ms to avoid blocking in readBytes
    serial.setTimeout(1);

    // We reset the circular receive buffer
    receiveBufferWriteIndex = 0;
    receiveBufferReadIndex = 0;

    unsigned long startTime = millis();

    // This loop continues until either:
    // - We have found a packet (returns true)
    // - We found no packet and there is no time left (returns false)
    while (millis() - startTime < timeout)
    {
        // Read continuous chunk of data in circular buffer (no overflow possible)
        int continuousSpace;
        if (receiveBufferReadIndex <= receiveBufferWriteIndex)
        {
            // Normal case, there is free space until the end of the buffer.
            continuousSpace = RECEIVE_BUFFER_SIZE - receiveBufferWriteIndex;
        }
        else
        {
            // Wrap around, there is only space in between the two indexes.
            continuousSpace = receiveBufferReadIndex - receiveBufferWriteIndex;
        }
        int readable = min(continuousSpace, serial.available());
        size_t bytesRead = serial.readBytes(receiverBuffer + receiveBufferWriteIndex, readable);

        // Update write index
        receiveBufferWriteIndex = (receiveBufferWriteIndex + bytesRead) % RECEIVE_BUFFER_SIZE;

        // Check if we have found a packet
        if (DecodePacket(payload))
        {
            return true;
        }

        // Check for buffer overflow
        if (receiveBufferReadIndex == receiveBufferWriteIndex)
        {
            std::cerr << "UART Receive buffer overflow. Data will be lost" << std::endl;
        }
    }
    return false;
}

// TODO
// Tries to write a packet from UART in timeout time.
// Retuns false if the packet could not be written in time.
// bool WriteOrTimeout(int timeout, Payload &payload)
// {
//     bool success = parser.EncodePacket(data, payload);
//     if (!success)
//     {
//         return false;
//     }

//     serial.setTimeout(timeout);

//     size_t bytesWritten = serial.writeBytes(data);
//     if (bytesWritten != dataSize)
//     {
//         return false;
//     }
//     else
//     {
//         return true;
//     }
// }

#endif // ARDUINO