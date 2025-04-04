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
	unsigned int direction : 8;  // 1 byte
	unsigned int duration : 8;  // 1 byte
	unsigned int speed : 8;  // 1 byte
};

// CRC struct: now 8 bits.
struct CRC {
	unsigned int crc : 8;        // 1 byte
};

// The complete packet structure is made up of Header, drivebody, and CRC.
struct cmdPacket {
	Header header;
	drivebody body;
	CRC crc;
};


class pktdef {
	int localcount = 0;
	char* RawBuffer;

	cmdPacket packet; // Packet structure to hold the header, body, and CRC


	pktdef() : localcount(0), RawBuffer(nullptr) {
		memset(&packet, 0, sizeof(packet));
	}
	pktdef(char* buffer) : localcount(0), RawBuffer(nullptr) {
		packet.header.pktcount = buffer[0] | (buffer[1] << 8);
		packet.header.drive = (buffer[2] & 0x80) >> 7;
		packet.header.status = (buffer[2] & 0x40) >> 6;
		packet.header.sleep = (buffer[2] & 0x20) >> 5;
		packet.header.ack = (buffer[2] & 0x10) >> 4;
		packet.header.padding = (buffer[2] & 0x0F);
		packet.header.pktlength = buffer[3] | (buffer[4] << 8);
		packet.body.direction = buffer[5];
		packet.body.duration = buffer[6];
		packet.body.speed = buffer[7];
		packet.crc.crc = buffer[8];
	}
	void SetCmd(cmdType type) {
		if (type == SLEEP && packet.header.drive == 1) {
			cout << "Error: Cannot transition directly from DRIVE to SLEEP. Transition to waiting first." << endl;
			return;
		}
		// Clear all flags first.
		packet.header.drive = 0;
		packet.header.status = 0;
		packet.header.sleep = 0;
		packet.header.ack = 0;
		// Set the appropriate flag based on the command type.
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

	void SetBodyData(char* buffer, int size) {
		int direction;
		unsigned int duration;
		int speed;
		if (sscanf(buffer, "%d,%u,%d", &direction, &duration, &speed) == 3) {
			packet.body.direction = direction;
			packet.body.duration = duration;
			packet.body.speed = speed;
		}
		else {
			cout << "Invalid input format" << endl;
		}
	}
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

	int GetLength() {
		return PACKET_SIZE;
	}
	

	void SetPktCount(int count) {
		packet.header.pktcount = count;
		localcount = count;
	}


	char* GetBodyData() {
		char* buff = new char[50];
		sprintf(buff, "%d,%d,%d", packet.body.direction, packet.body.duration, packet.body.speed);
		return buff;
	}

	int GetPktCount() {
		return packet.header.pktcount;
	}

	bool CheckCRC(char* buffer, int size) {
		if (size < PACKET_SIZE)
			return false;
		int computed = 0;
		for (int i = 0; i < PACKET_SIZE - 1; i++) {
			unsigned char byte = static_cast<unsigned char>(buffer[i]);
			for (int j = 0; j < 8; j++) {
				computed += (byte & 0x01);
				byte >>= 1;
			}
		}
		unsigned char received = static_cast<unsigned char>(buffer[PACKET_SIZE - 1]);
		return (computed == received);
	}

	void CalcCRC() {
		unsigned char temp[8];
		temp[0] = packet.header.pktcount & 0xFF;
		temp[1] = (packet.header.pktcount >> 8) & 0xFF;
		temp[2] = (packet.header.drive << 7) |
			(packet.header.status << 6) |
			(packet.header.sleep << 5) |
			(packet.header.ack << 4) |
			(packet.header.padding & 0x0F);
		temp[3] = packet.header.pktlength & 0xFF;
		temp[4] = (packet.header.pktlength >> 8) & 0xFF;
		temp[5] = packet.body.direction & 0xFF;
		temp[6] = packet.body.duration & 0xFF;
		temp[7] = packet.body.speed & 0xFF;

		int count = 0;
		for (int i = 0; i < 8; i++) {
			unsigned char byte = temp[i];
			for (int j = 0; j < 8; j++) {
				count += (byte & 0x01);
				byte >>= 1;
			}
		}
		packet.crc.crc = count & 0xFF;  
	}

	char* GenPacket() {
		packet.header.pktlength = PACKET_SIZE;
		CalcCRC();
		RawBuffer = new char[PACKET_SIZE];
		RawBuffer[0] = packet.header.pktcount & 0xFF;
		RawBuffer[1] = (packet.header.pktcount >> 8) & 0xFF;
		RawBuffer[2] = (packet.header.drive << 7) |
			(packet.header.status << 6) |
			(packet.header.sleep << 5) |
			(packet.header.ack << 4) |
			(packet.header.padding & 0x0F);
		RawBuffer[3] = packet.header.pktlength & 0xFF;
		RawBuffer[4] = (packet.header.pktlength >> 8) & 0xFF;
		RawBuffer[5] = packet.body.direction;
		RawBuffer[6] = packet.body.duration;
		RawBuffer[7] = packet.body.speed;
		RawBuffer[8] = packet.crc.crc;
		return RawBuffer;
	}
};

int main() {
	char* input;

	cout << "what is the direction, duration and speed you want the thingy to move, E.G. 1, 10, 90: " << endl;
	cin >> input;

}