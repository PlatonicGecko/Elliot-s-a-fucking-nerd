#include <iostream>
#include <string>
#include <bitset>
const int FORWARD = 1;
const int BACKWARD = 2;
const int LEFT = 3;
const int RIGHT = 4;
const int HEADERSIZE = 5;     
const int PACKET_SIZE = 9;

using namespace std;

enum  cmdType
{
	DRIVE, SLEEP, RESPONSE
};

struct Header {
	unsigned short pktcount : 16;  // 2 bytes
	unsigned int drive : 1;        // 1 bit
	unsigned int status : 1;        // 1 bit
	unsigned int sleep : 1;        // 1 bit
	unsigned int ack : 1;        // 1 bit
	unsigned int padding : 4;        // 4 bits (padding, always zero)
	unsigned short pktlength : 16;   // 2 bytes
};

// Body struct that contains the drive command parameters,
// each now 8 bits (1 byte) as required.
struct drivebody {
	unsigned int direction : 8;  
	unsigned int duration : 8;  
	unsigned int speed : 8;  
};

// CRC struct: now 8 bits.
struct CRC {
	unsigned int crc : 8;        
};

// The complete packet structure is made up of Header, drivebody, and CRC.
struct cmdPacket {
	Header header;
	BodyUnion body;
	CRC crc;
};

const int TELEMETRY_PACKET_SIZE = 15;

// For telemetry responses (from the robot), define the telemetry structure.
struct telemetryBody {
	unsigned short lastPktCounter; 
	unsigned short currentGrade;   
	unsigned short hitCount;       
	unsigned char lastCmd;         
	unsigned char lastCmdValue;    
	unsigned char lastCmdSpeed;   
};

union BodyUnion {
	drivebody drive;
	telemetryBody telemetry;
};


class pktdef {
	int localcount = 0;
	char* RawBuffer;

	cmdPacket packet; // Packet structure to hold the header, body, and CRC


    pktdef() : localcount(0), RawBuffer(nullptr) {
        memset(&packet, 0, sizeof(packet));
    }

    // Overloaded constructor to parse a received raw data buffer.
    // 'size' is the number of bytes received.
    pktdef(char* buffer, int size) : localcount(0), RawBuffer(nullptr) {
        if (size == PACKET_SIZE) {  // 9-byte DRIVE command packet
            packet.header.pktcount = buffer[0] | (buffer[1] << 8);
            packet.header.drive = (buffer[2] & 0x80) >> 7;
            packet.header.status = (buffer[2] & 0x40) >> 6;
            packet.header.sleep = (buffer[2] & 0x20) >> 5;
            packet.header.ack = (buffer[2] & 0x10) >> 4;
            packet.header.padding = (buffer[2] & 0x0F);
            packet.header.pktlength = buffer[3] | (buffer[4] << 8);
            packet.body.drive.direction = buffer[5];
            packet.body.drive.duration = buffer[6];
            packet.body.drive.speed = buffer[7];
            packet.crc.crc = buffer[8];
        }
        else if (size == 6) {  // 6-byte SLEEP/ACK response packet (no body)
            packet.header.pktcount = buffer[0] | (buffer[1] << 8);
            packet.header.drive = (buffer[2] & 0x80) >> 7;
            packet.header.status = (buffer[2] & 0x40) >> 6;
            packet.header.sleep = (buffer[2] & 0x20) >> 5;
            packet.header.ack = (buffer[2] & 0x10) >> 4;
            packet.header.padding = (buffer[2] & 0x0F);
            packet.header.pktlength = buffer[3] | (buffer[4] << 8);
            // Set drivebody to zero
            packet.body.drive.direction = 0;
            packet.body.drive.duration = 0;
            packet.body.drive.speed = 0;
            packet.crc.crc = buffer[5];
        }
        else if (size == TELEMETRY_PACKET_SIZE) {  // 15-byte TELEMETRY response packet
            packet.header.pktcount = buffer[0] | (buffer[1] << 8);
            packet.header.drive = (buffer[2] & 0x80) >> 7;
            packet.header.status = (buffer[2] & 0x40) >> 6;
            packet.header.sleep = (buffer[2] & 0x20) >> 5;
            packet.header.ack = (buffer[2] & 0x10) >> 4;
            packet.header.padding = (buffer[2] & 0x0F);
            packet.header.pktlength = buffer[3] | (buffer[4] << 8);
            // Parse telemetry body (9 bytes: bytes 5 to 13)
            packet.body.telemetry.lastPktCounter = buffer[5] | (buffer[6] << 8);
            packet.body.telemetry.currentGrade = buffer[7] | (buffer[8] << 8);
            packet.body.telemetry.hitCount = buffer[9] | (buffer[10] << 8);
            packet.body.telemetry.lastCmd = buffer[11];
            packet.body.telemetry.lastCmdValue = buffer[12];
            packet.body.telemetry.lastCmdSpeed = buffer[13];
            packet.crc.crc = buffer[14];
        }
        else {
            cout << "Unexpected packet size received." << endl;
        }
    }

    // Set the command type and adjust flags accordingly.
    void SetCmd(cmdType type) {
        if (type == SLEEP && packet.header.drive == 1) {
            cout << "Error: Cannot transition directly from DRIVE to SLEEP. Transition to waiting first." << endl;
            return;
        }
        // Clear all flags.
        packet.header.drive = 0;
        packet.header.status = 0;
        packet.header.sleep = 0;
        packet.header.ack = 0;
        // Set the flag based on the command type.
        switch (type) {
        case DRIVE:
            packet.header.drive = 1;
            break;
        case SLEEP:
            packet.header.sleep = 1;
            break;
        case RESPONSE:
            packet.header.status = 1;
            break;
        default:
            cout << "Invalid command type" << endl;
        }
    }

    // Set the body data.
    // For telemetry responses (when status flag is set), expect:
    // "lastPktCounter,currentGrade,hitCount,lastCmd,lastCmdValue,lastCmdSpeed"
    // Otherwise (DRIVE), expect: "direction,duration,speed"
    void SetBodyData(char* buffer, int size) {
        if (packet.header.status == 1) {  // Telemetry
            unsigned int lastPktCounter, currentGrade, hitCount;
            unsigned int lastCmd, lastCmdValue, lastCmdSpeed;
            if (sscanf(buffer, "%u,%u,%u,%u,%u,%u", &lastPktCounter, &currentGrade, &hitCount,
                &lastCmd, &lastCmdValue, &lastCmdSpeed) == 6) {
                packet.body.telemetry.lastPktCounter = static_cast<unsigned short>(lastPktCounter);
                packet.body.telemetry.currentGrade = static_cast<unsigned short>(currentGrade);
                packet.body.telemetry.hitCount = static_cast<unsigned short>(hitCount);
                packet.body.telemetry.lastCmd = static_cast<unsigned char>(lastCmd);
                packet.body.telemetry.lastCmdValue = static_cast<unsigned char>(lastCmdValue);
                packet.body.telemetry.lastCmdSpeed = static_cast<unsigned char>(lastCmdSpeed);
            }
            else {
                cout << "Invalid telemetry input format" << endl;
            }
        }
        else {  // DRIVE command or SLEEP response (drivebody remains zero for SLEEP)
            int direction;
            unsigned int duration;
            int speed;
            if (sscanf(buffer, "%d,%u,%d", &direction, &duration, &speed) == 3) {
                packet.body.drive.direction = direction;
                packet.body.drive.duration = duration;
                packet.body.drive.speed = speed;
            }
            else {
                cout << "Invalid drive input format" << endl;
            }
        }
    }

    // Return the command type based on flags.
    cmdType GetCmd() {
        if (packet.header.drive)
            return DRIVE;
        else if (packet.header.sleep)
            return SLEEP;
        else if (packet.header.status)
            return RESPONSE;
        else
            return DRIVE;
    }

    bool GetAck() {
        return (packet.header.ack == 1);
    }

    // Get the packet length based on flags.
    int GetLength() {
        if (packet.header.status == 1)
            return TELEMETRY_PACKET_SIZE;  // Telemetry response
        if (packet.header.drive == 1)
            return PACKET_SIZE;            // DRIVE command
        return 6;                          // SLEEP/ACK response (no body)
    }

    void SetPktCount(int count) {
        packet.header.pktcount = count;
        localcount = count;
    }

    // Return a string representation of the body.
    char* GetBodyData() {
        char* buff = new char[100];
        if (packet.header.status == 1) {  // Telemetry
            sprintf(buff, "%d,%d,%d,%d,%d,%d",
                packet.body.telemetry.lastPktCounter,
                packet.body.telemetry.currentGrade,
                packet.body.telemetry.hitCount,
                packet.body.telemetry.lastCmd,
                packet.body.telemetry.lastCmdValue,
                packet.body.telemetry.lastCmdSpeed);
        }
        else {  // DRIVE
            sprintf(buff, "%d,%d,%d",
                packet.body.drive.direction,
                packet.body.drive.duration,
                packet.body.drive.speed);
        }
        return buff;
    }

    int GetPktCount() {
        return packet.header.pktcount;
    }

    // Check CRC using the actual packet length.
    bool CheckCRC(char* buffer, int size) {
        int totalSize = GetLength();
        if (size < totalSize)
            return false;
        int computed = 0;
        int crcSize = totalSize - 1; // Exclude the CRC field.
        for (int i = 0; i < crcSize; i++) {
            unsigned char byte = static_cast<unsigned char>(buffer[i]);
            for (int j = 0; j < 8; j++) {
                computed += (byte & 0x01);
                byte >>= 1;
            }
        }
        unsigned char received = static_cast<unsigned char>(buffer[totalSize - 1]);
        return (computed == received);
    }

    // Calculate CRC over the header and body based on the packet type.
    void CalcCRC() {
        int totalSize = GetLength();
        int crcSize = totalSize - 1;
        unsigned char* temp = new unsigned char[crcSize];

        // Serialize header (5 bytes)
        temp[0] = packet.header.pktcount & 0xFF;
        temp[1] = (packet.header.pktcount >> 8) & 0xFF;
        temp[2] = (packet.header.drive << 7) |
            (packet.header.status << 6) |
            (packet.header.sleep << 5) |
            (packet.header.ack << 4) |
            (packet.header.padding & 0x0F);
        temp[3] = packet.header.pktlength & 0xFF;
        temp[4] = (packet.header.pktlength >> 8) & 0xFF;

        if (totalSize == PACKET_SIZE) { // DRIVE packet: include drivebody (3 bytes)
            temp[5] = packet.body.drive.direction & 0xFF;
            temp[6] = packet.body.drive.duration & 0xFF;
            temp[7] = packet.body.drive.speed & 0xFF;
        }
        else if (totalSize == TELEMETRY_PACKET_SIZE) { // Telemetry: include telemetry body (9 bytes)
            temp[5] = packet.body.telemetry.lastPktCounter & 0xFF;
            temp[6] = (packet.body.telemetry.lastPktCounter >> 8) & 0xFF;
            temp[7] = packet.body.telemetry.currentGrade & 0xFF;
            temp[8] = (packet.body.telemetry.currentGrade >> 8) & 0xFF;
            temp[9] = packet.body.telemetry.hitCount & 0xFF;
            temp[10] = (packet.body.telemetry.hitCount >> 8) & 0xFF;
            temp[11] = packet.body.telemetry.lastCmd;
            temp[12] = packet.body.telemetry.lastCmdValue;
            temp[13] = packet.body.telemetry.lastCmdSpeed;
        }
        // For 6-byte responses, no body is added.
        int count = 0;
        for (int i = 0; i < crcSize; i++) {
            unsigned char byte = temp[i];
            for (int j = 0; j < 8; j++) {
                count += (byte & 0x01);
                byte >>= 1;
            }
        }
        packet.crc.crc = count & 0xFF;
        delete[] temp;
    }

    // Generate a raw packet (serialize fields based on packet type).
    char* GenPacket() {
        int length = GetLength();
        packet.header.pktlength = length;
        CalcCRC();
        RawBuffer = new char[length];

        // Serialize header (5 bytes)
        RawBuffer[0] = packet.header.pktcount & 0xFF;
        RawBuffer[1] = (packet.header.pktcount >> 8) & 0xFF;
        RawBuffer[2] = (packet.header.drive << 7) |
            (packet.header.status << 6) |
            (packet.header.sleep << 5) |
            (packet.header.ack << 4) |
            (packet.header.padding & 0x0F);
        RawBuffer[3] = packet.header.pktlength & 0xFF;
        RawBuffer[4] = (packet.header.pktlength >> 8) & 0xFF;

        if (length == PACKET_SIZE) {
            RawBuffer[5] = packet.body.drive.direction;
            RawBuffer[6] = packet.body.drive.duration;
            RawBuffer[7] = packet.body.drive.speed;
            RawBuffer[8] = packet.crc.crc;
        }
        else if (length == 6) {
            RawBuffer[5] = packet.crc.crc;
        }
        else if (length == TELEMETRY_PACKET_SIZE) {
            RawBuffer[5] = packet.body.telemetry.lastPktCounter & 0xFF;
            RawBuffer[6] = (packet.body.telemetry.lastPktCounter >> 8) & 0xFF;
            RawBuffer[7] = packet.body.telemetry.currentGrade & 0xFF;
            RawBuffer[8] = (packet.body.telemetry.currentGrade >> 8) & 0xFF;
            RawBuffer[9] = packet.body.telemetry.hitCount & 0xFF;
            RawBuffer[10] = (packet.body.telemetry.hitCount >> 8) & 0xFF;
            RawBuffer[11] = packet.body.telemetry.lastCmd;
            RawBuffer[12] = packet.body.telemetry.lastCmdValue;
            RawBuffer[13] = packet.body.telemetry.lastCmdSpeed;
            RawBuffer[14] = packet.crc.crc;
        }
        return RawBuffer;

	}
};

int main() {

}