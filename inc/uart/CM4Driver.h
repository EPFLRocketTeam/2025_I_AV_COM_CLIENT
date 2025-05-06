#ifndef ARDUINO // Only compile on CM4

#ifndef CM4_DRIVER_H
#define CM4_DRIVER_H

#include "UARTDriver.h"
#include <string>
#include <sys/epoll.h>

class CM4UARTDriver : public UARTDriver
{
  public:
    /**
     * Constructor for CM4UARTDriver
     * @param device The path to the UART device (e.g., "/dev/ttyS0")
     * @param baudRate The baud rate for the UART connection
     */
    CM4UARTDriver(const std::string &device, int baudRate);

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
     * Waits for data to arrive on the UART device
     * @param timeoutMS The timeout in milliseconds
     * @throws std::runtime_error in case of error
     * @return true if data was received, false in case of timeout
     */
    bool WaitForData(int timeoutMS);

    /**
     * Reads all of the available data from the UART port and tries to find a packet
     * @throws std::runtime_error in case of error
     * @return the payload read from the UART port
     */
    Payload ReadPacket();

    /**
     * Tries to sends a packet over the UART connection
     * @param timeout The timeout in milliseconds
     * @param payload The Payload to send
     * @throws std::runtime_error in case of error
     */
    void WritePacketOrTimeout(int timeout, Payload &payload);

  private:
    std::string devicePath;
    int baudRate;
    int uartFd;  // UART file descriptor
    // epoll file descriptor
    int epollFdRead; // For reading
    int epollFdWrite; // For writing
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
};

#endif // CM4_DRIVER_H
#endif // ARDUINO