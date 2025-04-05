#pragma once

#include <cassert>
#include <cstring>
#include <cstdio>

// Constant definitions
const int FORWARD = 1;
const int BACKWARD = 2;
const int LEFT = 3;
const int RIGHT = 4;
const int HEADERSIZE = 5;           // 2 bytes (PktCount) + 1 byte (flags) + 2 bytes (Length)
const int PACKET_SIZE = 9;          // For DRIVE commands: header (5) + drivebody (3) + CRC (1)
const int TELEMETRY_PACKET_SIZE = 15; // For TELEMETRY responses: header (5) + telemetry body (9) + CRC (1)

// Command types
enum cmdType {
    DRIVE,
    SLEEP,
    RESPONSE
};

// Define drivebody structure (3 bytes)
struct drivebody {
    unsigned int direction : 8;
    unsigned int duration : 8;
    unsigned int speed : 8;
};

// Define telemetryBody structure (9 bytes)
struct telemetryBody {
    unsigned short lastPktCounter;
    unsigned short currentGrade;
    unsigned short hitCount;
    unsigned char lastCmd;
    unsigned char lastCmdValue;
    unsigned char lastCmdSpeed;
};

// Define BodyUnion so that the packet body can be interpreted as either drive or telemetry data.
union BodyUnion {
    drivebody drive;
    telemetryBody telemetry;
};

// Define the Header structure (5 bytes total)
struct Header {
    unsigned short pktcount : 16;  // 2 bytes
    unsigned int drive : 1;       // 1 bit
    unsigned int status : 1;       // 1 bit
    unsigned int sleep : 1;       // 1 bit
    unsigned int ack : 1;       // 1 bit
    unsigned int padding : 4;       // 4 bits (always zero)
    unsigned short pktlength : 16; // 2 bytes
};

// Define the CRC structure (1 byte)
struct CRC {
    unsigned int crc : 8;
};

// Define the complete packet structure.
struct cmdPacket {
    Header header;
    BodyUnion body;
    CRC crc;
};

// Declaration of the pktdef class.
class pktdef {
public:
    // Constructors
    pktdef();
    pktdef(char* buffer, int size);

    // Member functions
    void SetCmd(cmdType type);
    void SetBodyData(char* buffer, int size);
    cmdType GetCmd();
    bool GetAck();
    int GetLength();
    void SetPktCount(int count);
    char* GetBodyData();
    int GetPktCount();
    bool CheckCRC(char* buffer, int size);
    void CalcCRC();
    char* GenPacket();

private:
    int localcount;
    char* RawBuffer;
    cmdPacket packet;
};