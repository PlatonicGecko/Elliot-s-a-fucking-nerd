#include "drive.h"
#include <iostream>
#include <cassert>
#include <cstring>
#include <cstdio>

using namespace std;

// Default constructor: sets all fields to zero.
pktdef::pktdef() : localcount(0), RawBuffer(nullptr) {
    // Initialize header fields.
    packet.header.pktcount = 0;
    packet.header.drive = 0;      // Not a drive command.
    packet.header.status = 1;     // RESPONSE state.
    packet.header.sleep = 0;
    packet.header.ack = 0;
    packet.header.padding = 0;
    packet.header.pktlength = 6;  // 6 bytes: header (5) + CRC (1)

    // Clear the union completely.
    packet.body.drive.direction = 0;
    packet.body.drive.duration = 0;
    packet.body.drive.speed = 0;
    packet.body.telemetry.lastPktCounter = 0;
    packet.body.telemetry.currentGrade = 0;
    packet.body.telemetry.hitCount = 0;
    packet.body.telemetry.lastCmd = 0;
    packet.body.telemetry.lastCmdValue = 0;
    packet.body.telemetry.lastCmdSpeed = 0;

    // Initialize CRC.
    packet.crc.crc = 0;
}


// Overloaded constructor: parses a received raw data buffer.
// 'size' is the number of bytes in the received packet.
pktdef::pktdef(char* buffer, int size) : localcount(0), RawBuffer(nullptr) {
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
    else if (size == 6) {  // 6-byte response packet (no body)
        packet.header.pktcount = buffer[0] | (buffer[1] << 8);
        packet.header.drive = (buffer[2] & 0x80) >> 7;
        packet.header.status = (buffer[2] & 0x40) >> 6;
        packet.header.sleep = (buffer[2] & 0x20) >> 5;
        packet.header.ack = (buffer[2] & 0x10) >> 4;
        packet.header.padding = (buffer[2] & 0x0F);
        packet.header.pktlength = buffer[3] | (buffer[4] << 8);
        // No body is present.
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
        // Parse telemetry body (9 bytes)
        packet.body.telemetry.lastPktCounter = buffer[5] | (buffer[6] << 8);
        packet.body.telemetry.currentGrade = buffer[7] | (buffer[8] << 8);
        packet.body.telemetry.hitCount = buffer[9] | (buffer[10] << 8);
        packet.body.telemetry.lastCmd = buffer[11];
        packet.body.telemetry.lastCmdValue = buffer[12];
        packet.body.telemetry.lastCmdSpeed = buffer[13];
        packet.crc.crc = buffer[14];
    }
    else {
        throw std::invalid_argument("Unexpected packet size in overloaded constructor.");
    }
}

// SetCmd: sets the command flag based on the provided cmdType.
void pktdef::SetCmd(cmdType type) {
    // Clear flags.
    packet.header.drive = 0;
    packet.header.status = 0;
    packet.header.sleep = 0;
    packet.header.ack = 0;
    // Also clear the entire union.
    packet.body.drive.direction = 0;
    packet.body.drive.duration = 0;
    packet.body.drive.speed = 0;
    packet.body.telemetry.lastPktCounter = 0;
    packet.body.telemetry.currentGrade = 0;
    packet.body.telemetry.hitCount = 0;
    packet.body.telemetry.lastCmd = 0;
    packet.body.telemetry.lastCmdValue = 0;
    packet.body.telemetry.lastCmdSpeed = 0;

    // Set flag according to the parameter.
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

// SetBodyData: if the status flag is set, expect telemetry data; otherwise, expect drive data.
void pktdef::SetBodyData(char* buffer, int size) {
    if (packet.header.status == 1) {  // Telemetry expected.
        unsigned int lastPktCounter, currentGrade, hitCount;
        unsigned int lastCmd, lastCmdValue, lastCmdSpeed;
        if (sscanf_s(buffer, "%u,%u,%u,%u,%u,%u",
            &lastPktCounter, &currentGrade, &hitCount,
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
    else {  // DRIVE command (or SLEEP response: drivebody remains zero)
        int direction;
        unsigned int duration;
        int speed;
        if (sscanf_s(buffer, "%d,%u,%d", &direction, &duration, &speed) == 3) {
            packet.body.drive.direction = direction;
            packet.body.drive.duration = duration;
            packet.body.drive.speed = speed;
        }
        else {
            cout << "Invalid drive input format" << endl;
        }
    }
}

// GetCmd: returns the current command type based on header flags.
cmdType pktdef::GetCmd() {
    if (packet.header.drive)
        return DRIVE;
    else if (packet.header.sleep)
        return SLEEP;
    else if (packet.header.status)
        return RESPONSE;
    else
        return DRIVE;
}

// GetAck: returns true if the ACK flag is set.
bool pktdef::GetAck() {
    return (packet.header.ack == 1);
}

// GetLength: returns the packet length based on the header flags.
int pktdef::GetLength() {
    // If status flag is set, determine if telemetry data is loaded.
    if (packet.header.status == 1) {
        // If any telemetry field is nonzero, assume telemetry packet.
        if (packet.body.telemetry.lastPktCounter != 0 ||
            packet.body.telemetry.currentGrade != 0 ||
            packet.body.telemetry.hitCount != 0 ||
            packet.body.telemetry.lastCmd != 0 ||
            packet.body.telemetry.lastCmdValue != 0 ||
            packet.body.telemetry.lastCmdSpeed != 0)
            return TELEMETRY_PACKET_SIZE;
        else
            return 6;  // Response packet with no body.
    }
    if (packet.header.drive == 1)
        return PACKET_SIZE;
    return 6;
}

// SetPktCount: sets the packet count.
void pktdef::SetPktCount(int count) {
    packet.header.pktcount = count;
    localcount = count;
}

// GetBodyData: returns a string representation of the packet body.
char* pktdef::GetBodyData() {
    char* buff = new char[100];
    if (packet.header.status == 1) {  // Telemetry
        sprintf_s(buff, 100, "%d,%d,%d,%d,%d,%d",
            packet.body.telemetry.lastPktCounter,
            packet.body.telemetry.currentGrade,
            packet.body.telemetry.hitCount,
            packet.body.telemetry.lastCmd,
            packet.body.telemetry.lastCmdValue,
            packet.body.telemetry.lastCmdSpeed);
    }
    else {  // DRIVE
        sprintf_s(buff, 100, "%d,%d,%d",
            packet.body.drive.direction,
            packet.body.drive.duration,
            packet.body.drive.speed);
    }
    return buff;
}

// GetPktCount: returns the current packet count.
int pktdef::GetPktCount() {
    return packet.header.pktcount;
}

// CheckCRC: computes the CRC over the header and body (excluding the CRC field) and compares it.
bool pktdef::CheckCRC(char* buffer, int size) {
    int totalSize = GetLength();
    if (size < totalSize)
        return false;
    int computed = 0;
    int crcSize = totalSize - 1; // Exclude CRC field.
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

// CalcCRC: calculates the CRC over the header and body (excluding the CRC field).
void pktdef::CalcCRC() {
    int totalSize = GetLength();
    int crcSize = totalSize - 1;
    unsigned char temp[14] = { 0 };  // Maximum required size is 14 bytes (for telemetry responses)

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

    if (totalSize == PACKET_SIZE) { // 9-byte DRIVE packet: include drivebody (3 bytes)
        temp[5] = packet.body.drive.direction & 0xFF;
        temp[6] = packet.body.drive.duration & 0xFF;
        temp[7] = packet.body.drive.speed & 0xFF;
    }
    else if (totalSize == TELEMETRY_PACKET_SIZE) { // 15-byte telemetry packet: include telemetry body (9 bytes)
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
}

// GenPacket: serializes the packet into a dynamically allocated raw buffer.
char* pktdef::GenPacket() {
    int length = GetLength();
    assert(length == 6 || length == PACKET_SIZE || length == TELEMETRY_PACKET_SIZE);
    packet.header.pktlength = length;
    CalcCRC();
    RawBuffer = new char[length];
    memset(RawBuffer, 0, length);

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
    else {
        for (int i = 5; i < length; i++) {
            RawBuffer[i] = 0;
        }
    }
    return RawBuffer;


}
int main() {}