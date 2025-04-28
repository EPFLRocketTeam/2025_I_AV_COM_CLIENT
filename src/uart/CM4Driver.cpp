#ifndef ARDUINO // Only compile on CM4

#include "uart/CM4Driver.h"

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

    // Set baud rate
    speed_t baud;
    switch (baudRate)
    {
    case 9600:
        baud = B9600;
        break;
    case 19200:
        baud = B19200;
        break;
    case 38400:
        baud = B38400;
        break;
    case 57600:
        baud = B57600;
        break;
    case 115200:
        baud = B115200;
        break;
    case 230400:
        baud = B230400;
        break;
    default:
        std::cerr << "Unsupported baud rate: " << baudRate << std::endl;
        return false;
    }

    cfsetispeed(&options, baud);
    cfsetospeed(&options, baud);

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

bool CM4UARTDriver::ReadUntilPacketOrNoData(Payload &payload)
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
        if (bytesRead == -1 && (errno == EAGAIN || errno == EWOULDBLOCK))
        {
            // No more data available for now, but we did not find a packet :(
            std::cerr << "Failed to find packet in received UART data." << std::endl;
            return false;
        }
        else if (bytesRead < 0)
        {
            // Error or EOF
            std::cerr << "Error reading UART" << std::endl;
            return false;
        }

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
}

bool CM4UARTDriver::WaitForAndReadPacket(Payload &payload)
{

    if (!isInitialized)
    {
        std::cerr << "UART not initialized" << std::endl;
        return false;
    }

    while (true)
    {
        // Wait for data to become available using epoll
        struct epoll_event events[1];
        int eventCount = epoll_wait(epollFd, events, 1, -1); // -1 means wait indefinitely

        if (eventCount < 0)
        {
            std::cerr << "epoll_wait() error: " << strerror(errno) << std::endl;
            continue;
        }

        if ((eventCount > 0) && (events[0].events & EPOLLIN))
        {
            if (ReadUntilPacketOrNoData(payload))
            {
                return true;
            }
            else
            {
                continue;
            }
        }
    }
}

bool CM4UARTDriver::WritePacketOrTimeout(int timeout, Payload &payload)
{
    if (!isInitialized)
    {
        std::cerr << "UART not initialized" << std::endl;
        return false;
    }

    // Encode the packet
    if (!EncodePacket(payload))
    {
        std::cerr << "Failed to encode packet" << std::endl;
        return false;
    }

    // Send the packet or timeout trying
    size_t total_written = 0;
    auto start_time = std::chrono::steady_clock::now();
    while (total_written < sendBufferIndex)
    {
        auto now = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - start_time).count();
        int remaining_timeout = timeout - static_cast<int>(elapsed);
        if (remaining_timeout <= 0)
        {
            std::cerr << "Timed out trying to write the packet to UART" << std::endl;
            return false;
        }

        struct pollfd pfd = {uartFd, POLLOUT, 0};
        int poll_res = poll(&pfd, 1, remaining_timeout);
        if (poll_res < 0)
        {
            std::cerr << "Error polling UART for writing packet" << std::endl;
            return false;
        }
        else if (poll_res == 0)
        {
            std::cerr << "Timed out trying to write the packet to UART" << std::endl;
            return false;
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
                std::cerr << "Error writing packet to UART device" << std::endl;
                return false;
            }
        }
        total_written += bytesWritten;
    }
    return true;
}

#endif // ARDUINO