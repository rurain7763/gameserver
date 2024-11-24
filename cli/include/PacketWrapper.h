#ifndef PACKET_WRAPPER_H
#define PACKET_WRAPPER_H

#include "Packet.h"
#include <msclr/marshal_cppstd.h>

using namespace System;
using namespace System::Runtime::InteropServices;

namespace CLIWrapper {
    public ref class PacketWrapper {
    private:
        flaw::Packet* packet;

    public:
        PacketWrapper(int senderId, short packetId, array<Byte>^ data) {
            std::vector<char> vecData(data->Length);
            Marshal::Copy(data, 0, IntPtr(vecData.data()), data->Length);
            packet = new flaw::Packet(senderId, packetId, vecData);
        }

        ~PacketWrapper() {
            this->!PacketWrapper();
        }

        !PacketWrapper() {
            delete packet;
        }

        flaw::Packet* GetPacket() {
            return packet;
        }
    };
}

#endif