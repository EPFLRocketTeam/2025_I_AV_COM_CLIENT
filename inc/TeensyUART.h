#ifdef ARDUINO  // Compiles only in an Arduino environment
#ifndef TEENSY_UART_H
#define TEENSY_UART_H

#include "Arduino.h"
#include <string>

class TeensyUART : public UART
{
public:
    TeensyUART(HardwareSerial &serial, long baudrate);
    ~TeensyUART() = default;
    bool Begin() override;

private:
    size_t Send(const unsigned char *data, const size_t data_size) override;
    size_t Receive(unsigned char *data, const size_t data_size) override;
    void Log(LOG_LEVEL level, std::string message) override;

    HardwareSerial &serial;
    int baudrate;
};

#endif // TEENSY_UART_H
#endif // ARDUINO