#ifndef ARDUINO // Compiles only in a non-Arduino environment
#ifndef CM4_UART_H
#define CM4_UART_H

#include "UART.h"
#include "quill/Quill.h" // For Logger

class CM4UART : public UART
{
  public:
    CM4UART(const int baudrate, const char *device, quill::Logger *logger);
    ~CM4UART();

  private:
    int baudrate;
    const char *device;
    quill::Logger *logger;
    int uart_fd;

    bool Begin() override;
    size_t Send(const unsigned char *data, const size_t data_size) override;
    size_t Receive(unsigned char *data, const size_t data_size) override;
    void Log(LOG_LEVEL level, const char *message) override;
};

#endif // CM4_UART_H
#endif // ARDUINO