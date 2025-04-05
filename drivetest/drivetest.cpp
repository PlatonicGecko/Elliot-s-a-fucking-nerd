#include "pch.h"
#include "CppUnitTest.h"
#include "drive.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace Microsoft {
    namespace VisualStudio {
        namespace CppUnitTestFramework {

            template<>
            static std::wstring ToString<cmdType>(const cmdType& t)
            {
                switch (t)
                {
                case DRIVE:    return L"DRIVE";
                case SLEEP:    return L"SLEEP";
                case RESPONSE: return L"RESPONSE";
                default:       return L"UNKNOWN";
                }
            }

        }
    }
} // namespace Microsoft::VisualStudio::CppUnitTestFramework


namespace drivetest
{
	TEST_CLASS(drivetest)
	{
    public:

        TEST_METHOD(DefaultConstructorTest)
        {
            pktdef packet;
            Assert::AreEqual(RESPONSE, packet.GetCmd());
            Assert::AreEqual(6, packet.GetLength());
            Assert::AreEqual(0, packet.GetPktCount());
            Assert::AreEqual(false, packet.GetAck());
        }

        // Test the overloaded constructor for a DRIVE packet.
        TEST_METHOD(OverloadedConstructorDriveTest)
        {
            char raw[PACKET_SIZE];
            // pktcount = 1 (little-endian)
            raw[0] = 1;
            raw[1] = 0;
            // Flags: set DRIVE=1 in bit7.
            raw[2] = (1 << 7);
            // Length = 9
            raw[3] = PACKET_SIZE;
            raw[4] = 0;
            // drivebody: direction=1, duration=10, speed=90.
            raw[5] = 1;
            raw[6] = 10;
            raw[7] = 90;
            // Calculate CRC (simple parity over first 8 bytes).
            int crcVal = 0;
            for (int i = 0; i < PACKET_SIZE - 1; i++) {
                unsigned char byte = raw[i];
                for (int j = 0; j < 8; j++) {
                    crcVal += (byte & 1);
                    byte >>= 1;
                }
            }
            raw[8] = crcVal & 0xFF;

            pktdef packet(raw, PACKET_SIZE);
            Assert::AreEqual(1, packet.GetPktCount());
            Assert::AreEqual(DRIVE, packet.GetCmd());
            Assert::AreEqual(PACKET_SIZE, packet.GetLength());
            char* body = packet.GetBodyData();
            Assert::AreEqual(std::string("1,10,90"), std::string(body));
            delete[] body;
        }

        // Test the overloaded constructor for a SLEEP/ACK response packet.
        TEST_METHOD(OverloadedConstructorSleepTest)
        {
            char raw[6];
            raw[0] = 2; // pktcount = 2
            raw[1] = 0;
            // Flags: no drive, no status, no sleep (for basic response)
            raw[2] = 0;
            raw[3] = 6; // length = 6
            raw[4] = 0;
            int crcVal = 0;
            for (int i = 0; i < 5; i++) {
                unsigned char byte = raw[i];
                for (int j = 0; j < 8; j++) {
                    crcVal += (byte & 1);
                    byte >>= 1;
                }
            }
            raw[5] = crcVal & 0xFF;

            pktdef packet(raw, 6);
            Assert::AreEqual(2, packet.GetPktCount());
            // With no command flag set, our GetCmd() defaults to DRIVE.
            // Adjust expectation if needed.
            Assert::AreEqual(DRIVE, packet.GetCmd());
            Assert::AreEqual(6, packet.GetLength());
        }

        // Test the overloaded constructor for a TELEMETRY packet.
        TEST_METHOD(OverloadedConstructorTelemetryTest)
        {
            char raw[TELEMETRY_PACKET_SIZE];
            // pktcount = 3.
            raw[0] = 3;
            raw[1] = 0;
            // Flags: set status=1 (telemetry) in bit6.
            raw[2] = (1 << 6);
            // Length = 15.
            raw[3] = TELEMETRY_PACKET_SIZE;
            raw[4] = 0;
            // Telemetry body:
            // lastPktCounter = 5, currentGrade = 95, hitCount = 3,
            // lastCmd = 1, lastCmdValue = 10, lastCmdSpeed = 80.
            raw[5] = 5;  raw[6] = 0;
            raw[7] = 95; raw[8] = 0;
            raw[9] = 3;  raw[10] = 0;
            raw[11] = 1;
            raw[12] = 10;
            raw[13] = 80;
            int crcVal = 0;
            for (int i = 0; i < TELEMETRY_PACKET_SIZE - 1; i++) {
                unsigned char byte = raw[i];
                for (int j = 0; j < 8; j++) {
                    crcVal += (byte & 1);
                    byte >>= 1;
                }
            }
            raw[14] = crcVal & 0xFF;

            pktdef packet(raw, TELEMETRY_PACKET_SIZE);
            Assert::AreEqual(3, packet.GetPktCount());
            Assert::AreEqual(RESPONSE, packet.GetCmd());
            // Since telemetry fields are nonzero, GetLength should return 15.
            Assert::AreEqual(TELEMETRY_PACKET_SIZE, packet.GetLength());
            char* body = packet.GetBodyData();
            Assert::AreEqual(std::string("5,95,3,1,10,80"), std::string(body));
            delete[] body;
        }

        TEST_METHOD(SetCmdTest)
        {
            pktdef packet;
            packet.SetCmd(DRIVE);
            Assert::AreEqual(DRIVE, packet.GetCmd());

            packet.SetCmd(RESPONSE);
            Assert::AreEqual(RESPONSE, packet.GetCmd());

            packet.SetCmd(DRIVE);
            Assert::AreEqual(DRIVE, packet.GetCmd());

            packet.SetCmd(RESPONSE);
            Assert::AreEqual(RESPONSE, packet.GetCmd());

            packet.SetCmd(SLEEP);
            Assert::AreEqual(SLEEP, packet.GetCmd());


        }

        TEST_METHOD(SetBodyDataDriveTest)
        {
            pktdef packet;
            packet.SetCmd(DRIVE);
            char driveInput[] = "1,10,90";
            packet.SetBodyData(driveInput, sizeof(driveInput));
            char* body = packet.GetBodyData();
            // Expect the drivebody string representation to be "1,10,90"
            Assert::AreEqual(std::string("1,10,90"), std::string(body));
            delete[] body;
        }

        TEST_METHOD(SetBodyDataTelemetryTest)
        {
            pktdef packet;
            // Set command to RESPONSE to indicate telemetry
            packet.SetCmd(RESPONSE);
            char telemetryInput[] = "5,95,3,1,10,80";
            packet.SetBodyData(telemetryInput, sizeof(telemetryInput));
            char* body = packet.GetBodyData();
            Assert::AreEqual(std::string("5,95,3,1,10,80"), std::string(body));
            delete[] body;
        }

        TEST_METHOD(SetPktCountTest)
        {
            pktdef packet;
            packet.SetPktCount(5);
            Assert::AreEqual(5, packet.GetPktCount());
        }

        TEST_METHOD(GenPacketDriveTest)
        {
            pktdef packet;
            packet.SetPktCount(1);
            packet.SetCmd(DRIVE);
            char driveInput[] = "1,10,90";
            packet.SetBodyData(driveInput, sizeof(driveInput));
            char* buf = packet.GenPacket();
            Assert::AreEqual(PACKET_SIZE, packet.GetLength());
            Assert::IsTrue(packet.CheckCRC(buf, packet.GetLength()));
            delete[] buf;
        }

        TEST_METHOD(GenPacketSleepTest)
        {
            pktdef packet;
            packet.SetPktCount(2);
            packet.SetCmd(SLEEP);
            char* buf = packet.GenPacket();
            Assert::AreEqual(6, packet.GetLength());
            Assert::IsTrue(packet.CheckCRC(buf, packet.GetLength()));
            delete[] buf;
        }

        TEST_METHOD(GenPacketTelemetryTest)
        {
            pktdef packet;
            packet.SetPktCount(3);
            packet.SetCmd(RESPONSE);
            char telemetryInput[] = "5,95,3,1,10,80";
            packet.SetBodyData(telemetryInput, sizeof(telemetryInput));
            char* buf = packet.GenPacket();
            Assert::AreEqual(TELEMETRY_PACKET_SIZE, packet.GetLength());
            Assert::IsTrue(packet.CheckCRC(buf, packet.GetLength()));
            delete[] buf;
        }
    };
}
