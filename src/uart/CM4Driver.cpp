#ifndef ARDUINO // Only compile on CM4

#include "CM4Driver.h"

#include <fcntl.h>
#include <termios.h>
#include <unistd.h>
#include <sys/epoll.h>
#include <cstring>
#include <iostream>
#include <poll.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <chrono>
#include <stdexcept>

CM4UARTDriver::CM4UARTDriver(const std::string &device, int baud)
    : UARTDriver(),
      devicePath(device),
      baudRate(baud),
      uartFd(-1),
      epollFd(-1),
      isInitialized(false)
{
}

CM4UARTDriver::~CM4UARTDriver()
{
    if (uartFd >= 0)
    {
        close(uartFd);
    }

    if (epollFd >= 0)
    {
        close(epollFd);
    }
}

bool CM4UARTDriver::Begin()
{
    // Open the UART device
    uartFd = open(devicePath.c_str(), O_RDWR | O_NOCTTY | O_NONBLOCK);
    if (uartFd < 0)
    {
        std::cerr << "Failed to open UART device: " << devicePath << std::endl;
        return false;
    }

    // Configure the UART port
    if (!ConfigureUART())
    {
        close(uartFd);
        uartFd = -1;
        return false;
    }

    // Set up epoll for the UART fd
    if (!SetupEpoll())
    {
        close(uartFd);
        uartFd = -1;
        return false;
    }

    isInitialized = true;
    return true;
}

bool CM4UARTDriver::ConfigureUART()
{
    struct termios options;

    // Get the current terminal attributes
    if (tcgetattr(uartFd, &options) < 0)
    {
        std::cerr << "Failed to get terminal attributes" << std::endl;
        return false;
    }

    // Set the baud rate
    cfsetispeed(&options, baudRate);
    cfsetospeed(&options, baudRate);

    // Configure 8N1 (8 data bits, no parity, 1 stop bit)
    options.c_cflag = (options.c_cflag & ~CSIZE) | CS8; // 8-bit characters
    options.c_cflag |= CLOCAL | CREAD;                  // Ignore modem controls, enable reading
    options.c_cflag &= ~(PARENB | PARODD);              // No parity
    options.c_cflag &= ~CSTOPB;                         // 1 stop bit
    options.c_cflag &= ~CRTSCTS;                        // No hardware flow control

    // Raw input/output mode
    options.c_lflag = 0;
    options.c_oflag = 0;
    options.c_iflag = 0;

    // Set the new attributes
    if (tcsetattr(uartFd, TCSANOW, &options) < 0)
    {
        std::cerr << "Failed to set terminal attributes" << std::endl;
        return false;
    }

    return true;
}

// Update your setup function to create both epoll instances
bool CM4UARTDriver::SetupEpoll()
{
    // Setup for reading
    epollFdRead = epoll_create1(0);
    if (epollFdRead < 0)
    {
        std::cerr << "Failed to create read epoll instance" << std::endl;
        return false;
    }

    // Setup for writing
    epollFdWrite = epoll_create1(0);
    if (epollFdWrite < 0)
    {
        std::cerr << "Failed to create write epoll instance" << std::endl;
        close(epollFdRead);
        epollFdRead = -1;
        return false;
    }

    // Configure read epoll
    struct epoll_event eventRead;
    eventRead.events = EPOLLIN;
    eventRead.data.fd = uartFd;
    if (epoll_ctl(epollFdRead, EPOLL_CTL_ADD, uartFd, &eventRead) < 0)
    {
        std::cerr << "Failed to add UART fd to read epoll" << std::endl;
        close(epollFdRead);
        close(epollFdWrite);
        epollFdRead = -1;
        epollFdWrite = -1;
        return false;
    }

    // Configure write epoll
    struct epoll_event eventWrite;
    eventWrite.events = EPOLLOUT;
    eventWrite.data.fd = uartFd;
    if (epoll_ctl(epollFdWrite, EPOLL_CTL_ADD, uartFd, &eventWrite) < 0)
    {
        std::cerr << "Failed to add UART fd to write epoll" << std::endl;
        close(epollFdRead);
        close(epollFdWrite);
        epollFdRead = -1;
        epollFdWrite = -1;
        return false;
    }

    return true;
}

Payload CM4UARTDriver::ReadPacket()
{
    // We reset the circular receive buffer
    receiveBufferWriteIndex = 0;
    receiveBufferReadIndex = 0;

    // This loop continues until either:
    // - We have found a packet (returns true)
    // - We found no packet and there is no more data available (returns false)
    while (true)
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
        ssize_t bytesRead = read(uartFd, receiverBuffer + receiveBufferWriteIndex, continuousSpace);

        // Error handling
        if (bytesRead < 0)
        {
            if (errno == EAGAIN || errno == EWOULDBLOCK)
            {
                // No more data available for now, but we did not find a packet :(
                throw std::runtime_error("No packet found in received data");
            }
            else
            {
                // Error or EOF
                throw std::runtime_error("Error reading from UART device");
            }
        }

        // Update write index
        receiveBufferWriteIndex = (receiveBufferWriteIndex + bytesRead) % RECEIVE_BUFFER_SIZE;

        // Check if we have found a packet
        Payload payload;
        if (DecodePacket(payload))
        {
            return payload;
        }

        // Check for buffer overflow
        if (receiveBufferReadIndex == receiveBufferWriteIndex)
        {
            std::cerr << "UART Receive buffer overflow. Data will be lost" << std::endl;
        }
    }
}

bool CM4UARTDriver::WaitForData(int timeoutMs)
{
    if (!isInitialized)
    {
        throw std::runtime_error("UART not initialized");
    }

    // Wait for data to become available using epoll
    struct epoll_event events[1];
    int eventCount = epoll_wait(epollFd, events, 1, timeoutMs);

    if (eventCount < 0)
    {
        throw std::runtime_error("epoll_wait() failed");
    }

    if (eventCount == 0)
    {
        // Timeout occurred
        return false;
    }

    if (events[0].events & EPOLLIN)
    {
        // Data is available to read
        return true;
    }
    else
    {
        // Unexpected event
        throw std::runtime_error("Unexpected event on UART");
    }
}

void CM4UARTDriver::WritePacketOrTimeout(int timeoutMs, Payload &payload)
{
    if (!isInitialized)
    {
        throw std::runtime_error("UART not initialized");
    }

    // Encode the packet
    EncodePacket(payload);
    
    // Start time for timeout calculation
    struct timespec startTime;
    clock_gettime(CLOCK_MONOTONIC, &startTime);

    // Track how much data we've sent so far
    size_t bytesSent = 0;

    // Continue until all data is sent or timeout/error occurs
    while (bytesSent < sendBufferIndex)
    {
        // Calculate remaining time for timeout
        struct timespec currentTime;
        clock_gettime(CLOCK_MONOTONIC, &currentTime);
        int elapsedMs = (currentTime.tv_sec - startTime.tv_sec) * 1000 +
                        (currentTime.tv_nsec - startTime.tv_nsec) / 1000000;
        int remainingMs = timeoutMs - elapsedMs;

        // Check if we've timed out
        if (remainingMs <= 0)
        {
            throw std::runtime_error("Timeout while writing to UART");
        }

        // Wait for the UART to be ready for writing
        struct epoll_event events[1];
        int numEvents = epoll_wait(epollFdWrite, events, 1, remainingMs);

        // Check epoll results
        if (numEvents < 0)
        {
            // Error in epoll_wait
            if (errno == EINTR)
            {
                // Interrupted by signal, continue trying
                continue;
            }
            throw std::runtime_error("Error waiting for UART write readiness");
        }
        else if (numEvents == 0)
        {
            // Timeout occurred in epoll_wait
            continue;
        }

        // UART is ready for writing, attempt to write remaining data
        ssize_t bytesWritten = write(uartFd, sendBuffer + bytesSent, sendBufferIndex - bytesSent);

        if (bytesWritten < 0)
        {
            // Error handling
            if (errno == EAGAIN || errno == EWOULDBLOCK)
            {
                // Device is not ready yet, continue trying
                continue;
            }
            else
            {
                // Other error
                throw std::runtime_error("Error writing to UART device");
            }
        }
        else if (bytesWritten == 0)
        {
            // No data written, but no error reported - unusual for write()
            throw std::runtime_error("Zero bytes written to UART");
        }
        else
        {
            // Update progress
            bytesSent += bytesWritten;
        }
    }

    // Reset buffer index after successful write
    sendBufferIndex = 0;
}

#endif // ARDUINO