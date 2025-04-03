#include <iostream>
#include <string>
#include <bitset>
const  int FORWARED =1;
const int BACKWARD =2;
const int LEFT =3;
const int RIGHT =4;
const int HEADERSIZE;

using namespace std;

class pktdef {
	int localcount = 0;
	char* RawBuffer;
	enum  cmdType
	{
		DRIVE, SLEEP, RESPONSE
	};

	//packet header
	struct Header
	{
		int pktcount : 2;
		int drive : 1;
		int status : 1;
		int sleep : 1;
		int ack : 1;
		int padding : 4;
		unsigned short int pktlength : 2;

	}; 
	
	//body struct that contains drivebody
	struct drivebody {
		int direction : 1;
		unsigned char duration : 1;
		int speed : 1;
	};


	//might need to change after
	struct CRC
	{
		int crc : 10;

	};

	//packet struct
	struct cmdPacket
	{
		Header header;
		drivebody body;
		CRC crc;
	};

	cmdPacket packet; // Packet structure to hold the header, body, and CRC
	Header header;
	drivebody body;
	CRC crc;

	void SetCmd(cmdType type) {
		// Set the command type based on the enum value
		//setting to sleep
		if (type == 1 ) {
			if (header.drive == 1) {
				cout << "can't go from drive to sleep" << endl;
			}
			else
			{
				header.drive = 0;
				header.sleep = 1;
				header.status = 0;

			}
		}
		//setting to drive
		else if (type == 0) {
			header.drive = 1;
			header.sleep = 0;
			header.status = 0;
		}
		//setting to wait 
		else if (type == 2){
			header.drive = 0;
			header.sleep = 0;
			header.status = 1;
		}
		
		else
		{
			cout << "invalid input" << endl;
		}
		
	}

	void SetBodyData(char* buffer, int size) {
		// Set the body data based on the command type
		//parse the data and set the body values
		int direction;
		unsigned char duration;
		int speed;

		// Parse the input string
		if (sscanf(buffer, "%d,%hhu,%d", &direction, &duration, &speed) == 3) {
			// Set the body values
			body.direction = direction;
			body.duration = duration;
			body.speed = speed;
		}
		else {
			cout << "Invalid input format" << endl;
		}
		
	}
	cmdType GetCmd() {
			// Return the command type based on the header values
			if (header.drive == 1) {
				return DRIVE;
			}
			else if (header.sleep == 1) {
				return SLEEP;
			}
			else if (header.status == 1) {
				return RESPONSE;
			}
			else {
				return DRIVE; // Default case
			}
	}

	bool GetAck() {
		if (header.ack == 1) {
			return true;
		}
		else {
			return false;
		}
	}

	int GetLength() {
		// Return the length of the packet
		return sizeof(packet);
	}
	

	void SetPktCount(int) {
		// Set the packet count
		header.pktcount = localcount;
		localcount++;
	}


	char* GetBodyData() {
		// Return the body data as a string
		char* buffer = new char[100]; // Allocate memory for the buffer
		sprintf(buffer, "%d,%hhu,%d", body.direction, body.duration, body.speed);
		return buffer;
	}

	int GetPktCount() {
		return header.pktcount;
	}

	bool CheckCRC(char*, int) {}

	void CalcCRC() {}

	char* GenPacket() {
		
	}
};

int main() {
	char* input;

	cout << "what is the direction, duration and speed you want the thingy to move, E.G. 1, 10, 90: " << endl;
	cin >> input;

}