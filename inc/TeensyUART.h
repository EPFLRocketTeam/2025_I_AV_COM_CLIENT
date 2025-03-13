#ifdef ARDUINO  // Compiles only in an Arduino environment
#ifndef TEENSY_UART_H
#define TEENSY_UART_H

#include "Arduino.h"

// TODO: Maybe consider having CM4UART/TeensyUART be a component of UART instead of a subclass
class TeensyUART : public UART
{
public:
    TeensyUART(HardwareSerial &serial, long baudrate);
    ~TeensyUART() = default;

private:
    size_t Send(const unsigned char *data, const size_t data_size) override;
    size_t Receive(unsigned char *data, const size_t data_size) override;
    void Log(LOG_LEVEL level, const char *message) override;

    HardwareSerial &serial;
};

#endif // TEENSY_UART_H
#endif // ARDUINO