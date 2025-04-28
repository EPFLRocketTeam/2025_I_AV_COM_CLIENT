#ifdef ARDUINO // Only compile on Teensy

#ifndef TEENSY_DRIVER_H
#define TEENSY_DRIVER_H

#include "UARTDriver.h"

class TeensyDriver : public UARTDriver
{
  public:
    /**
     * Constructor for TeensyDriver
     * @param serial The serial port to use
     * @param baudRate The baud rate for the UART connection
     */
    TeensyDriver(HardwareSerial &serial, int baudrate);

    /**
     * Destructor, we use the default destructor
     */
    ~TeensyDriver() = default;

    /**
     * Initializes the UART connection
     * @return true if initialization succeeded, false otherwise
     */
    bool Begin();

    /**
     * Tries to read a packet from UART in timeout time.
     * @return true if it has read a packet, false if it could not read a packet in the available time.
     */
    bool ReadUntilPacketOrTimeout(int timeout, Payload &payload);

    // TODO
    /**
     * Tries to sends a packet over the UART connection
     * @param payload The Payload to send
     * @return true if send was successful, false otherwise
     */
    // bool WritePacketOrTimeout(int timeout, Payload &payload);

  private:
    HardwareSerial serial;
    int baudRate;
};

#endif // TEENSY_DRIVER_H
#endif // ARDUINO