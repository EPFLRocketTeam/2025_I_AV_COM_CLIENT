#ifndef ARDUINO // Compiles only in a non-Arduino environment

#include "CM4UART.h"
#include "quill/Quill.h" // For Logger
#include <cstring>       // For memset
#include <fcntl.h>       // For open
#include <stdexcept>     // For runtime_error
#include <termios.h>     // Terminal I/O
#include <unistd.h>      // For read, write, close

CM4UART::CM4UART(const int baudrate, const char *device, quill::Logger *logger) : UART(),
                                                                                  baudrate(baudrate),
                                                                                  device(device)
{
}

CM4UART::~CM4UART()
{
    close(uart_fd);
}

bool CM4UART::Begin()
{
    // Open UART device, in read-write, non-blocking mode.
    // NOCTTY means that the device is not the controlling terminal for the process.
    uart_fd = open(device, O_RDWR | O_NOCTTY | O_NONBLOCK);
    if (uart_fd < 0)
    {
        Log(LOG_LEVEL::ERROR, "Failed to open UART device " + device));
        return false;
    }

    struct termios tty;
    memset(&tty, 0, sizeof tty);

    if (tcgetattr(uart_fd, &tty) != 0)
    {
        close(uart_fd);
        Log(LOG_LEVEL::ERROR, "Failed to get UART attributes.");
        return false;
    }

    // Set baud rate, input/output speed
    cfsetospeed(&tty, baudrate);
    cfsetispeed(&tty, baudrate);

    // Configure 8N1 (8 data bits, no parity, 1 stop bit)
    tty.c_cflag = (tty.c_cflag & ~CSIZE) | CS8; // 8-bit characters
    tty.c_cflag |= CLOCAL | CREAD;              // Ignore modem controls, enable reading
    tty.c_cflag &= ~(PARENB | PARODD);          // No parity
    tty.c_cflag &= ~CSTOPB;                     // 1 stop bit
    tty.c_cflag &= ~CRTSCTS;                    // No hardware flow control

    // Raw input/output mode
    tty.c_lflag = 0;
    tty.c_oflag = 0;
    tty.c_iflag = 0;

    if (tcsetattr(uart_fd, TCSANOW, &tty) != 0)
    {
        close(uart_fd);
        Log(LOG_LEVEL::ERROR, "Failed to set UART attributes.");
        return false;
    }

    Log(LOG_LEVEL::INFO, "UART set up successfully");
}

size_t CM4UART::Send(const unsigned char *data, const size_t data_size)
{
    ssize_t bytes_written = write(uart_fd, data, data_size);
    if (bytes_written == -1)
    {
        // This error means that the device is busy
        if (errno == EAGAIN)
        {
            bytes_written = 0;
        }
        else
        {
            // throw std::runtime_error("Failed to send data");
            Log(LOG_LEVEL::ERROR, "Failed to send data");
        }
    }
    return bytes_written;
}

size_t CM4UART::Receive(unsigned char *data, const size_t data_size)
{
    ssize_t bytes_read = read(uart_fd, data, data_size);
    if (bytes_read == -1)
    {
        // This error means that the device is busy or the read was interrupted
        if (errno == EAGAIN or errno == EWOULDBLOCK or errno == EINTR)
        {
            bytes_read = 0;
        }
        else
        {
            // throw std::runtime_error("Failed to receive data");
            Log(LOG_LEVEL::ERROR, "Failed to receive data");
        }
    }
    return bytes_read;
}

void CM4UART::Log(LOG_LEVEL level, std::string message)
{
    switch (level)
    {
    case LOG_LEVEL::DEBUG:
        LOG_DEBUG(logger, "", message);
        break;
    case LOG_LEVEL::INFO:
        LOG_INFO(logger, "", message);
        break;
    case LOG_LEVEL::WARNING:
        LOG_WARNING(logger, "", message);
        break;
    case LOG_LEVEL::ERROR:
        LOG_ERROR(logger, "", message);
        break;
    default:
        LOG_INFO(logger, "", message);
        break;
    }
}

#endif // ARDUINO