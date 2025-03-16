// The integer id for each of the packet.
// Each packet structure will need to bed definned in the README
// Each client then need to separately implement the encoder/decoder
// NEVER REUSE AN ID

#ifndef PACKETID_H
#define PACKETID_H

enum class PacketId
{
    ControlInput = 1,
    ControlOutput = 2,
};

#endif // PACKETID_H