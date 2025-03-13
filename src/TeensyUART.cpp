#ifdef ARDUINO  // Compiles only in an Arduino environment

#include "Arduino.h"

TeensyUART::TeensyUART(HardwareSerial &serial, long baudrate) : UART(), serial(serial)
{
    serial.begin(baudrate);
}

size_t TeensyUART::Send(const unsigned char *data, const size_t data_size)
{
    size_t writable = min(data_size, serial.availableForWrite());
    return serial.write(data, writable);
}

size_t TeensyUART::Receive(unsigned char *data, const size_t data_size)
{
    size_t readable = min(data_size, serial.available());
    return serial.readBytes(data, readable);
}

void TeensyUART::Log(LOG_LEVEL level, const char *message)
{
    switch (level)
    {
    case LOG_LEVEL::DEBUG:
        Serial.print("DEBUG: ");
        break;
    case LOG_LEVEL::INFO:
        Serial.print("INFO: ");
        break;
    case LOG_LEVEL::WARNING:
        Serial.print("WARNING: ");
        break;
    case LOG_LEVEL::ERROR:
        Serial.print("ERROR: ");
        break;
    default:
        Serial.print("UNKNOWN: ");
        break;
    }
    Serial.println(message);
}

#endif // ARDUINO