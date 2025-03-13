#include "catch.hpp"
#include "UART.h"
#include <cstring>


// Fake UART class for testing
class FakeUART : public UART
{
  public:
    FakeUART() : UART() {}

    uint8_t send_buffer[1024];
    size_t send_buffer_size = 0;
    size_t Send(const uint8_t *data, const size_t data_size) override
    {
        if (data_size > sizeof(send_buffer))
        {
            throw std::runtime_error("Packet too big for send buffer");
        }
        std::memcpy(send_buffer, data, data_size);
        send_buffer_size = data_size;
        return data_size;
    }

    uint8_t receive_buffer[1024];
    size_t receive_buffer_size = 0;
    size_t Receive(uint8_t *data, const size_t data_size) override
    {
        if (receive_buffer_size > data_size)
        {
            throw std::runtime_error("Receive buffer too small");
        }
        std::memcpy(data, receive_buffer, receive_buffer_size);
        return receive_buffer_size;
    }

    const char *log_message;
    void Log(LOG_LEVEL level, const char *message) override
    {
        // std::cout << message << std::endl;
        log_message = message;
    }
};

TEST_CASE("Test sending integer packets")
{
    FakeUART uart;
    
    // Test sending a packet with an integer
    Payload payload;
    payload.WriteInt(313);
    uart.SendUARTPacket(1, payload);
    uart.Update();
    
    REQUIRE(uart.send_buffer_size == 9);
    uint8_t expected[] = {START_BYTE, 0x01, 0x04, 0x39, 0x01, 0x00, 0x00, 0x3f, END_BYTE};
    REQUIRE(std::memcmp(uart.send_buffer, expected, sizeof(expected)) == 0);
    
    // Test sending another packet with an integer
    Payload payload2;
    payload2.WriteInt(312);
    uart.SendUARTPacket(1, payload2);
    uart.Update();

    
    REQUIRE(uart.send_buffer_size == 9);
    uint8_t expected2[] = {START_BYTE, 0x01, 0x04, 0x38, 0x01, 0x00, 0x00, 0x3e, END_BYTE};
    REQUIRE(std::memcmp(uart.send_buffer, expected2, sizeof(expected2)) == 0);
}

TEST_CASE("Test sending float packets")
{
    FakeUART uart;
    
    // Test sending a packet with a float
    Payload payload;
    payload.WriteFloat(3.1415926f);
    uart.SendUARTPacket(2, payload);
    uart.Update();
    
    REQUIRE(uart.send_buffer_size == 9);
    uint8_t expected[] = {START_BYTE, 0x02, 0x04, 0xda, 0x0f, 0x49, 0x40, 0x78, END_BYTE};
    REQUIRE(std::memcmp(uart.send_buffer, expected, sizeof(expected)) == 0);
}

TEST_CASE("Test sending boolean packets")
{
    FakeUART uart;
    
    // Test sending a packet with a boolean true
    Payload payload;
    payload.WriteBool(true);
    uart.SendUARTPacket(3, payload);
    uart.Update();
    
    REQUIRE(uart.send_buffer_size == 6);
    uint8_t expected[] = {START_BYTE, 0x03, 0x01, 0x01, 0x05, END_BYTE};
    REQUIRE(std::memcmp(uart.send_buffer, expected, sizeof(expected)) == 0);
    
    // Test sending a packet with a boolean false
    Payload payload2;
    payload2.WriteBool(false);
    uart.SendUARTPacket(3, payload2);
    uart.Update();
    
    REQUIRE(uart.send_buffer_size == 6);
    uint8_t expected2[] = {START_BYTE, 0x03, 0x01, 0x00, 0x04, END_BYTE};
    REQUIRE(std::memcmp(uart.send_buffer, expected2, sizeof(expected2)) == 0);
}

TEST_CASE("Test sending raw byte packets")
{
    FakeUART uart;
    
    // Test sending a packet with raw bytes
    Payload payload;
    uint8_t rawBytes[] = {0x01, 0x02, 0x03, 0x04};
    payload.WriteBytes(rawBytes, 4);
    uart.SendUARTPacket(4, payload);
    uart.Update();
    
    REQUIRE(uart.send_buffer_size == 9);
    uint8_t expected[] = {START_BYTE, 0x04, 0x04, 0x01, 0x02, 0x03, 0x04, 0x12, END_BYTE};
    REQUIRE(std::memcmp(uart.send_buffer, expected, sizeof(expected)) == 0);
}

TEST_CASE("Test byte stuffing")
{
    FakeUART uart;
    
    // Test start byte stuffing
    Payload payload1;
    payload1.WriteInt((int)START_BYTE);
    uart.SendUARTPacket(1, payload1);
    uart.Update();
    
    REQUIRE(uart.send_buffer_size == 10);
    uint8_t expected1[] = {START_BYTE, 0x01, 0x04, ESCAPE_BYTE, START_BYTE ^ ESCAPE_MASK, 0x00, 0x00, 0x00, 0x83, END_BYTE};
    REQUIRE(std::memcmp(uart.send_buffer, expected1, sizeof(expected1)) == 0);
    
    // Test end byte stuffing
    Payload payload2;
    payload2.WriteInt((int)END_BYTE);
    uart.SendUARTPacket(1, payload2);
    uart.Update();
    
    REQUIRE(uart.send_buffer_size == 10);
    uint8_t expected2[] = {START_BYTE, 0x01, 0x04, ESCAPE_BYTE, END_BYTE ^ ESCAPE_MASK, 0x00, 0x00, 0x00, 0x84, END_BYTE};
    REQUIRE(std::memcmp(uart.send_buffer, expected2, sizeof(expected2)) == 0);
    
    // Test escape byte stuffing
    Payload payload3;
    payload3.WriteInt((int)ESCAPE_BYTE);
    uart.SendUARTPacket(1, payload3);
    uart.Update();
    
    REQUIRE(uart.send_buffer_size == 10);
    uint8_t expected3[] = {START_BYTE, 0x01, 0x04, ESCAPE_BYTE, ESCAPE_BYTE ^ ESCAPE_MASK, 0x00, 0x00, 0x00, 0x82, END_BYTE};
    REQUIRE(std::memcmp(uart.send_buffer, expected3, sizeof(expected3)) == 0);
}

// TODO: large packets and error conditions
// Zero length packets
// doubles