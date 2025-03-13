#include "catch.hpp"
#include "UART.h"
#include <cstring>

int intReceived;
float floatReceived;
bool boolReceived;
uint8_t rawReceived[4];

// Handler to test receiving an integer
void intHandler(Payload &payload)
{
    intReceived = payload.ReadInt();
}

// Handler to test receiving a float
void floatHandler(Payload &payload)
{
    floatReceived = payload.ReadFloat();
}

// Handler to test receiving a bool
void boolHandler(Payload &payload)
{
    boolReceived = payload.ReadBool();
}

// Handler to test receiving raw bytes
void rawHandler(Payload &payload)
{
    payload.ReadBytes(rawReceived, 4);
}

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

TEST_CASE("Test receiving integer packets")
{
    FakeUART uart;
    uart.RegisterHandler(1, intHandler);
    uart.RegisterHandler(2, floatHandler);
    uart.RegisterHandler(3, boolHandler);
    uart.RegisterHandler(4, rawHandler);

    // Test receiving a packet with an integer
    uint8_t packet[] = {START_BYTE, 0x01, 0x04, 0x39, 0x01, 0x00, 0x00, 0x3f, END_BYTE};
    uart.receive_buffer_size = sizeof(packet);
    std::memcpy(uart.receive_buffer, packet, sizeof(packet));
    REQUIRE(uart.Update() == 1);
    REQUIRE(intReceived == 313);

    // Test receiving another packet with an integer
    intReceived = 0;
    uint8_t packet1[] = {START_BYTE, 0x01, 0x04, 0x38, 0x01, 0x00, 0x00, 0x3e, END_BYTE};
    uart.receive_buffer_size = sizeof(packet1);
    std::memcpy(uart.receive_buffer, packet1, sizeof(packet1));
    REQUIRE(uart.Update() == 1);
    REQUIRE(intReceived == 312);
}

TEST_CASE("Test receiving float packets")
{
    FakeUART uart;
    uart.RegisterHandler(1, intHandler);
    uart.RegisterHandler(2, floatHandler);
    uart.RegisterHandler(3, boolHandler);
    uart.RegisterHandler(4, rawHandler);

    // Test receiving a packet with a float
    uint8_t packet2[] = {START_BYTE, 0x02, 0x04, 0xda, 0x0f, 0x49, 0x40, 0x78, END_BYTE};
    uart.receive_buffer_size = sizeof(packet2);
    std::memcpy(uart.receive_buffer, packet2, sizeof(packet2));
    REQUIRE(uart.Update() == 1);

    // Fix for the float comparison - use separate tests instead of chained expressions
    const float expectedValue = 3.14159f;
    const float tolerance = 0.00001f;
    REQUIRE(floatReceived >= (expectedValue - tolerance));
    REQUIRE(floatReceived <= (expectedValue + tolerance));
}

TEST_CASE("Test receiving boolean packets")
{
    FakeUART uart;
    uart.RegisterHandler(1, intHandler);
    uart.RegisterHandler(2, floatHandler);
    uart.RegisterHandler(3, boolHandler);
    uart.RegisterHandler(4, rawHandler);

    // Test receiving a packet with a bool
    uint8_t packet3[] = {START_BYTE, 0x03, 0x01, 0x01, 0x05, END_BYTE};
    uart.receive_buffer_size = sizeof(packet3);
    std::memcpy(uart.receive_buffer, packet3, sizeof(packet3));
    REQUIRE(uart.Update() == 1);
    REQUIRE(boolReceived == true);
}

TEST_CASE("Test receiving raw byte packets")
{
    FakeUART uart;
    uart.RegisterHandler(1, intHandler);
    uart.RegisterHandler(2, floatHandler);
    uart.RegisterHandler(3, boolHandler);
    uart.RegisterHandler(4, rawHandler);

    // Test receiving a packet with raw bytes
    uint8_t packet4[] = {START_BYTE, 0x04, 0x04, 0x01, 0x02, 0x03, 0x04, 0x12, END_BYTE};
    uart.receive_buffer_size = sizeof(packet4);
    std::memcpy(uart.receive_buffer, packet4, sizeof(packet4));
    REQUIRE(uart.Update() == 1);
    REQUIRE(rawReceived[0] == 1);
    REQUIRE(rawReceived[1] == 2);
    REQUIRE(rawReceived[2] == 3);
    REQUIRE(rawReceived[3] == 4);
}

TEST_CASE("Test noise rejection")
{
    FakeUART uart;
    uart.RegisterHandler(1, intHandler);
    uart.RegisterHandler(2, floatHandler);
    uart.RegisterHandler(3, boolHandler);
    uart.RegisterHandler(4, rawHandler);

    // Test noise rejection
    // These bytes only contain two valid packets
    intReceived = 0;
    boolReceived = 1;
    uint8_t packet5[] = {0xe0, ESCAPE_BYTE, 0x33, END_BYTE, 0xf6, START_BYTE, 0x21, 0x31, 0x00,
                         0x33, START_BYTE, 0x01, 0x04, 0x54, START_BYTE, START_BYTE, 0x03, 0x01,
                         0x00, 0x04, END_BYTE, ESCAPE_BYTE, START_BYTE, 0x01, 0x05, ESCAPE_BYTE,
                         START_BYTE, 0x01, 0x04, 0x39, 0x01, 0x00, 0x00, 0x3f, END_BYTE, 0x33};
    uart.receive_buffer_size = sizeof(packet5);
    std::memcpy(uart.receive_buffer, packet5, sizeof(packet5));
    REQUIRE(uart.Update() == 2);
    REQUIRE(intReceived == 313);
    REQUIRE(boolReceived == false);
}

TEST_CASE("Test receiving packets in chunks")
{
    FakeUART uart;
    uart.RegisterHandler(1, intHandler);
    uart.RegisterHandler(2, floatHandler);
    uart.RegisterHandler(3, boolHandler);
    uart.RegisterHandler(4, rawHandler);

    // Test receiving packets broken in chunks
    intReceived = 0;
    uint8_t packet6a[] = {0x33, START_BYTE, 0x01, 0x04, 0x39};
    uint8_t packet6b[] = {0x01, 0x00, 0x00, 0x3f, END_BYTE, 0x33};

    uart.receive_buffer_size = sizeof(packet6a);
    std::memcpy(uart.receive_buffer, packet6a, sizeof(packet6a));
    REQUIRE(uart.Update() == 0);

    uart.receive_buffer_size = sizeof(packet6b);
    std::memcpy(uart.receive_buffer, packet6b, sizeof(packet6b));
    REQUIRE(uart.Update() == 1);
    REQUIRE(intReceived == 313);
}

TEST_CASE("Test error handling")
{
    FakeUART uart;
    uart.RegisterHandler(1, intHandler);
    uart.RegisterHandler(2, floatHandler);
    uart.RegisterHandler(3, boolHandler);
    uart.RegisterHandler(4, rawHandler);

    // Test invalid packet id
    uint8_t packet7[] = {START_BYTE, 0xFF, 0x01, 0x01, 0x05, END_BYTE};
    uart.receive_buffer_size = sizeof(packet7);
    std::memcpy(uart.receive_buffer, packet7, sizeof(packet7));
    REQUIRE(uart.Update() == 0);
    REQUIRE(std::strcmp(uart.log_message, "Invalid packet ID received") == 0);

    // Test invalid checksum
    uint8_t packet8[] = {START_BYTE, 0x03, 0x01, 0x01, 0xFF, END_BYTE};
    uart.receive_buffer_size = sizeof(packet8);
    std::memcpy(uart.receive_buffer, packet8, sizeof(packet8));
    REQUIRE(uart.Update() == 0);
    REQUIRE(std::strcmp(uart.log_message, "Invalid checksum received") == 0);
}

TEST_CASE("Test byte unstuffing")
{
    FakeUART uart;
    uart.RegisterHandler(1, intHandler);
    uart.RegisterHandler(2, floatHandler);
    uart.RegisterHandler(3, boolHandler);
    uart.RegisterHandler(4, rawHandler);

    // Test start byte unstuffing
    intReceived = 0;
    uint8_t packet1[] = {START_BYTE, 0x01, 0x04, ESCAPE_BYTE, START_BYTE ^ ESCAPE_MASK, 0x00, 0x00, 0x00, 0x83, END_BYTE};
    uart.receive_buffer_size = sizeof(packet1);
    std::memcpy(uart.receive_buffer, packet1, sizeof(packet1));
    REQUIRE(uart.Update() == 1);
    REQUIRE(intReceived == (int)START_BYTE);

    // Test end byte unstuffing
    intReceived = 0;
    uint8_t packet2[] = {START_BYTE, 0x01, 0x04, ESCAPE_BYTE, END_BYTE ^ ESCAPE_MASK, 0x00, 0x00, 0x00, 0x84, END_BYTE};
    uart.receive_buffer_size = sizeof(packet2);
    std::memcpy(uart.receive_buffer, packet2, sizeof(packet2));
    REQUIRE(uart.Update() == 1);
    REQUIRE(intReceived == (int)END_BYTE);

    // Test escape byte unstuffing
    intReceived = 0;
    uint8_t packet3[] = {START_BYTE, 0x01, 0x04, ESCAPE_BYTE, ESCAPE_BYTE ^ ESCAPE_MASK, 0x00, 0x00, 0x00, 0x82, END_BYTE};
    uart.receive_buffer_size = sizeof(packet3);
    std::memcpy(uart.receive_buffer, packet3, sizeof(packet3));
    REQUIRE(uart.Update() == 1);
    REQUIRE(intReceived == (int)ESCAPE_BYTE);
}

// TODO: More realistic tests
// float d1 = 0;
// float d2 = 0;
// float thrust = 0;
// float mz = 0;
// void realHandler(Payload &payload)
// {
//     d1 = payload.ReadFloat();
//     d2 = payload.ReadFloat();
//     thrust = payload.ReadFloat();
//     mz = payload.ReadFloat();
// }

// TEST_CASE("Test more realistic packets")
// {
//     FakeUART uart;
//     uart.RegisterHandler(1, &realHandler);
//     uint8_t packet[] = {START_BYTE, 0x01, 0x10,
//                         0x87, 0x16, 0xab, 0x40, // d1 c051e1b1
//                         0xb1, 0xe1, 0x51, 0xc0,
//                         0x00, END_BYTE};
//     uart.receive_buffer_size = sizeof(packet);
//     std::memcpy(uart.receive_buffer, packet, sizeof(packet));
//     REQUIRE(uart.Update() == 1);

//     float expectedD1 =  5.3465f;
//     float expectedD2 = -3.2794f;
// }

// TODO: Test byte stuffing and big packets
// And filling up buffers
// And end-to-end tests
// And doubles