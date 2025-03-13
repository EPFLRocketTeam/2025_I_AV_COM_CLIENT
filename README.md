# UART Communication Protocol
## Overview
This protocol defines a structured way to send and receive data over UART. Each packet contains a **start byte, length field, identifier, payload, checksum, and end byte**.

## Packet Structure

Each packet follows this format:

```
[ Start Byte | ID | Length | Payload | Checksum | End Byte ]
```

### Field Descriptions

| Field          | Size (bytes) | Description                                                                |
| -------------- | ------------ | -------------------------------------------------------------------------- |
| **Start Byte** | 1            | Marks the beginning of a packet (`0x7E`)                                   |
| **ID**         | 1            | Identifies the type of packet                                              |
| **Length**     | 1            | Number of bytes in payload                                                 |
| **Payload**    | Variable     | Data being transmitted                                                     |
| **Checksum**   | 1            | XOR of all bytes from `ID` to `Payload` (error detection)              |
| **End Byte**   | 1            | Marks the end of the packet (`0x7F`)                                       |

## Packet Processing

### Encoding Steps:

1. Start with `0x7E` as the **Start Byte**.
3. Add an **ID** field for identifying packet type.
2. Compute the **Length** field (total bytes of payload).
4. Insert the **Payload**.
5. Compute the **Checksum** as an XOR of all bytes from `Length` to `Payload`.
5. Byte stuff everything except the **Start Byte** and **End Byte**.
6. Append the **End Byte**.

### Decoding Steps:

1. Wait for a **Start Byte**.
2. When reading bytes, make sure to unstuff them.
2. Read the **Length** field to determine expected bytes.
3. Extract **ID, Payload, and Checksum**.
4. Verify the **Checksum** (if incorrect, discard packet).
5. Check for the **End Byte**.

## Byte Stuffing
### When sending a packet:  
If the start byte (`0x7E`) or end byte (`0x7F`) appears in the payload, replace it with an escape byte (0x7D) followed by a modified version of the byte.
The modified byte is obtained by XORing the original byte with 0x20.

### When receiving a packet:
If an escape byte (`0x7D`) is detected, take the next byte and XOR it with 0x20 to recover the original value.

## Error Handling
- **Checksum Mismatch:** Packet is discarded.
- **Invalid Length Field:** Ignore the packet.
- **Missing End Byte (if expected):** Timeout or resync.

## Running the tests
To compile the tests, execute the following command from the root of the project:
```bash
mkdir build_local
cd build_local
cmake .. -DENABLE_TESTS=ON
make
```

To run the tests, execute the following command from the build directory of the project:
```bash
./com_client/tests/test_com_client
```