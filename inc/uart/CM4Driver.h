#ifndef ARDUINO // Only compile on CM4

#ifndef CM4_DRIVER_H
#define CM4_DRIVER_H

#include "UARTDriver.h"
#include <string>
#include <sys/epoll.h>

class CM4UARTDriver : public UARTDriver {
public:
    /**
     * Constructor for CM4UARTDriver
     * @param device The path to the UART device (e.g., "/dev/ttyS0")
     * @param baudRate The baud rate for the UART connection
     */
    CM4UARTDriver(const std::string& device, int baudRate);
    
    /**
     * Destructor, closes the file descriptor and epoll instance
     */
    ~CM4UARTDriver();
    
    /**
     * Initializes the UART connection
     * @return true if initialization succeeded, false otherwise
     */
    bool Begin();
    
    /**
     * Waits for a packet to arrive on the UART device, reads it,
     * and decodes it into a Payload object
     * @return The decoded Payload
     */
    bool WaitForAndReadPacket(Payload &payload);
    
    /**
     * Tries to sends a packet over the UART connection
     * @param payload The Payload to send
     * @return true if send was successful, false otherwise
     */
    bool WritePacketOrTimeout(int timeout, Payload& payload);

private:
    std::string devicePath;
    int baudRate;
    int uartFd;        // UART file descriptor
    int epollFd;       // epoll file descriptor
    bool isInitialized;
    
    /**
     * Configures the UART port with the specified settings
     * @return true if configuration succeeded, false otherwise
     */
    bool ConfigureUART();
    
    /**
     * Sets up epoll for monitoring the UART file descriptor
     * @return true if setup succeeded, false otherwise
     */
    bool SetupEpoll();

    /**
     * Reads all of the available data from the UART port and tries to find a packet
     * @return true if a packet was found, false if no packet was found
     */
    bool ReadUntilPacketOrNoData(Payload &payload);
};

#endif // CM4_DRIVER_H
#endif // ARDUINO