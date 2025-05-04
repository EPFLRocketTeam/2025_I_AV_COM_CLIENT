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

bool CM4UARTDriver::SetupEpoll()
{
    epollFd = epoll_create1(0);
    if (epollFd < 0)
    {
        std::cerr << "Failed to create epoll instance" << std::endl;
        return false;
    }

    struct epoll_event event;
    event.events = EPOLLIN;
    event.data.fd = uartFd;

    if (epoll_ctl(epollFd, EPOLL_CTL_ADD, uartFd, &event) < 0)
    {
        std::cerr << "Failed to add UART fd to epoll" << std::endl;
        close(epollFd);
        epollFd = -1;
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

// TODO: Could be improved to re use epoll for writing, or maybe just a simple write and the rest is overkill
void CM4UARTDriver::WritePacketOrTimeout(int timeoutMs, Payload &payload)
{
    if (!isInitialized)
    {
        throw std::runtime_error("UART not initialized");
    }

    // Encode the packet
    EncodePacket(payload);

    // Send the packet or timeout trying
    size_t total_written = 0;
    auto start_time = std::chrono::steady_clock::now();
    while (total_written < sendBufferIndex)
    {
        auto now = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - start_time).count();
        int remaining_timeout = timeoutMs - static_cast<int>(elapsed);
        if (remaining_timeout <= 0)
        {
            throw std::runtime_error("Timeout while writing to UART");
        }

        struct pollfd pfd = {uartFd, POLLOUT, 0};
        int poll_res = poll(&pfd, 1, remaining_timeout);
        if (poll_res < 0)
        {
            throw std::runtime_error("poll() failed");
        }
        else if (poll_res == 0)
        {
            throw std::runtime_error("Timeout while writing to UART");
        }

        // Ready to write
        ssize_t bytesWritten = write(uartFd, sendBuffer + total_written, sendBufferIndex - total_written);
        if (bytesWritten < 0)
        {
            if (errno == EAGAIN || errno == EWOULDBLOCK)
            {
                continue; // try again
            }
            else
            {
                throw std::runtime_error("Error writing to UART device");
            }
        }
        total_written += bytesWritten;
    }
}

#endif // ARDUINO